# twi.py
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

'''
This module defines TWI_Slave which is a simple reimplementation of
a TWI Endpoint that can be used for simple TWI/I2C part simulation.
'''


import yasimavr.lib.core as _corelib


class TWI_Slave(_corelib.TWIEndPoint):

    def __init__(self, address):
        super().__init__()
        self._address = address & 0x7F
        self._active = False
        self._rw = False


    #Generic handler for a write request
    #Should be reimplemented to process the provided data
    #and return True for ACK or False for NACK
    #The handler is called once for each byte being written
    #by the master
    def write_handler(self, data):
        return False


    #Generic handler for a read request
    #Should be reimplemented to provide the data being read
    #The handler is called only once for each byte of a read request.
    #It should return a 8-bits integer, which will be sent over
    #to the master.
    def read_handler(self):
        return 0


    #Generic handler for an address and read/write match
    #May be reimplemented for more complex address match behaviours
    #Return True for ACK or False for NACK
    def match_address(self, address, rw):
        return address == self._address


    @property
    def address(self):
        '''Default address on the bus'''
        return self._address

    @address.setter
    def set_address(self, address):
        if not self._active:
            self._address = address


    @property
    def active(self):
        '''Boolean indicating if the slave is actively addressed on the bus'''
        return self._active


    @property
    def rw(self):
        '''Boolean indicating the type of operation (True=read, False=write)
        The value is only relevant when active == True'''
        return (self._rw == _corelib.TWIPacket.Read)


    #TWIEndPoint override
    def packet(self, packet):
        pass


    #TWIEndPoint override
    def packet_ended(self, packet):
        if packet.cmd == _corelib.TWIPacket.Cmd.Address:
            try:
                self._active = self.match_address(packet.addr, packet.rw)
            except Exception:
                self._active = False

            if self._active:
                packet.ack = _corelib.TWIPacket.Ack
                self._rw = packet.rw
            else:
                packet.ack = _corelib.TWIPacket.Nack

            packet.hold = 0

        elif packet.cmd == _corelib.TWIPacket.Cmd.DataRequest and self._active:
            if self._rw == _corelib.TWIPacket.Read:
                try:
                    packet.data = self.read_handler()
                except Exception:
                    packet.data = 0xFF
            else:
                try:
                    ack = self.write_handler(packet.data)
                except Exception:
                    ack = False

                packet.ack = _corelib.TWIPacket.Ack if ack else _corelib.TWIPacket.Nack

            packet.hold = 0


    #TWIEndPoint override
    def bus_acquired(self):
        pass


    #TWIEndPoint override
    def bus_released(self):
        self._active = False
