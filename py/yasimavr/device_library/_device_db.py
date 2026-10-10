# _device_db.py
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


import os.path

from yaml import load as _yaml_load
try:
    from yaml import CLoader as _YAMLLoader
except ImportError:
    from yaml import SafeLoader as _YAMLLoader


LibraryRepository = os.path.join(os.path.dirname(__file__), 'configs')
LibraryModelDatabase = os.path.join(LibraryRepository, 'devices.yml')


def load_config_file(fn):
    if not fn.lower().endswith('.yml'):
        fn += '.yml'
    with open(fn) as f:
        return _yaml_load(f, _YAMLLoader)


class DeviceDatabase:

    def __init__(self):
        self._db_file = None

    def find_device_entry(self, model):
        model = os.path.basename(model).lower()

        if self._db_file is None:
            self._db_file = load_config_file(LibraryModelDatabase)

        for factory_name, dev_list in self._db_file.items():
            for dev in dev_list:
                if os.path.basename(dev).lower() == model:
                    return (factory_name, dev)

        raise Exception('No model found for ' + model)

    def find_device_path(self, model):
        _, fn = self.find_device_entry(model)
        return os.path.join(LibraryRepository, fn)

    def model_list(self):
        if self._db_file is None:
            self._db_file = load_config_file(LibraryModelDatabase)

        models = []
        for dev_list in self._db_file.values():
            models.extend(dev_list)

        models = [ os.path.basename(m) for m in models ]

        return models


device_database = DeviceDatabase()
