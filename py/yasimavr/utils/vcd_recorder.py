# vcd_recorder.py
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

import vcd
from yasimavr.lib import core as _corelib

__all__ = ['Formatter', 'VCD_Recorder']


class Formatter(_corelib.SignalHook):
    """Generic formatter class.

    A formatter is a signal hook connected to receive data changes and
    format it for writing into a VCD file.

    :param VarType var_type: VCD variable type (see pyvcd documentation)
    :param int var_size: VCD variable size (see pyvcd documentation)
    :param str init_value: initial value
    """

    def __init__(self, var_type, var_size, init_value):
        super().__init__()
        self._recorder = None
        self._var_type = var_type
        self._var_size = var_size
        self._init_value = init_value


    def register(self, recorder, var_name):
        """Called by the recorder to register this Formatter instance
        with a associated VCD Variable object.

        :param VCD_Recorder recorder: VCD_Recorder instance
        :param str var_name: VCD variable name (see pyvcd documentation)
        """

        self._recorder = recorder
        self._var = recorder._writer.register_var(scope='device',
                                                  name=var_name,
                                                  var_type=self._var_type,
                                                  size=self._var_size,
                                                  init=self._init_value)


    def filter(self, sigdata, hooktag):
        """Generic filtering facility to be reimplemented by subclasses.

        Must return True if the value shall be recorded, based on the
        signal data fields. By default all values are recorded.
        """
        return True


    def format(self, sigdata, hooktag):
        """Generic conversion facility to be reimplemented by subclasses.

        Must return a value compatible with the variable type, to be recorded
        in the VCD file.
        """
        return None


    def raised(self, sigdata, hooktag):
        """Reimplementation of SignalHook, to filter, format and record
        the value associated to the signal.
        """

        try:
            if self.filter(sigdata, hooktag):
                fmt_value = self.format(sigdata, hooktag)
                self._recorder._change(self._var, fmt_value)
        except Exception:
            import traceback
            traceback.print_exc()


class _PinDigitalFormatter(Formatter):
    """Formatter specialised for the digital state of GPIO pins.
    """

    _SIGID = _corelib.Pin.SignalId.DigitalChange
    _PinState = _corelib.Pin.State

    def __init__(self, pin):
        super().__init__('tri', 1, None)
        pin.signal().connect(self)

    def filter(self, sigdata, hooktag):
        return (sigdata.sigid == self._SIGID)

    def format(self, sigdata, hooktag):
        pin_state = self._PinState(sigdata.data.as_uint())
        if pin_state == self._PinState.Floating:
            return None
        elif pin_state in (self._PinState.High, self._PinState.PullUp):
            return True
        elif pin_state in (self._PinState.Low, self._PinState.PullDown):
            return False
        else: #Analog and Shorted
            return 'X'


class _PortFormatter(Formatter):
    """Formatter specialised for a GPIO port as an 8-bits vectors.
    """

    def __init__(self, port_signal):
        super().__init__('reg', 8, None)
        port_signal.connect(self)

    def format(self, sigdata, hooktag):
        return sigdata.data.as_uint()


class _SignalFormatter(Formatter):

    def __init__(self, sig, size, sigid, index):
        vartype = 'tri' if size == 1 else 'reg'
        super().__init__(vartype, size, 'x')
        self._sigid = sigid
        self._index = index
        sig.connect(self)

    def filter(self, sigdata, hooktag):
        return ((self._sigid is None or sigdata.sigid == self._sigid) and \
                (self._index is None or sigdata.index == self._index))

    def format(self, sigdata, hooktag):
        return sigdata.data.as_uint()


class _InterruptFormatter(Formatter):

    def __init__(self, vector):
        super().__init__('tri', 1, False)
        self._vector = int(vector)

    def filter(self, sigdata, hooktag):
        return (sigdata.index == self._vector)

    def format(self, sigdata, hooktag):
        vect_state = sigdata.data.as_uint()
        return vect_state & 0x01


class VCD_Recorder:
    """Value Change Dump recorder.

    A VCD file captures time-ordered changes to the value of variables as raised by
    signals.

    It is built on top of a VCDWriter instance from *PyVCD* and uses Formatter objects to
    connect to signals from the simulation model, filter and format the values received from
    signal notifications and  writes them into the VCD file.

    :param AbstractSimLoop simloop: The simulation loop instance.
    :param str file: file path to write the VCD data.
    :param dict kwargs: other arguments passed on to the underlying VCDWriter object.

    See *PyVCD* docs for details on VCDWriter and the VCD file format:
    `PyVCD documentation <https://pyvcd.readthedocs.io/>`_
    """

    def __init__(self, simloop, file, **kwargs):
        self._simloop = simloop
        self._ts_ratio = 100000000 / self._simloop.device().frequency()

        self._file = open(file, 'w')

        kwargs['timescale'] = '10ns'
        kwargs['init_timestamp'] = 0
        self._writer = vcd.VCDWriter(self._file, **kwargs)

        self._formatters = {}


    @property
    def writer(self):
        """Return the underlying VCDWriter object.
        """
        return self._writer


    def add_digital_pin(self, pin, var_name=''):
        """Register a new VCD variable for a Pin instance.

        :param Pin pin: Pin object
        :param str var_name: optional variable name, defaults to the pin identifier
        """

        if not var_name:
            var_name = _corelib.id_to_str(pin.id())

        formatter = _PinDigitalFormatter(pin)
        formatter.register(self, var_name)
        self._formatters[var_name] = formatter


    def add_gpio_port(self, port_name, var_name=''):
        """Register a new VCD variable for a GPIO port.

        :param str port_name: Letter identifying the GPIO port
        :param str var_name: optional variable name, defaults to the port identifier
        """

        if not var_name:
            var_name = 'GPIO_P' + port_name

        port_id = _corelib.IOCTL_PORT(port_name)
        ok, req = self._simloop.device().ctlreq(port_id, _corelib.CTLREQ_GET_SIGNAL)
        if ok:
            sig = req.data.as_ptr(_corelib.Signal)
            formatter = _PortFormatter(sig)
            formatter.register(self, var_name)
            self._formatters[var_name] = formatter
        else:
            raise Exception('Issue with access to the port signal')


    def add_interrupt(self, vector, var_name=''):
        """Register a new VCD variable for an interrupt vector.

        :param int vector: interrupt vector index
        :param str var_name: optional variable name, defaults to the vector index
        """

        if not var_name:
            var_name = 'vect_' + str(vector)

        ok, d = self._simloop.device().ctlreq(_corelib.IOCTL_INTR, _corelib.CTLREQ_GET_SIGNAL)
        if ok:
            sig = d.data.as_ptr(_corelib.Signal)
            formatter = _InterruptFormatter(vector)
            sig.connect(formatter)
            formatter.register(self, var_name)
            self._formatters[var_name] = formatter
        else:
            raise Exception('Unable to obtain the interrupt signal')


    def add_signal(self, sig, var_name, size=32, sigid=None, sigindex=None):
        """Register a new VCD variable for a generic peripheral signal.

        :param Signal sig: Signal to connect to
        :param str var_name: Variable name
        :param int size: variable size, default is 32 bits
        :param int sigid: optional SignalId value for filtering
        :param int sigindex: optional index value for filtering
        """

        formatter = _SignalFormatter(sig, size, sigid, sigindex)
        formatter.register(self, var_name)
        self._formatters[var_name] = formatter


    def add(self, formatter, var_name):
        """Register a new VCD variable for a generic formatter.

        :param Formatter formatter: Formatter object
        :param str var_name: variable name
        """

        formatter.register(self, var_name)
        self._formatters[var_name] = formatter


    def _change(self, var, value):
        ts = self._ts_ratio * self._simloop.cycle()
        self._writer.change(var, ts, value)


    def record_on(self):
        """Start or resume the recording.
        """

        ts = self._ts_ratio * self._simloop.cycle()
        self._writer.dump_on(ts)


    def record_off(self):
        """Pause the recording.
        """

        ts = self._ts_ratio * self._simloop.cycle()
        self._writer.dump_off(ts)


    def flush(self):
        """Flush the recorded data into the destination file.
        """

        ts = self._ts_ratio * self._simloop.cycle()
        self._writer.flush(ts)


    def close(self):
        """Close the record. The recorder may not be used anymore afterwards.
        """

        ts = self._ts_ratio * self._simloop.cycle()
        self._writer.close(ts)
        self._file.close()
        self._formatters.clear()
