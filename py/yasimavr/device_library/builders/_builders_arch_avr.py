# _builders_arch_avr.py
#
# Copyright 2022 Clement Savergne <csavergne@yahoo.com>
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
                    get_core_attributes)


#========================================================================================
#Intctrl register configuration

def _get_intctrl_builder():
    def _get_vector_count(per_desc):
        return len(per_desc.device.interrupt_map.vectors)

    return PeripheralBuilder(_archlib.AVR_ArchAVR_Interrupt, _get_vector_count)


#========================================================================================
#Sleep controller configuration

def _slpctrl_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'modes':
        py_modes = []
        for mode_reg_value, mode_name in yml_val.items():
            mode_cfg = _corelib.AVR_SleepConfig.mode_config_t()
            mode_cfg.reg_value = mode_reg_value
            mode_cfg.mode = _corelib.AVR_SleepMode[mode_name]

            int_mask = per_desc.device.interrupt_map.sleep_mask[mode_name]
            mode_cfg.int_mask = int_mask + [0] * (16 - len(int_mask)) #padding to length=16

            py_modes.append(mode_cfg)

        cfg.modes = py_modes

    else:
        raise Exception('Converter not implemented for ' + attr)

def _get_slpctrl_builder():
    cfg_builder = PeripheralConfigBuilder(_corelib.AVR_SleepConfig, _slpctrl_convertor)
    return PeripheralBuilder(_corelib.AVR_SleepController, cfg_builder)


#========================================================================================
#Misc register configuration

def _misc_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'gpior':
        cfg.gpior = [ per_desc.reg_address(v) for v in yml_val ]
    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_misc_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.AVR_ArchAVR_Misc_Config, _misc_convertor)
    return PeripheralBuilder(_archlib.AVR_ArchAVR_MiscRegCtrl, cfg_builder)


#========================================================================================
#General I/O port configuration

def _get_port_config(per_desc):
    cfg = _archlib.AVR_ArchAVR_PortConfig()
    cfg.name = per_desc.name[-1]
    cfg.reg_port = per_desc.reg_address('PORT')
    cfg.reg_pin = per_desc.reg_address('PIN')
    cfg.reg_dir = per_desc.reg_address('DDR')
    return cfg


def _get_port_builder():
    return PeripheralBuilder(_archlib.AVR_ArchAVR_Port, _get_port_config)


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
    cfg_builder = PeripheralConfigBuilder(_archlib.AVR_ArchAVR_ExtIntConfig, _extint_convertor)
    return PeripheralBuilder(_archlib.AVR_ArchAVR_ExtInt, cfg_builder)


#========================================================================================
#Timers/counters configuration

def _timer_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'clocks':
        py_clocks = []
        for clk_opt in yml_val:
            clk_cfg = _archlib.AVR_ArchAVR_TimerConfig.clock_config_t()
            clk_cfg.reg_value = clk_opt[0]
            clk_cfg.source = _archlib.AVR_ArchAVR_TimerConfig.ClockSource[clk_opt[1]]
            clk_cfg.div = clk_opt[2]
            py_clocks.append(clk_cfg)
        cfg.clocks = py_clocks

    elif attr == 'modes':
        py_modes = []
        for mode_opt in yml_val:
            mode_cfg = _archlib.AVR_ArchAVR_TimerConfig.mode_config_t()
            mode_cfg.reg_value = mode_opt[0]
            mode_cfg.mode = _archlib.AVR_ArchAVR_TimerConfig.Mode[mode_opt[1]]
            py_modes.append(mode_cfg)
        cfg.modes = py_modes

    elif attr == 'int_ovf':
        cfg.int_ovf.vector = per_desc.device.interrupt_map.vectors.index(yml_val[0])
        cfg.int_ovf.bit = yml_val[1]

    elif attr == 'int_ocra':
        cfg.int_ocra.vector = per_desc.device.interrupt_map.vectors.index(yml_val[0])
        cfg.int_ocra.bit = yml_val[1]

    elif attr == 'int_ocrb':
        cfg.int_ocrb.vector = per_desc.device.interrupt_map.vectors.index(yml_val[0])
        cfg.int_ocrb.bit = yml_val[1]

    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_timer_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.AVR_ArchAVR_TimerConfig, _timer_convertor)
    return IndexedPeripheralBuilder(_archlib.AVR_ArchAVR_Timer, cfg_builder)


#========================================================================================
#ADC configuration

def _adc_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'channels':
        py_channels = []
        for reg_value, item in yml_val.items():
            chan_cfg = _corelib.AVR_IO_ADC.channel_config_t()
            chan_cfg.reg_value = reg_value
            if isinstance(item, list):
                chan_cfg.type = _corelib.AVR_IO_ADC.Channel[item[0]]
                chan_cfg.pin_p = _corelib.str_to_id(item[1])
                chan_cfg.pin_n = _corelib.str_to_id(item[2]) if len(item) > 2 else 0
                chan_cfg.gain = int(item[3]) if len(item) > 3 else 1
            else:
                chan_cfg.type = _corelib.AVR_IO_ADC.Channel[item]
                chan_cfg.pin_p = 0
                chan_cfg.pin_n = 0
                chan_cfg.gain = 1

            py_channels.append(chan_cfg)

        cfg.channels = py_channels

    elif attr == 'references':
        py_refs = []
        for reg_value, item in yml_val.items():
            ref_cfg = _archlib.AVR_ArchAVR_ADC_Config.reference_config_t()
            ref_cfg.reg_value = reg_value
            ref_cfg.source = _corelib.AVR_IO_VREF.Source[item]
            py_refs.append(ref_cfg)

        cfg.references = py_refs

    elif attr == 'clk_ps_factors':
        cfg.clk_ps_factors = yml_val

    elif attr == 'triggers':
        py_triggers = []
        for reg_value, item in yml_val.items():
            trig_cfg = _archlib.AVR_ArchAVR_ADC_Config.trigger_config_t()
            trig_cfg.reg_value = reg_value
            trig_cfg.trig_type = _archlib.AVR_ArchAVR_ADC_Config.Trigger[item]
            py_triggers.append(trig_cfg)

        cfg.triggers = py_triggers

    elif attr == 'int_vector':
        cfg.int_vector = per_desc.device.interrupt_map.vectors.index(yml_val)

    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_adc_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.AVR_ArchAVR_ADC_Config, _adc_convertor)
    return IndexedPeripheralBuilder(_archlib.AVR_ArchAVR_ADC, cfg_builder)


#========================================================================================
#ACP configuration

def _acp_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'mux_pins':
        py_pins = []
        for reg_value, item in yml_val.items():
            mux_cfg = _archlib.AVR_ArchAVR_ACP_Config.mux_config_t()
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
    cfg_builder = PeripheralConfigBuilder(_archlib.AVR_ArchAVR_ACP_Config, _acp_convertor)
    return IndexedPeripheralBuilder(_archlib.AVR_ArchAVR_ACP, cfg_builder)


#========================================================================================
#Reference voltage configuration

def _get_vref_bandgap(per_desc):
    return per_desc.class_descriptor.config['bandgap']


def _get_vref_builder():
    return PeripheralBuilder(_archlib.AVR_ArchAVR_VREF, _get_vref_bandgap)


#========================================================================================
#USART configuration

def _usart_convertor(cfg, attr, yml_val, per_desc):
    if attr in ('rxc_vector', 'txc_vector', 'txe_vector'):
        v = per_desc.device.interrupt_map.vectors.index(yml_val)
        setattr(cfg, attr, v)
    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_usart_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.AVR_ArchAVR_USART_Config, _usart_convertor)
    return IndexedPeripheralBuilder(_archlib.AVR_ArchAVR_USART, cfg_builder)


#========================================================================================

class AVR_DeviceBuilder(DeviceBuilder):

    #Dictionary for the builder getters for AVR peripherals
    _per_builder_getters = {
        'CPUINT': _get_intctrl_builder,
        'SLPCTRL': _get_slpctrl_builder,
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
    }

    def _build_core_config(self, dev_desc):
        cfg = _archlib.AVR_ArchAVR_CoreConfig()
        cfg.attributes = get_core_attributes(dev_desc)
        cfg.iostart, cfg.ioend = dev_desc.mem_spaces['data'].segments['io']
        cfg.ramstart, cfg.ramend = dev_desc.mem_spaces['data'].segments['ram']
        cfg.dataend = dev_desc.mem_spaces['data'].memend
        cfg.flashend = dev_desc.mem_spaces['flash'].memend
        cfg.eepromend = dev_desc.mem_spaces['eeprom'].memend
        cfg.eind = dev_desc.reg_address('CPU.EIND', _corelib.INVALID_REGISTER)
        cfg.rampz = dev_desc.reg_address('CPU.RAMPZ', _corelib.INVALID_REGISTER)
        cfg.vector_size = dev_desc.interrupt_map.vector_size
        return cfg

    def _build_device_config(self, dev_desc, core_cfg):
        cfg = _archlib.AVR_ArchAVR_DeviceConfig(core_cfg)
        cfg.name = dev_desc.name
        cfg.pins = dev_desc.pins
        return cfg

    def _get_peripheral_builder(self, per_class):
        builder_getter = self._per_builder_getters.get(per_class, None)
        if builder_getter is None:
            raise DeviceBuildError('Unknown peripheral class: ' + per_class)
        per_builder = builder_getter()
        return per_builder

    @classmethod
    def build_device(cls, model, dev_class):
        if not issubclass(dev_class, _archlib.AVR_ArchAVR_Device):
            raise TypeError('The device class must be a ArchAVR_Device subclass')

        return super().build_device(model, dev_class)
