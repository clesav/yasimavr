# test_XT_VREF.py
#
# Copyright 2026 Clement Savergne <csavergne@yahoo.com>
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
Automatic tests for the VREF simulation model (XT architecture)
'''


import pytest
from _test_bench_xt import BenchXT, TESTFW_M4809
import yasimavr.lib.core as corelib
from yasimavr.device_library import load_device


class Bench(BenchXT):

    def req_getref(self, src, ch=0):
        vref_reqinfo = corelib.VREF.getset_t(corelib.VREF.Source[src], ch)
        reqdata = corelib.ctlreq_data_t()
        reqdata.data = corelib.vardata_t(vref_reqinfo)
        ok, _ = self.dev_model.ctlreq(corelib.IOCTL_VREF, corelib.CTLREQ_VREF_GET, reqdata)
        if not ok:
            raise Exception()
        return vref_reqinfo.voltage

    def req_setref(self, src, ch, v):
        vref_reqinfo = corelib.VREF.getset_t(corelib.VREF.Source[src], ch, v)
        reqdata = corelib.ctlreq_data_t()
        reqdata.data = corelib.vardata_t(vref_reqinfo)
        ok, _ = self.dev_model.ctlreq(corelib.IOCTL_VREF, corelib.CTLREQ_VREF_SET_REF, reqdata)
        if not ok:
            raise Exception()


@pytest.fixture
def bench():
    dev_model = load_device('atmega4809')
    b = Bench(dev_model, TESTFW_M4809)
    return b


def test_xt_vref_vcc(bench):
    assert bench.req_getref('VCC') == 5.0
    assert bench.req_getref('AVCC') == 1.0

    bench.req_setref('VCC', 0, 10.0)
    assert bench.req_getref('VCC') == 10.0
    assert bench.req_getref('AVCC') == 1.0

    bench.req_setref('VCC', 0, 0.0)
    with pytest.raises(Exception):
        bench.req_getref('AREF')

    bench.req_setref('VCC', 0, 5.0)
    assert bench.req_getref('AREF') == 1.0


def test_xt_vref_bandgap(bench):
    VREF = bench.dev.VREF

    assert bench.req_getref('Mux', 0) == pytest.approx(0.11)

    VREF.CTRLA.ADC0REFSEL = 0x01
    assert bench.req_getref('Mux', 0) == pytest.approx(0.22)

    VREF.CTRLA.ADC0REFSEL = 0x02
    assert bench.req_getref('Mux', 0) == pytest.approx(0.5)

    VREF.CTRLA.ADC0REFSEL = 0x03
    assert bench.req_getref('Mux', 0) == pytest.approx(0.86)

    VREF.CTRLA.ADC0REFSEL = 0x03
    assert bench.req_getref('Mux', 0) == pytest.approx(0.86)

    assert bench.req_getref('Mux', 1) == pytest.approx(0.11)

    VREF.CTRLA.AC0REFSEL = 0x07
    assert bench.req_getref('Mux', 1) == 1.0
