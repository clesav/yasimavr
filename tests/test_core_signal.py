# test_core_signal.py
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


import yasimavr.lib.core as corelib
from _test_utils import DictSignalHook


def test_signal_connect():
    sig = corelib.Signal()
    hook = DictSignalHook()

    sig.raise_()
    assert not hook.has_data()

    sig.connect(hook)
    sig.raise_()
    assert hook.has_data()

    hook.pop()
    assert not hook.has_data()

    sig.disconnect(hook)
    sig.raise_()
    assert not hook.has_data()


def test_signal_sigid():
    sig = corelib.Signal()
    hook = DictSignalHook(sig)

    sig.raise_(0, 0xAA)
    sig.raise_(1, 0x55)

    assert hook.has_data(0)
    assert hook.has_data(1)

    sigdata0, hooktag0 = hook.pop(0)
    sigdata1, hooktag1 = hook.pop(1)
    assert sigdata0.sigid == 0
    assert sigdata0.data.value() == 0xAA
    assert hooktag0 == 0
    assert sigdata1.sigid == 1
    assert sigdata1.data.value() == 0x55
    assert hooktag1 == 0


def test_signal_hooktag():
    sig = corelib.DataSignal()
    hook0 = DictSignalHook(sig, 0)
    hook1 = DictSignalHook(sig, 1)

    sig.raise_(0, 1)

    assert hook0.has_data(0)

    sigdata, hooktag = hook0.pop(0)
    assert sigdata.data.value() == 1
    assert hooktag == 0

    assert hook1.has_data(0)

    sigdata, hooktag = hook1.pop(0)
    assert sigdata.data.value() == 1
    assert hooktag == 1


def test_datasignal_set_get():
    sig = corelib.DataSignal()

    assert not sig.has_data(0)

    sig.set_data(0, 1.0)

    assert sig.has_data(0)
    assert sig.data(0).value() == 1.0

    assert not sig.has_data(1)

    sig.set_data(0, 2.0, 1)
    assert sig.data(0, 0).value() == 1.0
    assert sig.data(0, 1).value() == 2.0


def test_datasignal_raise():
    sig = corelib.DataSignal()
    hook = DictSignalHook(sig, 0)

    sig.set_data(0, 1.0)
    assert sig.has_data(0)
    assert not hook.has_data(0)

    sig.raise_(0, 1.0)
    assert sig.has_data(0)
    assert hook.has_data(0)

    sigdata, _ = hook.pop(0)
    assert sigdata.data.value() == 1.0
