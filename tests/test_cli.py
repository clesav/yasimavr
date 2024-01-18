# test_cli.py
#
# Copyright 2024 Clement Savergne <csavergne@yahoo.com>
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

"""
Testing of the Command Line mode.
This is mainly thought as smoke tests to ensure the simulation starts and run
without errors in both standalone and GDB mode.
"""


from yasimavr.cli.simrun import main as cli_main
import os
import threading
import socket
import time


fw_path = os.path.join(os.path.dirname(__file__), 'fw', 'testfw_atmega328.elf')


def test_cli_standalone():
    cli_args = ['-f', '1000',
                '-m', 'atmega328',
                '-d', 'dump_cli.txt',
                '-c', '1000000',
                fw_path]
    cli_main(cli_args)

    assert os.path.isfile('dump_cli.txt')


def _make_packet(s):
    checksum = 0
    for b in bytes(s):
        checksum = (checksum + b) % 256

    packet = b'$%s#%02x' % (s, checksum)
    return packet

_TEST_PACKETS = [
    _make_packet(b'c'), #Continue command
    _make_packet(b'k'), #Kill command
]


def test_cli_gdb():
    cli_args = ['-f', '1000',
                '-m', 'atmega328',
                '-d', 'dump_cli_gdb.txt',
                '-g', '1234',
                '-v', '4',
                fw_path]

    result = None
    def cli_main_wrapper():
        nonlocal result
        try:
            cli_main(cli_args)
        except Exception:
            result = False
        else:
            result = True

    #Start the simulation on a different thread
    th_main = threading.Thread(target=cli_main_wrapper, daemon=True)
    th_main.start()
    time.sleep(1)

    #Connect a client socket to the stub, and send some GDB packets
    sck = socket.socket()
    sck.connect(('localhost', 1234))
    for p in _TEST_PACKETS:
        sck.send(p)
        time.sleep(1)
    sck.close()

    #Check that the simulation exited
    th_main.join(10)

    assert not th_main.is_alive()
    assert result
    assert os.path.isfile('dump_cli_gdb.txt')
