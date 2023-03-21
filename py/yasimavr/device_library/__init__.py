# __init__.py
#
# Copyright 2021 Clement Savergne <csavergne@yahoo.com>
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

'''Python package which provides facilities for :
* Device building and configuration
* I/O registry and memory direct access
'''

from .descriptors import *
from .accessors import *
import importlib

_factory_cache = {}

def load_device(dev_name, verbose=False):
    low_dev_name = dev_name.lower()

    if low_dev_name in _factory_cache:
        if verbose:
            print('Using device factory from cache')
        return _factory_cache[low_dev_name](low_dev_name)

    from .builders import _base
    _base.VERBOSE = verbose

    path_device_db = os.path.join(os.path.dirname(__file__), 'configs', 'devices.yml')
    device_db = load_config_file(path_device_db)
    for f, dev_list in device_db.items():
        if low_dev_name in dev_list:
            dev_factory = f
            break
    else:
        raise Exception('No model found for ' + dev_name)

    mod_name = '.builders.' + dev_factory
    if verbose:
        print('Loading device factory module', mod_name)

    dev_mod = importlib.import_module(mod_name, __package__)
    importlib.invalidate_caches()

    factory = getattr(dev_mod, 'device_factory')
    _factory_cache[low_dev_name] = factory
    return factory(low_dev_name)


def model_list():
    models = []
    p = os.path.join(os.path.dirname(__file__), 'builders')
    for fn in os.listdir(p):
        fp = os.path.join(p, fn)
        if not os.path.isfile(fp): continue
        if fn.startswith('device_') and fn.endswith('.py'):
            fn = os.path.splitext(fn)[0].replace('device_', '')
            models.append(fn)

    return models
