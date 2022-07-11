# convertors_m0.py
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
This module applies to devices ATmega80x/160x/320x/480x
'''

import yasimavr.lib.core as _corelib
import yasimavr.lib.arch_m0 as _archlib
from .configbuilder import _PeripheralConfigBuilder, get_core_attributes


#========================================================================================
#Core configuration

def get_core_config(dev_desc):
    cfg = _archlib.AVR_ArchMega0_CoreConfig()
    
    cfg.attributes = get_core_attributes(dev_desc)
    
    cfg.iostart, cfg.ioend = dev_desc.mem_spaces['data'].segments['io']
    cfg.ramstart, cfg.ramend = dev_desc.mem_spaces['data'].segments['ram']
    cfg.flashstart_ds, cfg.flashend_ds = dev_desc.mem_spaces['data'].segments['flash']
    cfg.eepromstart_ds, cfg.eepromend_ds = dev_desc.mem_spaces['data'].segments['eeprom']
    
    cfg.dataend = dev_desc.mem_spaces['data'].memend
    cfg.flashend = dev_desc.mem_spaces['flash'].memend
    cfg.eepromend = dev_desc.mem_spaces['eeprom'].memend
    cfg.userrowend = dev_desc.mem_spaces['userrow'].memend
    
    cfg.eind = dev_desc.peripherals['CPU'].reg_address('EIND', 0)
    cfg.rampz = dev_desc.peripherals['CPU'].reg_address('RAMPZ', 0)
    
    cfg.vector_size = dev_desc.interrupt_map.vector_size
    
    cfg.fusesize = dev_desc.fuses['size']
    cfg.fuses = bytes(dev_desc.fuses['factory_values'])
    
    return cfg


#========================================================================================
#Device configuration

def get_dev_config(dev_desc, core_cfg):
    cfg = _archlib.AVR_ArchMega0_DeviceConfig()
    cfg.name = dev_desc.name
    cfg.core = core_cfg
    cfg.pins = dev_desc.pins
    return cfg


#========================================================================================
#Interrupt management configuration

_CpuIntConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_IntCtrl_Config)

def get_cpuint_config(per_desc):
    return _CpuIntConfigBuilder(per_desc)


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

_SleepConfigBuilder = _PeripheralConfigBuilder(_corelib.AVR_SleepConfig, _slpctrl_convertor)

def get_sleep_config(per_desc):
    return _SleepConfigBuilder(per_desc)


#========================================================================================
#NVM controller configuration

_NVMCtrlConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_NVM_Config)

def get_nvmctrl_config(per_desc):
    return _NVMCtrlConfigBuilder(per_desc)

#========================================================================================
#Misc controller configuration

_MISC_ConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_Misc_Config)

def get_misc_config(per_desc):
    return _MISC_ConfigBuilder(per_desc)


#========================================================================================
#Port management configuration

_PortConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_PortConfig)

def get_port_config(per_desc):
    return _PortConfigBuilder(per_desc)


#========================================================================================
#TCA management configuration

def _tca_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'ivs_cmp':
        for i, yml_iv in enumerate(yml_val):
            cfg.ivs_cmp[i] = per_desc.device.interrupt_map.vectors.index(yml_iv)
        cfg._proxy_touch('ivs_cmp')
    else:
        raise Exception('Converter not implemented for ' + attr)

_TCA_ConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_TimerA_Config, _tca_convertor)

def get_tca_config(per_desc):
    return _TCA_ConfigBuilder(per_desc)


#========================================================================================
#TCB management configuration

_TCB_ConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_TimerB_Config)

def get_tcb_config(per_desc):
    return _TCB_ConfigBuilder(per_desc)


#========================================================================================
#RTC management configuration

def _rtc_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'clocks':
        py_clocks = []
        for yml_clk in yml_val:
            clksel_cfg = _archlib.AVR_ArchMega0_RTC_Config.clksel_config_t()
            clksel_cfg.reg_value = yml_clk[0]
            clksel_cfg.source = _archlib.AVR_ArchMega0_RTC_Config.RTC_ClockSource[yml_clk[1]]
            py_clocks.append(clksel_cfg)
        
        cfg.clocks = py_clocks
        
    else:
        raise Exception('Converter not implemented for ' + attr)

_RTC_ConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_RTC_Config, _rtc_convertor)

def get_rtc_config(per_desc):
    return _RTC_ConfigBuilder(per_desc)


#========================================================================================
#ADC management configuration

def _adc_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'channels':
        py_chans = []
        for reg_value, item in yml_val.items():
            chan_cfg = _corelib.AVR_IO_ADC.channel_config_t()
            chan_cfg.reg_value = reg_value
            if isinstance(item, list):
                chan_type = item[0]
                if len(item) >= 2:
                    chan_cfg.pin_p = _corelib.str_to_id(item[1])
                if len(item) >= 3:
                    chan_cfg.pin_n = _corelib.str_to_id(item[2])
            else:
                chan_type = item
            
            chan_cfg.type = _corelib.AVR_IO_ADC.Channel[chan_type]
            py_chans.append(chan_cfg)
            
        cfg.channels = py_chans
    
    elif attr == 'references':
        py_refs = []
        for reg_value, item in yml_val.items():
            ref_cfg = _archlib.AVR_ArchMega0_ADC_Config.reference_config_t()
            ref_cfg.reg_value = reg_value
            ref_cfg.source = _corelib.AVR_IO_VREF.Source[item]
            py_refs.append(ref_cfg)
        
        cfg.references = py_refs
    
    elif attr == 'clk_ps_factors':
        cfg.clk_ps_factors = yml_val
    
    elif attr == 'init_delays':
        cfg.init_delays = yml_val
    
    else:
        raise Exception('Converter not implemented for ' + attr)

_ADC_ConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_ADC_Config, _adc_convertor)

def get_adc_config(per_desc):
    return _ADC_ConfigBuilder(per_desc)


#========================================================================================
#USART management configuration

_USART_ConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_USART_Config)

def get_usart_config(per_desc):
    return _USART_ConfigBuilder(per_desc)

#========================================================================================
#SPI management configuration

def _spi_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'pin_select':
        if yml_val is None:
            cfg.pin_select = 0
        else:
            cfg.pin_select = _corelib.str_to_id(yml_val)
    
    else:
        raise Exception('Converter not implemented for ' + attr)

_SPI_ConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_SPI_Config, _spi_convertor)

def get_spi_config(per_desc):
    return _SPI_ConfigBuilder(per_desc)

#========================================================================================
#TWI management configuration

_TWI_ConfigBuilder = _PeripheralConfigBuilder(_archlib.AVR_ArchMega0_TWI_Config)

def get_twi_config(per_desc):
    return _TWI_ConfigBuilder(per_desc)
