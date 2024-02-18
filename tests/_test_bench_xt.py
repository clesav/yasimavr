# _test_bench_xt.py
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

import os
import yasimavr.lib.core as corelib
import yasimavr.lib.arch_xt as archlib
from yasimavr.device_library import load_device, DeviceAccessor
from yasimavr.device_library.builders._builders_arch_xt import XT_BaseDevice


TESTFW_M4809 = os.path.join(os.path.dirname(__file__), 'fw', 'testfw_atmega4809.elf')


class BenchXT:

    def __init__(self, dev_model, path_fw):
        if isinstance(dev_model, str):
            self.dev_model = load_device(dev_model)
        else:
            self.dev_model = dev_model

        self.dev_model.logger().set_level(corelib.Logger.Level.Trace)
        self.dev_model.set_option(corelib.Device.Option.InfiniteLoopDetect, False)

        self.loop = corelib.SimLoop(self.dev_model)
        self.loop.set_fast_mode(True)

        self.fw = corelib.Firmware.read_elf(path_fw)
        self.fw.frequency = 1000
        self.fw.vcc = 5.0
        self.fw.aref = 5.0
        self.dev_model.load_firmware(self.fw)

        self.probe = corelib.DeviceDebugProbe(self.dev_model)

        self.dev = DeviceAccessor(self.probe)

        #execute enough cycles to ensure we're in the main loop
        self.sim_advance(1000)


    def sim_advance(self, nbcycles, expect_end=False):
        final_cycle = self.loop.cycle() + nbcycles
        while self.loop.cycle() < final_cycle:
            self.loop.run(final_cycle - self.loop.cycle())
            if self.loop.state() != self.loop.State.Running:
                if self.loop.state() == self.loop.State.Done and not expect_end:
                    raise Exception('Device stopped unexpectedly')
                break


def bench_m4809():
    return BenchXT('atmega4809', PATH_TEST_FW_M4809)
