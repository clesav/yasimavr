# _base.py
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

import yasimavr.lib.core as _corelib
from ..descriptors import convert_to_regbit, convert_to_regbit_compound, convert_to_bitmask
import inspect

global VERBOSE
VERBOSE = False

class _ConfigProxy:

    def __init__(self, cfg):
        self._cfg = cfg
        self._touched = set()
        self._ready = True

    def __getattr__(self, name):
        if name.startswith('_'):
            raise AttributeError()
        else:
            return getattr(self._cfg, name)

    def __setattr__(self, name, value):
        if not getattr(self, '_ready', False):
            object.__setattr__(self, name, value)
        else:
            self._touched.add(name)
            setattr(self._cfg, name, value)

    def untouched(self):
        s = set(k for k in dir(self._cfg)
                if (not k.startswith('__') and
                    not inspect.isclass(getattr(self._cfg, k))))
        return s - self._touched

    def _proxy_touch(self, arg):
        self._touched.add(arg)


class PeripheralConfigBuilder:
    '''Helper class to build a peripheral configuration structure

    The main use is to convert YAML configuration entry into values
    for the C++ structures defined to configure peripherals.

    The conversion process is as follow, if a convertor is provided:
    for each entry in the YAML configuration map:
     1 - call the converter function with the following arguments:
        - C++ structure instance,
        - attribute name,
        - YAML value,
        - peripheral descriptor
     2- if the function succeeds, the returned value is assigned to the
        C++ structure attribute
     3- if the function raises an exception, indicating that it failed to
        convert, the pre-defined converter is tried. If it fails as well,
        the exception is raised again.
    If no converter is provided, step 1 above is skipped
    '''

    #Special attributes names that do not need to be in the YAML file
    _SpecialAttributes = ['vector_count', 'vector_size', 'reg_base']


    def __init__(self, cfg_class, converter=None):
        self._cfg_class = cfg_class
        self._converter = converter

    def __call__(self, per_desc):
        cfg = self._cfg_class()
        cfg_proxy = _ConfigProxy(cfg)

        #Mixes the YAML configurations of the descriptors for the peripheral class
        #and the peripheral instance
        yml_cfg = dict(per_desc.class_descriptor.config)
        yml_cfg.update(per_desc.config)

        for attr in yml_cfg:

            yml_val = yml_cfg[attr]

            if self._converter is None:
                default_val = getattr(cfg, attr)
                conv_val = self._convert_yml_value(attr, yml_val, default_val, per_desc)
                if conv_val is None:
                    raise ValueError(attr)
                else:
                    setattr(cfg_proxy, attr, conv_val)

            else:
                try:
                    self._converter(cfg_proxy, attr, yml_val, per_desc)
                except:
                    default_val = getattr(cfg, attr)
                    conv_val = self._convert_yml_value(attr, yml_val, default_val, per_desc)

                    if conv_val is None:
                        raise
                    else:
                        setattr(cfg_proxy, attr, conv_val)

        #Case of special attributes. If they are defined in the config class, we convert them.
        for special_attr in self._SpecialAttributes:
            if hasattr(cfg, special_attr):
                default_val = getattr(cfg, special_attr)
                conv_val = self._convert_yml_value(special_attr, None, default_val, per_desc)
                if conv_val is None:
                    raise ValueError(special_attr)
                else:
                    setattr(cfg_proxy, special_attr, conv_val)


        s = cfg_proxy.untouched()
        if VERBOSE and s:
            print('### Untouched attributes of %s: %s' % (cfg.__class__.__name__, str(s)))

        return cfg


    def _convert_yml_value(self, attr, yml_val, default_val, per_desc):
        '''Pre-defined conversions:
        - (Special) attribute named 'vector_count' :
            length of the interrupt vector map
        - (Special) attribute named 'reg_base' :
            base address of the peripheral instance
        - Attribute name starting with 'iv_':
            interrupt vector name converted to int
        - Attribute name starting with 'reg_':
            register name converted to int
        - Attribute name starting with 'rb_':
            [register name, fields list ] converted to regbit_t
        - Default value is a integer or a float:
            the value is passed on unchanged
        '''

        if attr == 'vector_count':
            return len(per_desc.device.interrupt_map.vectors)

        elif attr == 'vector_size':
            return per_desc.device.interrupt_map.vector_size

        elif attr == 'reg_base':
            return per_desc.reg_base

        elif attr.startswith('iv_'):
            return per_desc.device.interrupt_map.vectors.index(yml_val)

        elif attr.startswith('reg_'):
            if isinstance(yml_val, int):
                return yml_val
            elif isinstance(yml_val, str):
                return per_desc.reg_address(yml_val)
            else:
                return None

        elif attr.startswith('rb_') or isinstance(default_val, _corelib.regbit_t):
            return convert_to_regbit(yml_val, per_desc)

        elif attr.startswith('rbc_') or isinstance(default_val, _corelib.regbit_compound_t):
            return convert_to_regbit_compound(yml_val, per_desc)

        elif attr.startswith('bm_') or isinstance(default_val, _corelib.bitmask_t):
            return convert_to_bitmask(yml_val, per_desc)

        elif isinstance(default_val, (int, float)):
            return yml_val

        else:
            return None

#========================================================================================

_core_attrs_map = {
    'ExtendedAddressing': 'extended_addressing',
    'ClearGIEOnInt': 'clear_GIE_on_int'
}

def get_core_attributes(dev_desc):
    result = 0
    for lib_flag_name, yml_flag_name in _core_attrs_map.items():
        if dev_desc.core_attributes[yml_flag_name]:
            result |= _corelib.CoreConfiguration.Attributes[lib_flag_name]

    return result


def dummy_config_builder(per_descriptor):
    yml_cfg = dict(per_descriptor.class_descriptor.config)

    py_regs = []
    for yml_item in yml_cfg['regs']:
        reg_cfg = _corelib.DummyController.dummy_register_t()
        if isinstance(yml_item, list):
            reg_name, reg_reset = yml_item
        else:
            reg_name = yml_item
            reg_reset = 0

        reg_cfg.reg = convert_to_regbit(reg_name, per_descriptor)
        reg_cfg.reset = reg_reset

        py_regs.append(reg_cfg)

    return py_regs


class PeripheralBuilder:
    """Default class for a peripheral builder
    Peripheral builder instances build peripheral model instances
    """

    def __init__(self, per_model, config_builder):
        """Initialise a peripheral builder
        per_model: peripheral model class
        config_builder: callable object that returns a configuration object to
                        be passed on to create a peripheral model
        """
        self.per_model = per_model
        self.config_builder = config_builder

    def build(self, per_name, per_config):
        """Constructs a new peripheral model instance
        per_name : Name of the peripheral model instance to construct
        per_config : Configuration object to use for the build
        Returns a new peripheral model instance that can be attached to a device
        """
        build_args = self._get_build_args(per_name, per_config)
        per_instance = self.per_model(*build_args)
        return per_instance

    def _get_build_args(self, per_name, per_config):
        """Provides the arguments to the peripheral model constructor.
        The default implementation only uses the per_config object.
        """
        return (per_config,)

    def __repr__(self):
        return '%s(model=%s)' % (self.__class__.__name__, self.per_model.__name__)


class IndexedPeripheralBuilder(PeripheralBuilder):
    """Specialisation of PeripheralBuilder for peripherals with an indexed name,
    such as USART0, USART1, ...
    The index is extracted from the name and passed on as 1st argument to the
    model constructor.
    The index is optional, and if absent, 0 is used.
    """

    def _get_build_args(self, per_name, per_config):
        try:
            ix = int(per_name[-1])
        except ValueError:
            return (0, per_config)
        else:
            return (ix, per_config)


class LetteredPeripheralBuilder(PeripheralBuilder):
    """Specialisation of PeripheralBuilder for peripherals with an lettered name,
    such as PORTA, PORTB, ...
    The index is extracted from the name and passed on as 1st argument to the
    model constructor.
    """

    def _get_build_args(self, per_name, per_config):
        return (per_name[-1], per_config)


class DummyPeripheralBuilder(PeripheralBuilder):
    """Specialisation of PeripheralBuilder for dummy peripherals (
    using the DummyController model) requiring the CTLID.
    """

    def __init__(self, ctl_id):
        super().__init__(_corelib.DummyController, dummy_config_builder)
        self._ctl_id = ctl_id

    def _get_build_args(self, per_name, per_config):
        return (self._ctl_id, per_config)


class DeviceBuildError(Exception):
    """Error type raised during the device and peripheral building process.
    """
    pass


class DeviceBuilder:
    """Generic device builder object, factory for device simulation models.
    Users don't normally have to instantiate directly this class but rather use one of
    he sub-classes for each of the core architectures.
    It implements a cache for both peripheral builders and configuration structure.
    """

    _builder_cache = {}

    def __init__(self, dev_descriptor):
        self._dev_descriptor = dev_descriptor
        self._core_config = None
        self._dev_config = None
        self._per_configs = {}
        self._per_builders = {}

    @property
    def dev_descriptor(self):
        return self._dev_descriptor

    def _build_core_config(self, dev_desc):
        raise NotImplementedError

    def _build_device_config(self, dev_desc):
        raise NotImplementedError

    def get_device_config(self):
        if self._dev_config is None:
            self._core_config = self._build_core_config(self._dev_descriptor)
            self._dev_config = self._build_device_config(self._dev_descriptor, self._core_config)

        return self._dev_config

    def build_peripheral(self, device, per_name):
        """Build a peripheral from the device descriptor and attach it to
        a device model.

        :param device: instance of Device model
        :param per_names: name of the peripheral instance to build
        """

        if VERBOSE:
            print('  Building peripheral', per_name)

        per_descriptor = self._dev_descriptor.peripherals[per_name]
        per_class = per_descriptor.per_class

        if per_class in self._per_builders:
            per_builder = self._per_builders[per_class]
        else:
            per_builder = self._get_peripheral_builder(per_class)
            self._per_builders[per_class] = per_builder

        if VERBOSE:
            print('    Peripheral builder found:', per_builder)

        if per_builder is None:
            return

        if per_name in self._per_configs:
            per_config = self._per_configs[per_name]
        else:
            if VERBOSE:
                print('    Building configuration for peripheral', per_name)
            per_config = per_builder.config_builder(per_descriptor)
            self._per_configs[per_name] = per_config

        per_instance = per_builder.build(per_descriptor.name, per_config)
        device.attach_peripheral(per_instance)

    def build_peripherals(self, device, per_names):
        """Build a list of peripherals from the device descriptor and attach them to
        a device model.

        :param device: instance of Device model
        :param per_names: list of peripheral names to build
        """

        for per_name in per_names:
            self.build_peripheral(device, per_name)

    def _get_peripheral_builder(self, per_class):
        raise NotImplementedError

    @classmethod
    def build_device(cls, dev_desc, dev_class):
        """Builds a device simulator model using the information from a descriptor.

        :param dev_desc: device descriptor object
        :param dev_class: base class for the device model to build and configure
        """

        model = dev_desc.name
        #Find the appropriate builder instance (use the cache)
        if model in cls._builder_cache:
            if VERBOSE:
                print('DeviceBuilder found in cache for model', model)
            builder = cls._builder_cache[model]
        else:
            if VERBOSE:
                print('Creating new DeviceBuilder for model', model)
            builder = cls(dev_desc)
            cls._builder_cache[model] = builder

        dev = dev_class(dev_desc, builder)
        dev._builder_ = builder
        dev._descriptor_ = dev_desc
        return dev

    @classmethod
    def clear_cache(cls):
        """Clears the internal class cache for device builders
        """

        cls._builder_cache.clear()


def convert_enum_member(klass, v):
    if v is None:
        return klass(0)
    elif isinstance(v, str):
        return klass[v]
    else:
        return klass(v)
