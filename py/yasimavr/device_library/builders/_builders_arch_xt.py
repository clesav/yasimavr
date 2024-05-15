# _builders_arch_xt.py
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

from ...lib import core as _corelib
from ...lib import arch_xt as _archlib
from ._base import (PeripheralBuilder, PeripheralConfigBuilder,
                    IndexedPeripheralBuilder, LetteredPeripheralBuilder, DummyPeripheralBuilder,
                    DeviceBuilder, DeviceBuildError,
                    get_core_attributes, convert_to_regbit)


def base_config_builder(per_descriptor):
    return per_descriptor.reg_base


#========================================================================================
#Interrupt management configuration

def _get_cpuint_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_IntCtrlConfig)
    return PeripheralBuilder(_archlib.ArchXT_IntCtrl, cfg_builder)


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
#CLK controller configuration

def _get_clkctrl_builder():
    return DummyPeripheralBuilder(_corelib.IOCTL_CLOCK)


#========================================================================================
#RST controller configuration

def _get_rstctrl_builder():
    return PeripheralBuilder(_archlib.ArchXT_ResetCtrl, base_config_builder)


#========================================================================================
#NVM controller configuration

def _get_nvmctrl_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_NVMConfig)
    return PeripheralBuilder(_archlib.ArchXT_NVM, cfg_builder)


#========================================================================================
#Misc controller configuration

def _get_misc_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_MiscConfig)
    return PeripheralBuilder(_archlib.ArchXT_MiscRegCtrl, cfg_builder)


#========================================================================================
#Port management configuration

def _get_port_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_PortConfig)
    return LetteredPeripheralBuilder(_archlib.ArchXT_Port, cfg_builder)


#========================================================================================
#Portmuxconfiguration

def _get_portmux_builder():
    return DummyPeripheralBuilder(_corelib.IOCTL_PORTMUX)


#========================================================================================
#TCA management configuration

def _tca_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'ivs_cmp':
        cfg.ivs_cmp = [per_desc.device.interrupt_map.vectors.index(v) for v in yml_val]
    elif attr == 'version':
        cfg.version = _archlib.ArchXT_TimerAConfig.Version[yml_val]
    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_tca_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_TimerAConfig, _tca_convertor)
    return PeripheralBuilder(_archlib.ArchXT_TimerA, cfg_builder)


#========================================================================================
#TCB management configuration

def _get_tcb_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_TimerBConfig)
    return IndexedPeripheralBuilder(_archlib.ArchXT_TimerB,cfg_builder)


#========================================================================================
#RTC management configuration

def _rtc_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'clocks':
        py_clocks = []
        for yml_clk in yml_val:
            clksel_cfg = _archlib.ArchXT_RTCConfig.clksel_config_t()
            clksel_cfg.reg_value = yml_clk[0]
            clksel_cfg.source = _archlib.ArchXT_RTCConfig.RTC_ClockSource[yml_clk[1]]
            py_clocks.append(clksel_cfg)

        cfg.clocks = py_clocks

    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_rtc_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_RTCConfig, _rtc_convertor)
    return PeripheralBuilder(_archlib.ArchXT_RTC, cfg_builder)


#========================================================================================
#VREF management configuration

def _vref_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'channels':
        py_chans = []
        for index, item in enumerate(yml_val):
            chan_cfg = _archlib.ArchXT_VREFConfig.channel_t()
            chan_cfg.index = index
            chan_cfg.rb_select = convert_to_regbit(item['rb_select'], per_desc)

            ref_cfg_list = []
            for reg_value, chan_ref in enumerate(item['references']):
                chan_ref_cfg = _archlib.ArchXT_VREFConfig.reference_config_t()
                chan_ref_cfg.reg_value = reg_value

                if chan_ref == 'AVCC':
                    chan_ref_cfg.source = _corelib.VREF.Source.AVCC
                elif chan_ref == 'AREF':
                    chan_ref_cfg.source = _corelib.VREF.Source.AREF
                elif chan_ref is not None:
                    chan_ref_cfg.source = _corelib.VREF.Source.Internal
                    chan_ref_cfg.level = float(chan_ref)

                ref_cfg_list.append(chan_ref_cfg)

            chan_cfg.references = ref_cfg_list
            py_chans.append(chan_cfg)

        cfg.channels = py_chans

    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_vref_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_VREFConfig, _vref_convertor)
    return PeripheralBuilder(_archlib.ArchXT_VREF, cfg_builder)


#========================================================================================
#ADC management configuration

def _adc_convertor(cfg, attr, yml_val, per_desc):
    if attr == 'channels':
        py_chans = []
        for reg_value, item in yml_val.items():
            chan_cfg = _corelib.ADC.channel_config_t()
            chan_cfg.reg_value = reg_value
            if isinstance(item, list):
                chan_type = item[0]
                if len(item) >= 2:
                    chan_cfg.pin_p = _corelib.str_to_id(item[1])
                if len(item) >= 3:
                    chan_cfg.pin_n = _corelib.str_to_id(item[2])
            else:
                chan_type = item

            chan_cfg.type = _corelib.ADC.Channel[chan_type]
            py_chans.append(chan_cfg)

        cfg.channels = py_chans

    elif attr == 'references':
        py_refs = []
        for reg_value, item in yml_val.items():
            ref_cfg = _archlib.ArchXT_ADCConfig.reference_config_t()
            ref_cfg.reg_value = reg_value
            ref_cfg.source = _corelib.VREF.Source[item]
            py_refs.append(ref_cfg)

        cfg.references = py_refs

    elif attr == 'clk_ps_factors':
        cfg.clk_ps_factors = yml_val

    elif attr == 'init_delays':
        cfg.init_delays = yml_val

    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_adc_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_ADCConfig, _adc_convertor)
    return IndexedPeripheralBuilder(_archlib.ArchXT_ADC, cfg_builder)


#========================================================================================
#ACP management configuration

def _acp_convertor(cfg, attr, yml_val, per_desc):
    if attr in ('pos_channels', 'neg_channels'):
        py_chans = []
        for reg_value, item in yml_val.items():
            chan_cfg = _corelib.ACP.channel_config_t()
            chan_cfg.reg_value = reg_value
            if isinstance(item, list):
                chan_type = item[0]
                if len(item) >= 2:
                    chan_cfg.pin = _corelib.str_to_id(item[1])
            else:
                chan_type = item

            chan_cfg.type = _corelib.ACP.Channel[chan_type]
            py_chans.append(chan_cfg)

        setattr(cfg, attr, py_chans)

    else:
        raise Exception('Converter not implemented for ' + attr)


def _get_acp_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_ACPConfig, _acp_convertor)
    return IndexedPeripheralBuilder(_archlib.ArchXT_ACP, cfg_builder)


#========================================================================================
#USART management configuration

def _get_usart_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_USARTConfig)
    return IndexedPeripheralBuilder(_archlib.ArchXT_USART, cfg_builder)


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


def _get_spi_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_SPIConfig, _spi_convertor)
    return IndexedPeripheralBuilder(_archlib.ArchXT_SPI, cfg_builder)


#========================================================================================
#TWI management configuration

def _get_twi_builder():
    cfg_builder = PeripheralConfigBuilder(_archlib.ArchXT_TWIConfig)
    return IndexedPeripheralBuilder(_archlib.ArchXT_TWI, cfg_builder)


#========================================================================================
#FUSES configuration

def _get_fuses_builder():
    return PeripheralBuilder(_archlib.ArchXT_Fuses, base_config_builder)


#========================================================================================
#USERROW configuration

def _get_userrow_builder():
    return PeripheralBuilder(_archlib.ArchXT_USERROW, base_config_builder)


#========================================================================================

class XT_BaseDevice(_archlib.ArchXT_Device):
    """Specialisation of BaseDevice for the device models using the XT architecture.
    """

    _NVMs_ = { 'Flash': _corelib.Core.NVM.Flash,
               'Fuses': _corelib.Core.NVM.Fuses,
               'EEPROM': _archlib.ArchXT_Core.ArchXT_NVM.EEPROM,
               'USERROW': _archlib.ArchXT_Core.ArchXT_NVM.USERROW,
    }

    def __init__(self, dev_descriptor, builder):
        super().__init__(builder.get_device_config())


#========================================================================================

class XT_DeviceBuilder(DeviceBuilder):
    """Specialisation of DeviceBuilder for the device models using the XT architecture.
    """

    #Dictionary for the builder getters for XT core peripherals
    _per_builder_getters = {
        'CPU': None,
        'CPUINT': _get_cpuint_builder,
        'SLPCTRL': _get_slpctrl_builder,
        'CLKCTRL': _get_clkctrl_builder,
        'RSTCTRL': _get_rstctrl_builder,
        'NVMCTRL': _get_nvmctrl_builder,
        'MISC': _get_misc_builder,
        'PORT': _get_port_builder,
        'PORTMUX': _get_portmux_builder,
        'RTC': _get_rtc_builder,
        'TCA': _get_tca_builder,
        'TCB': _get_tcb_builder,
        'VREF': _get_vref_builder,
        'ADC': _get_adc_builder,
        'ACP': _get_acp_builder,
        'USART': _get_usart_builder,
        'SPI': _get_spi_builder,
        'TWI': _get_twi_builder,
        'FUSES': _get_fuses_builder,
        'USERROW_64': _get_userrow_builder,
        'USERROW_32': _get_userrow_builder
    }

    def _build_core_config(self, dev_desc):
        cfg = _archlib.ArchXT_CoreConfig()

        cfg.attributes = get_core_attributes(dev_desc)

        cfg.iostart, cfg.ioend = dev_desc.mem_spaces['data'].segments['io']
        cfg.ramstart, cfg.ramend = dev_desc.mem_spaces['data'].segments['ram']
        cfg.flashstart_ds, cfg.flashend_ds = dev_desc.mem_spaces['data'].segments['flash']
        cfg.eepromstart_ds, cfg.eepromend_ds = dev_desc.mem_spaces['data'].segments['eeprom']

        cfg.dataend = dev_desc.mem_spaces['data'].memend
        cfg.flashend = dev_desc.mem_spaces['flash'].memend
        cfg.eepromend = dev_desc.mem_spaces['eeprom'].memend
        cfg.userrowend = dev_desc.mem_spaces['userrow'].memend

        cfg.eind = dev_desc.reg_address('CPU/EIND', _corelib.INVALID_REGISTER)
        cfg.rampz = dev_desc.reg_address('CPU/RAMPZ', _corelib.INVALID_REGISTER)

        cfg.fusesize = dev_desc.fuses['size']
        cfg.fuses = bytes(dev_desc.fuses['factory_values'])

        cfg.flash_page_size = dev_desc.mem_spaces['flash'].page_size

        return cfg

    def _build_device_config(self, dev_desc, core_cfg):
        cfg = _archlib.ArchXT_DeviceConfig(core_cfg)
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
