# simrun.py
#
# Copyright 2023 Clement Savergne <csavergne@yahoo.com>
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

import argparse
import threading

from ..lib import core as _corelib
from ..device_library import load_device, model_list
from ..utils.vcd_recorder import Formatter, VCD_Recorder

__all__ = ['main', 'clean']


def _create_argparser():
    """ Create the parser for the command line.
    """

    p = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description="Runs a simulation",
        epilog="""Trace arguments:
    port X[/VARNAME] : 8-bits GPIO port tracing
        X identifies the GPIO port (ex: 'A', 'B')
    pin PIN[/VARNAME] : Pin digital value tracing
        PIN identifies the pin (ex: 'PA0')
    data ADDR[/VARNAME] : Memory tracing, currently only works for SRAM
        ADDR is the hexadecimal address in data space
    vector N[/VARNAME] : Interrupt state tracing
        N is the vector index
    signal CTL[/SIZE][/ID][/IX][/VARNAME] : Generic peripheral signal tracing
        CTL identifies the peripheral
        SIZE: optional, variable size in bits (default is 32)
        ID : optional, sigid to filter (default is none)
        IX : optional, index to filter (default is none)
    for all trace types, VARNAME is an optional variable name""")

    p.add_argument('-f', '--frequency',
                   type=int, metavar='FREQ',
                   help="Set the clock frequency in Hertz for the MCU")

    p.add_argument('-m', '--mcu',
                   metavar='MCU',
                   help="Set the MCU model")

    p.add_argument('--list-cores',
                   action='store_true',
                   help="List all supported MCU models and exit")

    p.add_argument('-c', '--cycles',
                   metavar='CYCLES', type=int, default=0,
                   help="Limit the simulated time to <CYCLES>")

    p.add_argument('-g', '--gdb',
                   metavar='PORT', nargs = '?', default=None, const=1234, type=int,
                   help="Enable GDB mode. Listen for GDB connection on <PORT> (default 1234)")

    p.add_argument('-v', '--verbose',
                   metavar='LEVEL', action='store', default=0, type=int,
                   help='Set the verbosity level (0-4)')

    p.add_argument('-a', '--analog',
                   metavar='VCC', nargs='?', default=None, const=5.0, type=float,
                   help="Enable analog features with <VCC> as main supply voltage (default 5.0 Volts)")

    p.add_argument('-r', '--reference',
                   metavar='REF', type=float,
                   help="Analog voltage reference, relative to VCC (default is 1, i.e. AREF=VCC)")

    p.add_argument('-d', '--dump',
                   metavar='PATH',
                   help="Dump the final simulation state into a text file")

    p.add_argument('-o', '--output',
                   metavar='PATH',
                   help="Specify the VCD file to save the traced variables")

    p.add_argument('-t', '--trace',
                   metavar=('TYPE', 'ARGS'), action='append', dest='traces', nargs=2,
                   help="Add a variable to trace. TYPE can be port, pin, data, vector or signal")

    p.add_argument('firmware',
                   nargs='?',
                   help='ELF file containing the firmware to execute')

    return p


_run_args = None
_device = None
_firmware = None
_simloop = None
_vcd_out = None
_probe = None


class _WatchDataTrace(Formatter):

    def __init__(self, addr):
        super().__init__('reg', 8, 0)

        self._addr = addr

        f = _corelib.AVR_DeviceDebugProbe.WatchpointFlags
        _probe.insert_watchpoint(addr, 1, f.Write | f.Signal)
        _probe.watchpoint_signal().connect_hook(self)

    def filter(self, sigdata, hooktag):
        return (sigdata.sigid == _corelib.AVR_DeviceDebugProbe.WatchpointFlags.Write and \
                sigdata.index == self._addr)

    def format(self, sigdata, hooktag):
        return sigdata.data.as_uint()


def _print_list_cores():
    print(model_list())


def _load_firmware():
    global _firmware

    _firmware = _corelib.AVR_Firmware.read_elf(_run_args.firmware)
    if not _firmware:
        raise Exception('Reading the firmware failed')

    _firmware.frequency = _run_args.frequency
    _firmware.variant = _run_args.mcu
    if _run_args.analog is not None:
        _firmware.vcc = _run_args.analog
        if _run_args.reference is not None:
            _firmware.aref = _run_args.reference
        else:
            _firmware.aref = 1.

    _device.load_firmware(_firmware)


def _init_VCD():
    global _vcd_out
    _vcd_out = VCD_Recorder(_simloop, _run_args.output)

    for kind, s_params in _run_args.traces:
        params = s_params.split('/')

        if kind == 'port':
            port_id = params[0]
            port_name = '' if len(params) < 2 else params[1]
            _vcd_out.add_gpio_port(port_id, port_name)

        elif kind == 'pin':
            pin_id = params[0]
            pin_name = '' if len(params) < 2 else params[1]
            pin = _device.find_pin(pin_id)
            _vcd_out.add_digital_pin(pin, pin_name)

        elif kind == 'data':
            global _probe
            if _probe is None:
                _probe = _corelib.AVR_DeviceDebugProbe(_device)

            addr = int(params[0], 16)
            name = hex(addr) if len(params[0]) < 2 else params[1]
            tr = _WatchDataTrace(addr)
            _vcd_out.add(tr, name)

        elif kind == 'vector':
            ix = int(params[0])
            name = '' if len(params) < 2 else params[1]
            _vcd_out.add_interrupt(ix, name)

        elif kind == 'signal':
            ctlid = params[0]
            sigid = int(params[1]) if len(params) >= 2 else None
            sigindex = int(params[2]) if len(params) >= 3 else None
            size = int(params[3]) if len(params) >= 4 else 32
            name = 'sig_' + ctlid
            ok, d = _device.ctlreq(_corelib.str_to_id(ctlid), _corelib.CTLREQ_GET_SIGNAL)
            if not ok:
                raise Exception('Unable to obtain the peripheral signal')
            sig = d.data.as_ptr(_corelib.AVR_Signal)
            _vcd_out.add_signal(sig, name, size, sigid, sigindex)


def _run_syncloop():
    global _simloop

    _simloop = _corelib.AVR_SimLoop(_device)
    _simloop.set_fast_mode(True)

    _load_firmware()

    #If an output file is set
    if _run_args.output:
        _init_VCD()
        _vcd_out.record_on()

    _simloop.run(_run_args.cycles)


def _run_asyncloop(args):
    from .utils.gdb_server import GDB_Stub

    global _simloop

    _simloop = _corelib.AVR_AsyncSimLoop(_device)
    _simloop.set_fast_mode(True)

    _load_firmware()

    gdb = GDB_Stub(conn_point=('127.0.0.1', args.gdb),
                   fw_source=args.firmware,
                   simloop=_simloop)

    if args.verbose:
        gdb.set_verbose(True)

    gdb.start()

    th = threading.Thread(target=_simloop.run)
    th.start()

    th.join()


def main(args=None):
    global _run_args, _device

    parser = _create_argparser()
    _run_args = parser.parse_args(args=args)

    if _run_args.list_cores:
        _print_list_cores()
        return

    if _run_args.firmware is None:
        raise argparse.ArgumentError(None, 'No firmware provided')

    if _run_args.frequency is None:
        raise argparse.ArgumentError(None, 'No frequency provided')

    if _run_args.mcu is None:
        raise argparse.ArgumentError(None, 'No MCU model provided')

    _device = load_device(_run_args.mcu, _run_args.verbose > 1)

    #Set the verbose level
    log_level = _corelib.AVR_Logger.Level.Silent + _run_args.verbose
    if log_level > _corelib.AVR_Logger.Level.Trace:
        _corelib.AVR_Logger.Level.Trace
    _corelib.AVR_global_logger().set_level(log_level)
    _device.logger().set_level(log_level)

    try:
        if _run_args.gdb is None:
            _run_syncloop()
        else:
            _run_asyncloop()
    except KeyboardInterrupt:
        print('Simulation interrupted !!')

    if _vcd_out:
        _vcd_out.flush()

    if _run_args.dump is not None:
        from ..utils.sim_dump import sim_dump
        sim_dump(_simloop, open(_run_args.dump, 'w'))


def clean():
    global _device, _firmware, _probe, _simloop, _vcd_out

    if _vcd_out:
        _vcd_out.close()

    _vcd_out = None
    _probe = None
    _simloop = None
    _device = None
    _firmware = None


if __name__ == '__main__':
    try:
        main()
    finally:
        clean()
