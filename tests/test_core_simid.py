# test_core_simid.py
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


import pytest
import yasimavr.lib.core as corelib
import yasimavr.lib.arch_xt as archlib


def test_sim_id_invalid():
    s = corelib.sim_id_t()
    assert s.str() == ""
    assert repr(s) == "INVALID_ID"

    s = corelib.sim_id_t(0)
    assert repr(s) == "INVALID_ID"


def test_sim_id_from_int():
    a = 0x3031323334353637
    s = corelib.sim_id_t(a)
    assert s.str() == "01234567"
    assert repr(s) == "[01234567]"


def test_sim_id_from_str():
    s = corelib.sim_id_t("TEST")
    assert s.str() == "TEST"
    assert repr(s) == "[TEST]"

    s = corelib.sim_id_t("012345678")
    assert s.str() == "01234567"


def test_sim_id_copy():
    s = corelib.sim_id_t("TEST")
    t = corelib.sim_id_t(s)

    assert s.str() == "TEST"
    assert t.str() == "TEST"
    assert s == t


def test_sim_id_concat():
    s = corelib.sim_id_t("TEST")
    s += 'Z'
    assert s.str() == "TESTZ"


def test_sim_id_attr():
    mux_cfg = archlib.ArchXT_PortMuxConfig.mux_config_t()
    mux_cfg.drv_id = "TEST"


