# test_core_intctrl.py
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
import yasimavr.lib.arch_xt as archlib
from _test_bench_xt import BenchXT, TESTFW_M4809
from _test_utils import DictSignalHook, QueuedSignalHook


#Dummy IRQ handler
class NMI_Handler(corelib.InterruptHandler): pass


@pytest.fixture
def bench():
    bench = BenchXT('atmega4809', TESTFW_M4809)

    #Connect to the interrupt controller signal
    _, reqdata = bench.dev_model.ctlreq(corelib.IOCTL_INTR, corelib.CTLREQ_GET_SIGNAL)
    bench.irq_sig = reqdata.data.value(corelib.Signal)
    bench.irq_hook = DictSignalHook(bench.irq_sig)

    #Register a dummy handler for INT1
    bench.nmi_handler = NMI_Handler()
    reqdata = corelib.ctlreq_data_t()
    reqdata.data = corelib.vardata_t(bench.nmi_handler)
    reqdata.index = 1
    _ = bench.dev_model.ctlreq(corelib.IOCTL_INTR, corelib.CTLREQ_INTR_REGISTER, reqdata)

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


def test_xt_intr_signal(bench):
    '''Check the signalling of the successive states of an IRQ
    '''

    _set_irq_state(bench, 6, True)
    assert bench.irq_hook.pop_data(IntrStateChange, 6) == IntrState.Raised

    #Check that the state is updated when the CPU acknowledges the interrupt
    bench.sim_advance(1)
    assert bench.irq_hook.pop_data(IntrStateChange, 6) == IntrState.Acknowledged

    #Check that the state is updated when the CPU completes the ISR
    bench.sim_advance(100)
    assert bench.irq_hook.pop_data(IntrStateChange, 6) == IntrState.Cancelled
    assert bench.dev.MISC.GPIOR0 == 6


def test_xt_intr_reset(bench):
    '''Check that a raised interrupt is reset on device reset
    '''

    _set_irq_state(bench, 6, True)
    bench.dev_model.reset()
    assert bench.irq_hook.pop_data(IntrStateChange, 6) == IntrState.Reset

    #Check that the ISR is not executed
    bench.sim_advance(1000)
    assert bench.dev.MISC.GPIOR0 == 0


def test_xt_intr_order(bench):
    '''Check the priority mechanism between interrupts raised at the same time
    '''

    qhook = QueuedSignalHook(bench.irq_sig)
    #Raise the PORTA (6), PORTD (20) and PORTC (24) interrupts
    _set_irq_state(bench, 24, True)
    _set_irq_state(bench, 20, True)
    _set_irq_state(bench, 6, True)
    qhook.clear()

    assert bench.dev.CPUINT.STATUS.LVL0EX == 0
    #Check that the status flag is raised when an interrupt is acknowledged
    bench.sim_advance(1)
    assert bench.dev.CPUINT.STATUS.LVL0EX == 1
    #Check that the status flag is cleared when interrupts have returned
    bench.sim_advance(1000)
    assert bench.dev.CPUINT.STATUS.LVL0EX == 0
    #Extract the aknowledgment signals and check the acknowledging order
    assert _extract_acks(qhook) == [ 6, 20, 24 ]

    #Raise the PORTA (6), PORTD (20) and PORTC (24) interrupts and let the CPU run
    bench.dev.CPUINT.LVL0PRI = 22
    _set_irq_state(bench, 24, True)
    _set_irq_state(bench, 20, True)
    _set_irq_state(bench, 6, True)
    qhook.clear()
    bench.sim_advance(1000)
    #Extract the aknowledgment signals
    #Check that the interrupts are picked in the order PORTC, PORTA, PORTD
    assert _extract_acks(qhook) == [ 24, 6, 20 ]


def test_xt_intr_status(bench):
    '''Check the status reporting for interrupt levels
    '''

    _set_irq_state(bench, 6, True)
    assert bench.dev.CPUINT.STATUS.LVL0EX == 0
    assert bench.dev.CPUINT.STATUS.LVL1EX == 0
    assert bench.dev.CPUINT.STATUS.NMIEX == 0
    #Check that the status flag is raised when an interrupt is acknowledged
    bench.sim_advance(1)
    assert bench.dev.CPUINT.STATUS.LVL0EX == 1
    assert bench.dev.CPUINT.STATUS.LVL1EX == 0
    assert bench.dev.CPUINT.STATUS.NMIEX == 0
    #Check that the status flag is cleared when the CPU returns from the ISR
    _advance_until_reti(bench)
    assert bench.dev.CPUINT.STATUS.LVL0EX == 0
    assert bench.dev.CPUINT.STATUS.LVL1EX == 0
    assert bench.dev.CPUINT.STATUS.NMIEX == 0

    bench.sim_advance(1) #required to pass the inhibit period
    bench.dev.CPUINT.LVL1VEC = 6
    _set_irq_state(bench, 20, True)
    bench.sim_advance(1)
    _set_irq_state(bench, 6, True)
    bench.sim_advance(1)
    #Check that the status flag is raised when an interrupt is acknowledged
    assert bench.dev.CPUINT.STATUS.LVL0EX == 1
    assert bench.dev.CPUINT.STATUS.LVL1EX == 1
    assert bench.dev.CPUINT.STATUS.NMIEX == 0
    #Check that the status flag is cleared when the CPU returns from the ISR
    _advance_until_reti(bench)
    assert bench.dev.CPUINT.STATUS.LVL0EX == 1
    assert bench.dev.CPUINT.STATUS.LVL1EX == 0
    assert bench.dev.CPUINT.STATUS.NMIEX == 0
    _advance_until_reti(bench)
    assert bench.dev.CPUINT.STATUS.LVL0EX == 0

    bench.sim_advance(1)
    _set_irq_state(bench, 20, True)
    bench.sim_advance(1)
    _set_irq_state(bench, 6, True)
    bench.sim_advance(1)
    _set_irq_state(bench, 1, True)
    bench.sim_advance(1)
    assert bench.dev.CPUINT.STATUS.LVL0EX == 1
    assert bench.dev.CPUINT.STATUS.LVL1EX == 1
    assert bench.dev.CPUINT.STATUS.NMIEX == 1
    #Check that the status flag is cleared when the CPU returns from the ISR
    _advance_until_reti(bench)
    assert bench.dev.CPUINT.STATUS.LVL0EX == 1
    assert bench.dev.CPUINT.STATUS.LVL1EX == 1
    assert bench.dev.CPUINT.STATUS.NMIEX == 0


def test_xt_intr_lvl(bench):
    '''Check the priority level 1 system
    '''

    qhook = QueuedSignalHook(bench.irq_sig)

    #Make PORTD interrupt a priority level 1, and do the same check as above
    bench.dev.CPUINT.LVL1VEC = 20
    _set_irq_state(bench, 24, True)
    _set_irq_state(bench, 20, True)
    _set_irq_state(bench, 6, True)
    qhook.clear()
    #Check that the interrupts are then picked in the order PORTA, PORTC
    bench.sim_advance(1000)
    assert _extract_acks(qhook) == [ 20, 6, 24 ]


def test_xt_intr_round_robin(bench):
    '''Check the round robin function
    '''

    bench.dev.CPUINT.CTRLA.LVL0RR = 1
    qhook = QueuedSignalHook(bench.irq_sig)

    #Raise the PORTA (6) and PORTD (20) interrupts and let the CPU run
    _set_irq_state(bench, 6, True)
    _set_irq_state(bench, 20, True)
    qhook.clear()
    bench.sim_advance(1000)
    #Check the ack order is as expected and LVL0PRI is updated
    assert _extract_acks(qhook) == [ 6, 20 ]
    assert bench.dev.CPUINT.LVL0PRI == 20

    _set_irq_state(bench, 6, True)
    _set_irq_state(bench, 24, True)
    qhook.clear()
    bench.sim_advance(1000)
    #Extract the aknowledgment signals
    #Check that the interrupts are picked in the order 24, 6 and LVL0PRI is updated
    assert _extract_acks(qhook) == [ 24, 6 ]
    assert bench.dev.CPUINT.LVL0PRI == 6


def test_xt_intr_ivsel(bench):
    '''Check the IVSEL function
    '''

    BREAK_OPCODE = bytes((0x98, 0x95))
    INTR6_OFFSET = 6*4
    APPCODE_BASE = 100

    #Program a SW break at the entry 6 in the Interrupt Vector Tables (both Boot and Application)
    bench.dev.nvms['Flash'].program(BREAK_OPCODE, INTR6_OFFSET)
    bench.dev.nvms['Flash'].program(BREAK_OPCODE, APPCODE_BASE*256 + INTR6_OFFSET)
    #Set BOOTEND to APPCODE_BASE
    fuse_nvm = bench.dev.nvms['Fuses']
    fuse_nvm.dbg_write(APPCODE_BASE, 0x08)
    #Reset the device to apply the new fuse value and run the simulation
    bench.dev_model.reset(corelib.Device.ResetFlag.PowerOn)
    bench.sim_advance(1000)
    #Raise the interrupt 6 and check that the Program Counter is in the Application IVT
    _set_irq_state(bench, 6, True)
    bench.sim_advance(1)
    assert bench.probe.read_pc() == APPCODE_BASE*256 + INTR6_OFFSET

    #Repeat the same as above with IVSEL=1
    bench.dev_model.reset(corelib.Device.ResetFlag.PowerOn)
    bench.sim_advance(1000)
    bench.dev.CPUINT.CTRLA.IVSEL = 1
    #Raise the interrupt 6 and check that the Program Counter is in the Boot IVT
    _set_irq_state(bench, 6, True)
    bench.sim_advance(1)
    assert bench.probe.read_pc() == INTR6_OFFSET


def text_xt_intr_nmi(bench):
    '''Check that the interrupt 1 is non-maskable
    '''

    #Check that INT1 is acknowledged by the CPU even with GIE cleared
    bench.dev.CPU.SREG.I = 0
    _set_irq_state(bench, 1, True)
    bench.sim_advance(1)
    assert bench.irq_hook.pop_data(IntrStateChange, 1) == IntrState.Acknowledged
