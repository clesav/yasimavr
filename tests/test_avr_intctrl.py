# test_avr_intctrl.py
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

import pytest
import yasimavr.lib.core as corelib
import yasimavr.lib.arch_avr as archlib
from _test_bench_avr import bench_m328
from _test_utils import DictSignalHook, QueuedSignalHook


@pytest.fixture
def bench():
    bench = bench_m328()

    _, reqdata = bench.dev_model.ctlreq(corelib.IOCTL_INTR, corelib.CTLREQ_GET_SIGNAL)
    bench.irq_sig = reqdata.data.value(corelib.Signal)
    bench.irq_hook = DictSignalHook(bench.irq_sig)

    return bench


IntrState = corelib.InterruptController.State
IntrStateChange = corelib.InterruptController.SignalId.StateChange


def _set_irq_state(bench, vector, state):
    '''Utility function that raises or cancels an interrupt manually
    '''
    reqdata = corelib.ctlreq_data_t()
    reqdata.index = vector
    reqdata.data = corelib.vardata_t(1 if state else 0)
    ok, _ = bench.dev_model.ctlreq(corelib.IOCTL_INTR, corelib.CTLREQ_INTR_RAISE, reqdata)
    if not ok: raise Exception('set_irq_state() failed')


def _extract_acks(qhook):
    return [ sigdata.index for sigdata, _ in qhook.queue() if sigdata.data == IntrState.Acknowledged ]


def _advance_until_reti(bench, limit=1000):
    '''Utility function that advances the simulation loop until the CPU executes a RETI instruction
    '''
    has_reti = False

    def hook_reti(sigdata, _):
        nonlocal has_reti
        if sigdata.data.as_uint() == IntrState.Returned: has_reti = True

    hook = corelib.CallableSignalHook(hook_reti)
    bench.irq_sig.connect(hook)

    for _ in range(limit):
        bench.sim_advance(1)
        if has_reti: break

    if not has_reti: raise Exception()


def test_avr_intr_signal(bench):
    '''Check the signalling of the successive states of an IRQ
    '''

    #Raise the IRQ 3 and check the signal
    _set_irq_state(bench, 3, True)
    assert bench.irq_hook.pop_data(IntrStateChange, 3) == IntrState.Raised

    #Check that the state is updated when the CPU acknowledges the interrupt
    bench.sim_advance(1)
    assert bench.irq_hook.pop_data(IntrStateChange, 3) == IntrState.Acknowledged

    #Check that the state is updated when the CPU completes the ISR
    bench.sim_advance(100)
    assert bench.irq_hook.pop_data(IntrStateChange) == IntrState.Returned
    assert bench.dev.CPU.GPIOR0 == 3


def test_avr_intr_reset(bench):
    '''Check that a raised interrupt is reset on device reset
    '''

    _set_irq_state(bench, 3, True)
    bench.dev_model.reset()
    assert bench.irq_hook.pop_data(IntrStateChange, 3) == IntrState.Reset

    #Check that the ISR is not executed
    bench.sim_advance(1000)
    assert bench.dev.CPU.GPIOR0 == 0


def test_avr_intr_order(bench):
    '''Check the priority mechanism between interrupts raised at the same time
    '''

    qhook = QueuedSignalHook(bench.irq_sig)
    #Raise the PORTA (6), PORTD (20) and PORTC (24) interrupts
    _set_irq_state(bench, 24, True)
    _set_irq_state(bench, 20, True)
    _set_irq_state(bench, 3, True)
    qhook.clear()
    bench.sim_advance(1000)
    #Extract the aknowledgment signals and check the acknowledging order
    assert _extract_acks(qhook) == [ 3, 20, 24 ]


def test_avr_intr_reset_vector(bench):
    '''Check the dynamic reset vector change depending on fuse values
    '''

    bench.dev_model.reset(corelib.Device.ResetFlag.PowerOn)
    assert bench.probe.read_pc() == 0x0000

    #Clear BOOTRST (bit 0 of high fuse byte) and reset to apply the value
    fuse_nvm = bench.dev.nvms['Fuses']
    fuse_nvm.dbg_write(fuse_nvm[0x01] & 0xFE, 0x01)
    bench.dev_model.reset(corelib.Device.ResetFlag.PowerOn)

    #Check that after a reset, the PC is at the start of the Boot section
    assert bench.probe.read_pc() == 0x7000
