# sim_dump.py
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

import io

from ..lib import core as corelib
from ..device_library.accessors import AVR_DeviceAccessor

__all__ = ['simdump']


class _Dumper:

    def __init__(self, stream):
        self.stream = stream
        self.indent = 0
        self._empty_line = False


    def inc_level(self, tag):
        self.stream.write((' ' * (self.indent * 4)) + tag + ':\n')
        self._empty_line = False
        self.indent += 1


    def dec_level(self):
        if not self._empty_line:
            self.stream.write('\n')
        self._empty_line = True
        self.indent = max(0, self.indent - 1)


    def dump(self, tag, value):
        self.stream.write((' ' * (self.indent * 4)) + tag + ': ' + str(value) + '\n')
        self._empty_line = False

    __setitem__ = dump


    def dump_bytes(self, tag, value):
        indent = ' ' * (self.indent * 4)
        self.stream.write(indent + tag + ':\n')
        indent += ' ' * 4
        for i in range(0, len(value), 16):
            sv = ' '.join('%02x' % v for v in value[i:(i+16)])
            self.stream.write(indent + sv + '\n')

        self._empty_line = False


    def dump_memory(self, tag, data, addrstart):
        indent = ' ' * (self.indent * 4)
        self.stream.write(indent + tag + ':\n')
        indent += ' ' * 4
        for i in range(0, len(data), 16):
            sv = ' '.join('%02x' % v for v in data[i:(i+16)])
            self.stream.write('%s%04x : %s\n' % (indent, addrstart + i, sv))

        self.stream.write('\n')
        self._empty_line = True


    def dump_nvm(self, nvm):
        indent = ' ' * (self.indent * 4)
        self.stream.write(indent + nvm.name() + ':\n')
        indent += ' ' * 4
        for i in range(0, nvm.size(), 16):
            line_values = ( ('%02x' % nvm[j]) if nvm.programmed(j) else '--'
                            for j in range(i, i + 16) )

            sv = ' '.join(line_values)
            self.stream.write('%s%04x : %s\n' % (indent, i, sv))

        self.stream.write('\n')
        self._empty_line = True


def _serialize_registers(probe, dumper):
    dumper.inc_level('I/O Registers')

    accessor = AVR_DeviceAccessor(probe)

    for per_name, per_descriptor in accessor.descriptor.peripherals.items():
        per_class_descriptor = per_descriptor.class_descriptor
        per_accessor = getattr(accessor, per_name)

        reg_names = list(per_class_descriptor.registers)
        for reg_name in list(reg_names):
            if (reg_name + 'L') in reg_names:
                reg_names.remove(reg_name + 'L')
                reg_names.remove(reg_name + 'H')

        if not reg_names: continue

        dumper.inc_level(per_name)

        for reg_name in reg_names:
            v = getattr(per_accessor, reg_name).read()
            if isinstance(v, int):
                dumper[reg_name] = hex(v)
            elif isinstance(v, bytes):
                dumper.dump_bytes(reg_name, v)
            else:
                dumper[reg_name] = repr(v)

        dumper.dec_level()

    dumper.dec_level()


def _serialize_RAM(device, probe, dumper):
    ramstart = device.config().core.ramstart
    ramend = device.config().core.ramend
    ram_data = probe.read_data(ramstart, ramend - ramstart + 1)
    dumper.dump_memory('RAM', ram_data, ramstart)


def _serialize_NVM(device, dumper):
    reqdata = corelib.ctlreq_data_t()
    reqdata.index = corelib.AVR_Core.NVM.GetCount
    device.ctlreq(corelib.IOCTL_CORE, corelib.CTLREQ_CORE_NVM, reqdata)
    nvm_count = reqdata.data.as_uint()

    for i in range(nvm_count):
        reqdata = corelib.ctlreq_data_t()
        reqdata.index = i
        device.ctlreq(corelib.IOCTL_CORE, corelib.CTLREQ_CORE_NVM, reqdata)
        nvm = reqdata.data.as_ptr(corelib.AVR_NonVolatileMemory)
        if nvm:
            dumper.dump_nvm(nvm)


def _serialize_CPU(probe, dumper):
    dumper.inc_level('Core')
    dumper['Program Counter'] = hex(probe.read_pc())
    dumper['Stack Pointer'] = hex(probe.read_sp())
    dumper['SREG'] = hex(probe.read_sreg())

    reg_values = [probe.read_gpreg(i) for i in range(32)]

    for i in range(32):
        dumper['R%d' % i] = hex(reg_values[i])

    dumper['X'] = hex(reg_values[27] * 256 + reg_values[26])
    dumper['Y'] = hex(reg_values[29] * 256 + reg_values[28])
    dumper['Z'] = hex(reg_values[31] * 256 + reg_values[30])

    dumper.dec_level()


def _serialize_pins(device, dumper):
    dumper.inc_level('Pins')

    for pin_name in device.config().pins:
        pin = device.find_pin(pin_name)
        if pin.state() == corelib.AVR_Pin.State.Analog:
            s = 'Analog V=' + str(pin.analog_value())
        else:
            s = pin.state()._name_
        dumper[pin_name] = s

    dumper.dec_level()


def _serialize_device(device, dumper):
    dumper.inc_level('Device')

    dumper['Variant'] = device.config().name
    dumper['State'] = device.state()._name_
    dumper['Sleep mode'] = device.sleep_mode()._name_
    dumper['Frequency (Hz)'] = device.frequency()

    dumper.inc_level('Options')
    for dev_option in corelib.AVR_Device.Option:
        dumper[dev_option._name_] = device.test_option(dev_option)
    dumper.dec_level()

    dumper.dec_level()

    _serialize_pins(device, dumper)

    probe = corelib.AVR_DeviceDebugProbe()
    probe.attach(device)

    _serialize_CPU(probe, dumper)
    _serialize_registers(probe, dumper)
    _serialize_RAM(device, probe, dumper)
    _serialize_NVM(device, dumper)


def _serialize_simloop(simloop, dumper):
    dumper.inc_level('Simloop')
    dumper['State'] = simloop.state()._name_
    dumper['Simulated cycles'] = simloop.cycle()

    if simloop.device().state()._name_ not in ('Limbo', 'Ready'):
        t = simloop.cycle() / simloop.device().frequency()
    else:
        t = 0
    dumper['Simulated time (secs)'] = t

    dumper.dec_level()

    _serialize_device(simloop.device(), dumper)


def sim_dump(simloop : corelib.AVR_AbstractSimLoop, stream : io.TextIOBase = None) -> str | None:
    '''
    usage: sim_dump(simloop, stream = None) -> str | None

    Dumps the current state of a simulation into a I/O stream.

    simloop : instance of AVR_AbstractSimLoop to dump
    stream : writable TextIOBase instance or None
    If stream is None, the state is dumped into a string and the string is returned.
    If stream is not None, the state is written into it and None is returned.
    '''

    dumper = _Dumper(io.StringIO() if stream is None else stream)

    _serialize_simloop(simloop, dumper)

    if stream is None:
        return dumper.stream.getvalue()
    else:
        return None
