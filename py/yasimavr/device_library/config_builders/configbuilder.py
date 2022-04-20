# configbuilder.py
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
import inspect

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



class _PeripheralConfigBuilder:
    '''Helper class to build a peripheral configuration structure
    
    The main use is to convert YAML configuration entry into values
    for the C++ structures defined to configure peripherals.
    
    The conversion process is as follow, if a convertor is provided:
    for each entry in the YAML configuration map:
     1 - call the converter function with the following arguments:
        - C++ structure isntance,
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
    _SpecialAttributes = ['vector_count', 'reg_base']
        
    
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
        if s:
            print('Untouched attributes of %s: %s' % (cfg.__class__.__name__, str(s)))
        
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
            return convert_regbit(yml_val, per_desc)
            
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
            result |= _corelib.AVR_CoreConfiguration.Attributes[lib_flag_name]
    
    return result

def convert_regbit(rb_desc, per_desc):
    if isinstance(rb_desc, str):
        reg_name = rb_desc
        field_names = []
    else:
        reg_name, sf = rb_desc
        field_names = [ fn.strip() for fn in sf.split('|') ]
    
    reg = per_desc.class_descriptor.registers[reg_name]
    
    if field_names:
        fields = [ reg.fields[fn] for fn in field_names ]
    else:
        fields = reg.fields.values()
    
    bit = 7
    mask = 0
    for f in fields:
        b, m = f.shift_mask()
        bit = min(bit, b)
        mask |= m << b
    
    rb = _corelib.regbit_t(per_desc.reg_address(reg_name), bit, mask)
    
    return rb


def convert_dummy_controller_config(per_desc):
    yml_cfg = dict(per_desc.class_descriptor.config)
    
    py_regs = []
    for yml_item in yml_cfg['regs']:
        reg_cfg = _corelib.AVR_DummyController.dummy_register_t()
        if isinstance(yml_item, list):
            reg_name, reg_reset = yml_item
        else:
            reg_name = yml_item
            reg_reset = 0
        
        reg_cfg.reg = convert_regbit(reg_name, per_desc)
        reg_cfg.reset = reg_reset
        
        py_regs.append(reg_cfg)
    
    return py_regs
