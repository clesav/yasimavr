# gdb_server.py
#
# Copyright 2022 Clement Savergne <csavergne@yahoo.com>
#
# This file is part of yasim-avr.
#
# yasim-avr is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# yasim-avr is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.


import binascii
import struct
import threading
import socketserver
import time
import os, sys
from yasimavr.lib.core import DeviceDebugProbe, AsyncSimLoop, Device


#Templates for query replies and register descriptions

mem_map_query_tpl = '''<memory-map>
<memory type="flash" start="0" length="{flashlen}"/>
<memory type="ram" start="0x800000" length="{datalen}"/>
</memory-map>
'''

rx_desc_tpl = "name:{name};bitsize:8;offset:0;encoding:uint;format:hex;set:General Purpose Registers;gcc:{index};dwarf:{index};"

sreg_desc = "name:sreg;bitsize:8;offset:0;encoding:uint;format:binary;set:General Purpose Registers;gcc:32;dwarf:32;"

sp_desc = "name:sp;bitsize:16;offset:0;encoding:uint;format:hex;set:General Purpose Registers;gcc:33;dwarf:33;generic:sp;"

pc_desc = "name:pc;bitsize:32;offset:0;encoding:uint;format:hex;set:General Purpose Registers;gcc:34;dwarf:34;generic:pc;"


#This is like using struct.unpack but with arbitrary integer length
def _decode_hexa_int(hs):
    s = binascii.unhexlify(hs)
    v = 0
    for i, b in enumerate(s):
        v += b << (i * 8)
    return v


class _GDB_SocketHandler(socketserver.BaseRequestHandler):

    def handle(self):
        self.server._stub.handle_socket(self.request)


class _GDB_StubServer(socketserver.TCPServer):

    def __init__(self, conn_point, stub):
        super().__init__(conn_point, _GDB_SocketHandler)
        self._stub = stub


    def start(self):
        server_thread = threading.Thread(target=self.serve_forever)
        server_thread.daemon = True
        server_thread.start()


class GDB_Stub:
    """This class implements a stub that acts as an interface between
    yasimavr and the GNU Debugger. It is designed on top of a TCP server and
    implements the relevant parts of the Remote Serial Protocol.

    Source: https://www.sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html

    :param tuple[str, int] conn_point: Tuple (IP, port) for the socket to listen for GDB connections
    :param str fw_source: path to the firmware source code
    :param AsyncSimLoop simloop: Simulation loop to connect to
    :param Device device: Device simulation model to connect to

    .. note:: At least one of simloop or device should be specified.

    If simloop is provided, the stub will take control of it.
    If device is provided, the stub will create a simulation loop for it and dispose of it on shutdown.
    """

    def __init__(self, conn_point, fw_source, simloop=None, device=None):
        self._source = os.path.normpath(os.path.abspath(fw_source))
        self._source = self._source.replace('\\', '/')

        if simloop is not None:
            self._simloop = simloop
            self._device = simloop.device()
            self._ownloop = False
        elif device is not None:
            self._simloop = AsyncSimLoop(device)
            self._device = device
            self._ownloop = True
        else:
            raise ValueError('device and simloop cannot be both None')

        self._probe = DeviceDebugProbe(self._device)

        self._server = _GDB_StubServer(conn_point, self)
        self._socket = None

        self._simloopthread = None
        self._simloopjointhread = None

        self._verbose = False


    def set_simloop(self, simloop):
        if self._ownloop: return
        self._probe.detach()
        self._simloop = simloop
        self._device = simloop.device()
        self._probe = DeviceDebugProbe()
        self._probe.attach(self._device)


    def start(self):
        self._server.start()

        if self._ownloop:
            self._simloopthread = threading.Thread(target=self._simloop.run)
            self._simloopthread.start()


    def shutdown(self):
        self._probe.detach()
        self._server.shutdown()
        if self._ownloop:
            with self._simloop:
                self._simloop.loop_kill()

            self._simloopthread.join()


    def handle_socket(self, skt):
        if self._verbose:
            print('GDB Stub: Connection open')
            sys.stdout.flush()

        self._socket = skt
        packet_buffer = ''
        try:
            while 1:
                packet = str(skt.recv(1024), 'ascii')
                if not packet: break

                packet_buffer += packet

                n = self.__handle_packet(packet_buffer)

                if n < len(packet_buffer):
                    packet_buffer = packet_buffer[n:]
                else:
                    packet_buffer = ''
        except ConnectionError:
            #Exception raised when GDB closes the connection to the stub after a kill command.
            #We ignore it.
            pass
        finally:
            if self._verbose:
                print('GDB Stub: Connection closed')
                sys.stdout.flush()

            self._socket = None


    def __handle_packet(self, packet):
        i = 0
        while i < len(packet):
            if packet[i] == '+' or packet[i] == '-':
                i += 1

            elif packet[i] == '$':
                try:
                    j = packet.index('#', i)
                except ValueError:
                    break

                self._socket.send(b'+')

                self.__decode_command(packet[i+1:j])

                i = j + 3 #Skip the checksum

            elif ord(packet[i]) == 0x03:
                if self._verbose:
                    print('GDB >>> Stub : BREAK')
                    sys.stdout.flush()

                self.__handle_cmd_break()
                i += 1

            else:
                self._socket.send(b'-')
                i = len(packet)

        return i


    def __send_reply(self, reply):
        if self._verbose:
            print('GDB <<< Stub : ', reply)
            sys.stdout.flush()

        if isinstance(reply, str):
            reply = reply.encode('ascii')

        checksum = 0
        for b in reply:
            checksum = (checksum + b) % 256;

        packet = b'$%s#%02x' % (reply, checksum)
        self._socket.send(packet)


    def __start_simloop_join_thread(self):
        if not self._simloopjointhread:
            self._simloopjointhread = threading.Thread(target=self.__run_simloop_join_thread)
            self._simloopjointhread.start()


    def __run_simloop_join_thread(self):
        #Wait until the simloop stops
        while self._simloop.state() not in (AsyncSimLoop.State.Stopped,
                                            AsyncSimLoop.State.Done):
            time.sleep(0.001)

        if self._simloop.state() == AsyncSimLoop.State.Done:
            self.__send_reply('W01')
        else:
            self.__send_reply('S05')

        self._simloopjointhread = None


    def __decode_command(self, cmdline):
        if self._verbose:
            print('GDB >>> Stub : ', cmdline)
            sys.stdout.flush()

        cmd = cmdline[0]
        cmdargs = cmdline[1:]
        if cmd == 'q':
            self.__handle_cmd_query(cmdargs)
        elif cmd == '?':
            self.__handle_cmd_status()
        elif cmd == 'H':
            self.__handle_cmd_set_thread(cmdargs)
        elif cmd == 'g':
            self.__handle_cmd_read_all_regs()
        elif cmd == 'G':
            self.__handle_cmd_write_all_regs(cmdargs)
        elif cmd == 'p':
            self.__handle_cmd_read_reg(cmdargs)
        elif cmd == 'P':
            self.__handle_cmd_write_reg(cmdargs)
        elif cmd == 'm':
            self.__handle_cmd_read_data(cmdargs)
        elif cmd == 'M':
            self.__handle_cmd_write_data(cmdargs)
        elif cmd == 'c':
            self.__handle_cmd_continue()
        elif cmd == 's':
            self.__handle_cmd_step()
        elif cmd == 'k':
            self.__handle_cmd_kill()
        elif cmd == 'D':
            self.__handle_cmd_detach()
        elif cmd == 'Z':
            self.__handle_cmd_insert_breakpoints(cmdargs)
        elif cmd == 'z':
            self.__handle_cmd_remove_breakpoints(cmdargs)
        else:
            self.__send_reply('')


    def __handle_cmd_query(self, cmdargs):
        if cmdargs.startswith('Supported'):
            self.__send_reply("qXfer:memory-map:read+;qXfer:exec-file:read+")

        elif cmdargs == 'Attached':
            self.__send_reply('1')

        elif cmdargs.startswith('Xfer:memory-map:read'):
            core_cfg = self._device.config().core
            reply = mem_map_query_tpl.format(datalen = (core_cfg.dataend + 1),
                                             flashlen = (core_cfg.flashend + 1))
            self.__send_reply('l' + reply)

        elif cmdargs.startswith('Xfer:exec-file:read'):
            self.__send_reply('l' + self._source)

        elif cmdargs.startswith('RegisterInfo'):
            # Send back the information we have on this register (if any).
            n = int(cmdargs.split(':')[1])
            if n < 32:
                reply = rx_desc_tpl.format(name = 'r' + str(n), index = n)
            elif n == 32:
                reply = sreg_desc
            elif n == 33:
                reply = sp_desc
            elif n == 34:
                reply = pc_desc
            else:
                reply = ''

            self.__send_reply(reply)

        elif cmdargs.startswith('Rcmd'):
            rcmd_hexa = cmdargs.split(',')[1]
            rcmd_bytes = binascii.unhexlify(rcmd_hexa)
            rcmd = rcmd_bytes.decode('ascii')
            if rcmd == 'reset':
                with self._simloop:
                    self._probe.reset_device()
            elif rcmd == 'halt':
                with self._simloop:
                    self._probe.set_device_state(Device.State.Done)

            self.__send_reply('OK')

        elif cmdargs == 'fThreadInfo': #Querying the list of threads
            self.__send_reply('m0')
        elif cmdargs == 'sThreadInfo':
            self.__send_reply('l')
        elif cmdargs == 'C': #Querying the current thread. There is only one
            self.__send_reply('0')

        else:
            self.__send_reply('')


    def __handle_cmd_status(self):
        if self._simloop.state() == AsyncSimLoop.State.Done:
            self.__send_reply('X01')
        else:
            self.__send_reply('S05')


    def __handle_cmd_set_thread(self, cmdargs):
        if cmdargs in ('g0', 'c0', 'g-1', 'c-1'):
            self.__send_reply('OK')
        else:
            self.__send_reply('E01')


    def __handle_cmd_read_all_regs(self):
        with self._simloop:
            values = [self._probe.read_gpreg(n) for n in range(32)]
            values.extend([self._probe.read_sreg(),
                           self._probe.read_sp(),
                           self._probe.read_pc()])

        values = tuple(values)
        b = struct.pack('<33BHI', *values)
        hb = binascii.hexlify(b)
        self.__send_reply(hb)


    def __handle_cmd_write_all_regs(self, cmdargs):
        buf = binascii.unhexlify(cmdargs)
        values = struct.unpack('<33BHI', buf)

        with self._simloop:
            for n in range(32):
                self._probe.write_gpreg(n, values[n])

            self._probe.write_sreg(values[32])
            self._probe.write_sp(values[33])
            self._probe.write_pc(values[34])

        self.__send_reply('OK')


    def __handle_cmd_read_reg(self, cmdargs):
        num = _decode_hexa_int(cmdargs)
        with self._simloop:
            if num < 32:
                val = self._probe.read_gpreg(num)
                self.__send_reply(hex(val, 2))
            elif num == 32:
                val = self._probe.read_sreg()
                self.__send_reply(hex(val, 2))
            elif num == 33:
                val = self._probe.read_sp()
                self.__send_reply(hex(val, 4, 'big'))
            elif num == 34:
                val = self._probe.read_pc()
                self.__send_reply(hex(val, 8, 'big'))


    def __handle_cmd_write_reg(self, cmdargs):
        snum, sval = cmdargs.split('=')
        num = _decode_hexa_int(snum)
        val = _decode_hexa_int(sval)
        with self._simloop:
            if num < 32:
                self._probe.write_gpreg(num, val)
            elif num == 32:
                self._probe.write_sreg(val)
            elif num == 33:
                self._probe.write_sp(val)
            elif num == 34:
                self._probe.write_pc(val)

        self.__send_reply('OK')


    def __handle_cmd_read_data(self, cmdargs):
        saddr, slen = cmdargs.split(',')
        content_addr = int(saddr, 16) & 0xffffff
        content_len = int(slen, 16)

        #Check that the block given in argument respects
        #the flash/data boundaries and reads from the corresponding area
        core_cfg = self._device.config().core
        buf = None
        with self._simloop:
            if content_addr < 0x800000: #flash area
                if (content_addr + content_len - 1) <= core_cfg.flashend:
                    buf = self._probe.read_flash(content_addr, content_len)

            elif content_addr < 0x810000: #Data & I/O register area
                data_addr = content_addr - 0x800000
                if data_addr == core_cfg.ramend + 1 and content_len == 2:
                    #Allow GDB to read a value just after end of stack.
                    #This is necessary to make instruction stepping work when stack is empty
                    buf = b'\0\0\0\0'
                else:
                    buf = self._probe.read_data(data_addr, content_len)

            # else: #EEPROM area
            #   eeprom_addr = data_addr - 0x810000
            #   if (eeprom_addr + content_len) <= self._core_config.eepromend:
            #       buf = self._probe.read_eeprom(eeprom_addr, content_len)

        if buf:
            hexbuf = binascii.hexlify(buf)
            self.__send_reply(hexbuf)
        else:
            self.__send_reply('E01')


    def __handle_cmd_write_data(self, cmdargs):
        #Decode the command arguments
        s0, hexbuf = cmdargs.split(':')
        saddr, slen = s0.split(',')
        content_addr = int(saddr, 16) & 0xffffff
        content_len = int(slen, 16)
        buf = binascii.unhexlify(hexbuf)

        #Check that the block given in argument respects
        #the flash/data boundaries and writes to the corresponding area
        core_cfg = self._device.config().core
        ok = False
        with self._simloop:
            if content_addr < 0x800000: #flash area
                if (content_addr + content_len - 1) <= core_cfg.flashend:
                    self._probe.write_flash(content_addr, buf)
                    ok = True

            elif content_addr < 0x810000: #Data & I/O register area
                self._probe.write_data(content_addr - 0x800000, buf)
                ok = True

            # else : #EEPROM area
            #   eeprom_addr = data_addr - 0x810000
            #   if (eeprom_addr + content_len) <= core_cfg.eepromend:
            #       self._probe.write_eeprom(eeprom_addr, buf)
            #       ok = True

        self.__send_reply('OK' if ok else 'E01')


    def __handle_cmd_continue(self):
        with self._simloop:
            self._probe.set_device_state(Device.State.Running)
            self._simloop.loop_continue()

        self.__start_simloop_join_thread()


    def __handle_cmd_step(self):
        with self._simloop:
            self._probe.set_device_state(Device.State.Running)
            self._simloop.loop_step()

        self.__start_simloop_join_thread()


    def __handle_cmd_kill(self):
        with self._simloop:
            self._simloop.loop_kill()
        self.__send_reply('OK')


    def __handle_cmd_break(self):
        with self._simloop:
            self._simloop.loop_pause()


    def __handle_cmd_detach(self):
        self.__send_reply('OK')


    def __handle_cmd_insert_breakpoints(self, cmdargs):
        args = cmdargs.split(',')
        if args[0] == '0':
            addr = int(args[1], 16) & 0xffffff
            self._probe.insert_breakpoint(addr)
            self.__send_reply('OK')

        elif args[0] in ('2', '3', '4'):
            addr = int(args[1], 16) & 0xffffff
            size = int(args[2], 16)

            flags = DeviceDebugProbe.WatchpointFlags.Break
            if args[0] in ('2', '4'):
                flags |= DeviceDebugProbe.WatchpointFlags.Write
            if args[0] in ('3', '4'):
                flags |= DeviceDebugProbe.WatchpointFlags.Read

            if 0x800000 <= addr < 0x810000:
                self._probe.insert_watchpoint(addr - 0x800000, size, flags)
                self.__send_reply('OK')
            else:
                self.__send_reply('E01')

        else:
            self.__send_reply('')


    def __handle_cmd_remove_breakpoints(self, cmdargs):
        args = cmdargs.split(',')
        if args[0] == '0':
            addr = int(args[1], 16) & 0xffffff
            self._probe.remove_breakpoint(addr)
            self.__send_reply('OK')

        elif args[0] in ('2', '3', '4'):
            addr = int(args[1], 16) & 0xffffff

            flags = 0
            if args[0] in ('2', '4'):
                flags |= DeviceDebugProbe.WatchpointFlags.Write
            if args[0] in ('3', '4'):
                flags |= DeviceDebugProbe.WatchpointFlags.Read

            if 0x800000 <= addr < 0x810000:
                self._probe.remove_watchpoint(addr - 0x800000, flags)
                self.__send_reply('OK')
            else:
                self.__send_reply('E01')

        else:
            self.__send_reply('')


    @property
    def verbose(self):
        return self._verbose

    def set_verbose(self, v):
        self._verbose = bool(v)

