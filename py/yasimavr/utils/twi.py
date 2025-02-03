# twi.py
#
# Copyright 2022-2025 Clement Savergne <csavergne@yahoo.com>
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

'''
This module defines TWISimpleClient which is a simple reimplementation of
a TWI client device that can be used for simple TWI/I2C part simulation.
'''


import yasimavr.lib.core as _corelib
from yasimavr.lib.core import TWI

__all__ = ['TWI', 'TWISimpleClient']


class TWISimpleClient(TWI.Client):
    '''Simple TWI client used to emulate peripheral IC such as eeproms
    '''

    def __init__(self, address):
        super().__init__()

        self._address = address & 0x7F

        self._hook = _corelib.CallableSignalHook(self.state_signal_raised)
        self.signal().connect(self._hook)

        self._scl_wire = _corelib.Wire()
        self._scl_hook = _corelib.CallableSignalHook(self._line_signal_raised)
        self._scl_wire.signal().connect(self._scl_hook, TWI.Line.Clock)

        self._sda_wire = _corelib.Wire()
        self._sda_hook = _corelib.CallableSignalHook(self._line_signal_raised)
        self._sda_wire.signal().connect(self._sda_hook, TWI.Line.Data)


    def _line_signal_raised(self, sigdata, hooktag):
        if sigdata.sigid == _corelib.Wire.SignalId.DigitalChange:
            value = sigdata.data.value()
            line = TWI.Line(hooktag)
            self.line_state_changed(line, value)


    #Client override
    def set_line_state(self, line, state):
        if line == TWI.Line.Clock:
            self._scl_wire.set_state('U' if state else 'L')
        else:
            self._sda_wire.set_state('U' if state else 'L')


    @property
    def scl_wire(self):
        return self._scl_wire


    @property
    def sda_wire(self):
        return self._sda_wire


    def write_handler(self, data):
        '''Generic handler for a write request
        Should be reimplemented to process the provided data
        and return True for ACK or False for NACK.
        The handler is called once for each byte being written
        by the host.'''
        return False


    def read_handler(self):
        '''Generic handler for a read request
        Should be reimplemented to provide the data being read
        The handler is called only once for each byte of a read request.
        It should return a 8-bits integer, which will be sent over
        to the host.'''
        return 0


    @property
    def address(self):
        '''Default address on the bus'''
        return self._address


    @address.setter
    def set_address(self, address):
        self._address = int(address)


    def address_match(self, addr_rw):
        '''Test the received address for match.
        The default implementation returns True for the default address.
        It may be overriden to define different match requirements.
        \param addr_rw : ADDR+RW byte as sent by the host
        \return True to respond with ACK, False for NACK
        '''
        return (addr_rw >> 1) == self._address


    def transfer_start(self, rw):
        '''Generic handler called once at the start of a packet transfer.
        \param rw RW bit: 1 for Read (client->host), 0 for Write (host->client)
        '''
        pass


    def packet_end(self):
        '''Generic handler called once at the end of a packet, i.e. after
        data has been transferred and responded with a NACK.
        '''
        pass


    def transfer_stop(self, ok);
        '''Generic handler called once at the end of a transfer, i.e. when
        detecting a STOP or a RESTART condition (ok is True), or when the
        transfer is interrupted by a bus collision. (ok is False)
        '''
        pass


    def state_signal_raised(self, sigdata, _):
        sid = TWI.SignalId(sigdata.sigid)
        if sid == TWI.SignalId.AddressReceived:
            addr_rw = sigdata.data.value()
            ack = self.address_match(addr_rw)
            self.set_ack(ack)
            if ack:
                self.transfer_start(addr_rw & 0x01)

        elif sid == TWI.SignalId.DataStandby:
            if self.rw():
                data = self.read_handler()
                self.start_data_tx(data)
            elif sigdata.data.as_uint() or self.ack():
                self.start_data_rx()

        elif sid == TWI.SignalId.DataReceived:
            data = sigdata.data.value()
            ack = self.write_handler(data)
            self.set_ack(ack)
            if not ack:
                self.packet_end()

        elif sid == TWI.SignalId.DataAckReceived:
            if not sigdata.data.value():
                self.packet_end()

        elif sid == TWI.SignalId.Stop or sid == TWI.SignalId.Start:
            #If the client was active before the START/STOP condition,
            #the previous transfer needs to be closed first.
            if sigdata.data.value():
                self.transfer_stop(True)

        elif sid == TWI.SignalId.BusCollision:
            self.transfer_stop(False)
