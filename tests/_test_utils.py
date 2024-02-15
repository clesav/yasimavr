# _test_utils.py
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
import collections


PinState = corelib.Pin.State


class DictSignalHook(corelib.SignalHook):

    def __init__(self, signal=None, tag=0):
        super().__init__()
        if signal is not None:
            signal.connect(self, tag)
        self._signals = {}

    def pop(self, sigid=0, index=0):
        k = (int(sigid), index)
        if k in self._signals:
            return self._signals.pop(k)
        else:
            return None

    def pop_data(self, sigid=0, index=0):
        if self.has_data(sigid, index):
            return self.pop(sigid, index)[0].data
        else:
            return None

    def has_data(self, sigid=0, index=0):
        k = (int(sigid), index)
        return k in self._signals

    def raised(self, sigdata, tag):
        k = (int(sigdata.sigid), sigdata.index)
        self._signals[k] = (sigdata, tag)


class QueuedSignalHook(corelib.SignalHook):

    def __init__(self, signal=None, tag=0, split_by_index=False):
        super().__init__()
        if signal is not None:
            signal.connect(self, tag)
        self._signals = {}
        self._split_by_index = split_by_index

    def _make_key(self, sigid, index):
        if not self._split_by_index: index = 0
        return int(sigid), index

    def pop(self, sigid=0, index=0):
        k = self._make_key(sigid, index)
        if k in self._signals:
            q = self._signals[k]
            v = q.popleft()
            if not len(q):
                del self._signals[k]
            return v
        else:
            return None

    def pop_data(self, sigid=0, index=0):
        if self.has_data(sigid, index):
            return self.pop(sigid, index)[0].data
        else:
            return None

    def has_data(self, sigid=0, index=0):
        k = self._make_key(sigid, index)
        return k in self._signals

    def raised(self, sigdata, tag):
        #print('raised: id=%d,ix=%d,v=%s,tag=%d' % (sigdata.sigid, sigdata.index, sigdata.data.value(), tag))

        k = self._make_key(sigdata.sigid, sigdata.index)
        if k in self._signals:
            q = self._signals[k]
        else:
            q = collections.deque()
            self._signals[k] = q

        q.append((sigdata, tag))
