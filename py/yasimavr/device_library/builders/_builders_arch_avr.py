# _builders_arch_avr.py
#
# Copyright 2022-2024 Clement Savergne <csavergne@yahoo.com>
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
This module defines 'convertors' that take a peripheral descriptor
and returns a configuration structure used to initialised the C++ level objects
This module applies to devices ATmega48A/PA/88A/PA/168A/PA/328/P
'''

from ...lib import core as _corelib
from ...lib import arch_avr as _archlib
from ._base import (PeripheralBuilder, PeripheralConfigBuilder,
                    IndexedPeripheralBuilder,
                    DeviceBuilder, DeviceBuildError,
                    get_core_attributes, convert_to_regbit, convert_enum_member)


#========================================================================================
#Intctrl register configuration

def _get_intctrl_builder():
    def _get_intctrl_config(per_desc):
        return (len(per_desc.device.interrupt_map.vectors),
                per_desc.device.interrupt_map.vector_size)
    
    class builder (PeripheralBuilder):
        def _get_build_args(self, per_name, per_config):
            return per_config

    return builder(_archlib.ArchAVR_IntCtrl, _get_intctrl_config)


#========================================================================================
#Sleep controller configuration

def _slpctrl_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'modes':
        py_modes = []
        for mode_reg_value, mode_name in yml_val.items():
            mode_cfg = _corelib.SleepConfig.mode_config_t()
            mode_cfg.reg_value = mode_reg_value
            mode_cfg.mode = _corelib.SleepMode[mode_name]

            int_mask = per_desc.device.interrupt_map.sleep_mask[mode_name]
            mode_cfg.int_mask = int_mask + [0] * (16 - len(int_mask)) #padding to length=16

            py_modes.append(mode_cfg)

        cfg.modes = py_modes

    else:
        raise Exception('Converter not implemented for ' + attr)

def _get_slpctrl_builder():
    cfg_builder = PeripheralConfigBuilder(_corelib.SleepConfig, _slpctrl_convertor)
    return PeripheralBuilder(_corelib.SleepController, cfg_builder)



#========================================================================================
#Fuses controller configuration

def _fuses_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'boot_sizes':
        sizes = []
        for i, s in enumerate(yml_val):
            size_cfg = _archlib.ArchAVR_FusesConfig.bootsize_config_t()
            size_cfg.reg_value = i
            size_cfg.boot_size = s
            sizes.append(size_cfg)

        cfg.boot_sizes = sizes

    else:
        raise Exception('Converter not implemented for ' + attr)

def _get_fuses_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_FusesConfig, _fuses_convertor)
    return PeripheralBuilder(_archlib.ArchAVR_Fuses, cfg_builder)


#========================================================================================
#NVM controller configuration

def _get_nvmctrl_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_NVMConfig)
    return PeripheralBuilder(_archlib.ArchAVR_NVM, cfg_builder)


#========================================================================================
#Misc register configuration

def _misc_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'gpior':
        cfg.gpior = [ per_desc.reg_address(v) for v in yml_val ]
    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_misc_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_MiscConfig, _misc_convertor)
    return PeripheralBuilder(_archlib.ArchAVR_MiscRegCtrl, cfg_builder)


#========================================================================================
#General I/O port configuration

def _get_port_config(per_desc):
    cfg = _archlib.ArchAVR_PortConfig()
    cfg.name = per_desc.name[-1]
    cfg.reg_port = per_desc.reg_address('PORT')
    cfg.reg_pin = per_desc.reg_address('PIN')
    cfg.reg_dir = per_desc.reg_address('DDR')
    return cfg


def _get_port_builder():
    return PeripheralBuilder(_archlib.ArchAVR_Port, _get_port_config)


#========================================================================================
#External interrupts configuration

def _extint_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'extint_pins':
        cfg.extint_pins = [ _corelib.str_to_id(p) for p in yml_val ]
    elif attr == 'pcint_pins':
        cfg.pcint_pins= [ _corelib.str_to_id(p) for p in yml_val ]
    elif attr == 'reg_pcint_mask':
        cfg.reg_pcint_mask = [ per_desc.reg_address(r) for r in yml_val ]
    elif attr == 'extint_vector':
        cfg.extint_vector = [ per_desc.device.interrupt_map.vectors.index(v) for v in yml_val ]
    elif attr == 'pcint_vector':
        cfg.pcint_vector = [ per_desc.device.interrupt_map.vectors.index(v) for v in yml_val ]
    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_extint_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_ExtIntConfig, _extint_convertor)
    return PeripheralBuilder(_archlib.ArchAVR_ExtInt, cfg_builder)


#========================================================================================
#Timers/counters configuration

def _timer_convertor(cfg, attr, yml_val, per_desc):

    CFG = _archlib.ArchAVR_TimerConfig

    if attr == 'clocks':
        py_clocks = []
        for reg_value, clk_cfg_yml in yml_val.items():
            clk_cfg = _archlib.ArchAVR_TimerConfig.clock_config_t()
            clk_cfg.reg_value = reg_value
            clk_cfg.source = _corelib.TimerCounter.TickSource[clk_cfg_yml[0]]
            clk_cfg.div = clk_cfg_yml[1] if len(clk_cfg_yml) > 1 else 1
            py_clocks.append(clk_cfg)
        cfg.clocks = py_clocks

    elif attr == 'modes':
        py_modes = []
        for reg_value, mode_cfg_yml in yml_val.items():
            mode_cfg = CFG.mode_config_t()
            mode_cfg.reg_value = reg_value
            mode_cfg.top = convert_enum_member(CFG.Top, mode_cfg_yml.get('top'))
            mode_cfg.top_fixed_value = mode_cfg_yml.get('top_fixed_value', 0)
            mode_cfg.ovf = convert_enum_member(CFG.OVF, mode_cfg_yml.get('ovf'))
            mode_cfg.ocr = convert_enum_member(CFG.OCR, mode_cfg_yml.get('ocr'))
            mode_cfg.double_slope = bool(mode_cfg_yml.get('double_slope', False))
            mode_cfg.foc_disabled = bool(mode_cfg_yml.get('foc_disabled', False))
            mode_cfg.com_variant = mode_cfg_yml.get('com_variant', 0)
            py_modes.append(mode_cfg)

        cfg.modes = py_modes

    elif attr == 'oc_channels':
        oc_cfg_list = []
        for oc_cfg_yml in yml_val:
            oc_cfg = CFG.OC_config_t()
            oc_cfg.reg_oc = per_desc.reg_address(oc_cfg_yml['reg_oc'])
            oc_cfg.vector.num = per_desc.device.interrupt_map.vectors.index(oc_cfg_yml['vector'][0])
            oc_cfg.vector.bit = oc_cfg_yml['vector'][1]
            oc_cfg.rb_mode = convert_to_regbit(oc_cfg_yml['rb_mode'], per_desc)
            oc_cfg.rb_force = convert_to_regbit(oc_cfg_yml['rb_force'], per_desc)
            oc_cfg_list.append(oc_cfg)

        cfg.oc_channels = oc_cfg_list

    elif attr == 'vect_ovf':
        vect_cfg = CFG.vector_config_t()
        vect_cfg.num = per_desc.device.interrupt_map.vectors.index(yml_val[0])
        vect_cfg.bit = yml_val[1]
        cfg.vect_ovf = vect_cfg

    elif attr == 'vect_icr':
        vect_cfg = CFG.vector_config_t()
        vect_cfg.num = per_desc.device.interrupt_map.vectors.index(yml_val[0])
        vect_cfg.bit = yml_val[1]
        cfg.vect_icr = vect_cfg

    elif attr == 'com_modes':
        com_modes = []
        for variant_opt in yml_val:
            variant_cfg = []
            for reg_value, raw_opt in variant_opt.items():
                com_cfg = _archlib.ArchAVR_TimerConfig.COM_config_t()
                com_cfg.reg_value = reg_value
                com_cfg.up = convert_enum_member(CFG.COM, raw_opt & 0x0F)
                com_cfg.down = convert_enum_member(CFG.COM, (raw_opt >> 4) & 0x0F)
                com_cfg.top = convert_enum_member(CFG.COM, (raw_opt >> 8) & 0x0F)
                com_cfg.bottom = convert_enum_member(CFG.COM, (raw_opt >> 12) & 0x0F)
                variant_cfg.append(com_cfg)

            com_modes.append(variant_cfg)

        cfg.com_modes = com_modes

    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_timer_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_TimerConfig, _timer_convertor)
    return IndexedPeripheralBuilder(_archlib.ArchAVR_Timer, cfg_builder)


#========================================================================================
#ADC configuration

def _adc_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'channels':
        py_channels = []
        for reg_value, item in yml_val.items():
            chan_cfg = _corelib.ADC.channel_config_t()
            chan_cfg.reg_value = reg_value
            if isinstance(item, list):
                chan_cfg.type = _corelib.ADC.Channel[item[0]]
                chan_cfg.pin_p = _corelib.str_to_id(item[1])
                chan_cfg.pin_n = _corelib.str_to_id(item[2]) if len(item) > 2 else 0
                chan_cfg.gain = int(item[3]) if len(item) > 3 else 1
            else:
                chan_cfg.type = _corelib.ADC.Channel[item]
                chan_cfg.pin_p = 0
                chan_cfg.pin_n = 0
                chan_cfg.gain = 1

            py_channels.append(chan_cfg)

        cfg.channels = py_channels

    elif attr == 'references':
        py_refs = []
        for reg_value, item in yml_val.items():
            ref_cfg = _archlib.ArchAVR_ADCConfig.reference_config_t()
            ref_cfg.reg_value = reg_value
            ref_cfg.source = _corelib.VREF.Source[item]
            py_refs.append(ref_cfg)

        cfg.references = py_refs

    elif attr == 'clk_ps_factors':
        cfg.clk_ps_factors = yml_val

    elif attr == 'triggers':
        py_triggers = []
        for reg_value, item in yml_val.items():
            trig_cfg = _archlib.ArchAVR_ADCConfig.trigger_config_t()
            trig_cfg.reg_value = reg_value
            trig_cfg.trig_type = _archlib.ArchAVR_ADCConfig.Trigger[item]
            py_triggers.append(trig_cfg)

        cfg.triggers = py_triggers

    elif attr == 'int_vector':
        cfg.int_vector = per_desc.device.interrupt_map.vectors.index(yml_val)

    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_adc_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_ADCConfig, _adc_convertor)
    return IndexedPeripheralBuilder(_archlib.ArchAVR_ADC, cfg_builder)


#========================================================================================
#ACP configuration

def _acp_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'mux_pins':
        py_pins = []
        for reg_value, item in yml_val.items():
            mux_cfg = _archlib.ArchAVR_ACPConfig.mux_config_t()
            mux_cfg.reg_value = reg_value
            mux_cfg.pin = _corelib.str_to_id(item)
            py_pins.append(mux_cfg)

        cfg.mux_pins = py_pins

    elif attr == 'pos_pin':
        cfg.pos_pin = _corelib.str_to_id(yml_val)

    elif attr == 'neg_pin':
        cfg.neg_pin = _corelib.str_to_id(yml_val)

    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_acp_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_ACPConfig, _acp_convertor)
    return IndexedPeripheralBuilder(_archlib.ArchAVR_ACP, cfg_builder)


#========================================================================================
#Reference voltage configuration

def _get_vref_bandgap(per_desc):
    return per_desc.class_descriptor.config['bandgap']


def _get_vref_builder():
    return PeripheralBuilder(_archlib.ArchAVR_VREF, _get_vref_bandgap)


#========================================================================================
#USART configuration

def _usart_convertor(cfg, attr, yml_val, per_desc):
    if attr in ('rxc_vector', 'txc_vector', 'txe_vector'):
        v = per_desc.device.interrupt_map.vectors.index(yml_val)
        setattr(cfg, attr, v)
    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_usart_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_USARTConfig, _usart_convertor)
    return IndexedPeripheralBuilder(_archlib.ArchAVR_USART, cfg_builder)


#========================================================================================
#SPI configuration

def _spi_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'pin_select':
        cfg.pin_select = _corelib.str_to_id(yml_val)
    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_spi_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_SPIConfig, _spi_convertor)
    return IndexedPeripheralBuilder(_archlib.ArchAVR_SPI, cfg_builder)


#========================================================================================
#TWI configuration

def _twi_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'ps_factors':
        cfg.ps_factors = yml_val
    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_twi_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchAVR_TWIConfig, _twi_convertor)
    return IndexedPeripheralBuilder(_archlib.ArchAVR_TWI, cfg_builder)


#========================================================================================

class AVR_BaseDevice(_archlib.ArchAVR_Device):
    """Specialisation of BaseDevicer for the device models using the AVR architecture.
    """

    _NVMs_ = { 'Flash': _corelib.Core.NVM.Flash,
               'Fuses': _corelib.Core.NVM.Fuses,
               'EEPROM': _archlib.ArchAVR_Core.ArchAVR_NVM.EEPROM,
               'Lockbit': _archlib.ArchAVR_Core.ArchAVR_NVM.Lockbit,
    }

    def __init__(self, dev_descriptor, builder):
        super().__init__(builder.get_device_config())


#========================================================================================

class AVR_DeviceBuilder(DeviceBuilder):
    """Specialisation of DeviceBuilder for the device models using the AVR architecture.
    """

    #Dictionary for the builder getters for AVR peripherals
    _per_builder_getters = {
        'CPU': None,
        'CPUINT': _get_intctrl_builder,
        'SLPCTRL': _get_slpctrl_builder,
        'FUSES_48': _get_fuses_builder,
        'FUSES_88_168': _get_fuses_builder,
        'FUSES_328': _get_fuses_builder,
        'NVMCTRL': _get_nvmctrl_builder,
        'MISC': _get_misc_builder,
        'PORT': _get_port_builder,
        'EXTINT': _get_extint_builder,
        'TC0': _get_timer_builder,
        'TC1': _get_timer_builder,
        'TC2': _get_timer_builder,
        'VREF': _get_vref_builder,
        'ADC': _get_adc_builder,
        'ACP': _get_acp_builder,
        'USART': _get_usart_builder,
        'SPI': _get_spi_builder,
        'TWI': _get_twi_builder,
    }

    def _build_core_config(self, dev_desc):
        cfg = _archlib.ArchAVR_CoreConfig()
        cfg.attributes = get_core_attributes(dev_desc)
        cfg.iostart, cfg.ioend = dev_desc.mem_spaces['data'].segments['io']
        cfg.ramstart, cfg.ramend = dev_desc.mem_spaces['data'].segments['ram']
        cfg.dataend = dev_desc.mem_spaces['data'].memend
        cfg.flashend = dev_desc.mem_spaces['flash'].memend
        cfg.eepromend = dev_desc.mem_spaces['eeprom'].memend
        cfg.eind = dev_desc.reg_address('CPU/EIND', _corelib.INVALID_REGISTER)
        cfg.rampz = dev_desc.reg_address('CPU/RAMPZ', _corelib.INVALID_REGISTER)
        cfg.vector_size = dev_desc.interrupt_map.vector_size
        cfg.fusesize = dev_desc.fuses['size']
        cfg.fuses = bytes(dev_desc.fuses['factory_values'])
        cfg.flash_page_size = dev_desc.mem_spaces['flash'].page_size
        return cfg

    def _build_device_config(self, dev_desc, core_cfg):
        cfg = _archlib.ArchAVR_DeviceConfig(core_cfg)
        cfg.name = dev_desc.name
        cfg.pins = dev_desc.pins
        return cfg

    def _get_peripheral_builder(self, per_class):
        if per_class not in self._per_builder_getters:
            raise DeviceBuildError('Unknown peripheral class: ' + per_class)
        builder_getter = self._per_builder_getters[per_class]
        if builder_getter is not None:
            per_builder = builder_getter()
            return per_builder
        else:
            return None
