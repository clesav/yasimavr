/*
 * arch_avr_timer.sip
 *
 *  Copyright 2021 Clement Savergne <csavergne@yahoo.com>

    This file is part of yasim-avr.

    yasim-avr is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    yasim-avr is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.
 */

//=======================================================================================

%ModuleCode

#include "arch_avr_timer.h"

const ctlreq_id_t CTLREQ_TMR_GET_EXTCLK_HOOK = AVR_CTLREQ_TMR_GET_EXTCLK_HOOK;
const ctlreq_id_t CTLREQ_TMR_GET_CAPT_HOOK = AVR_CTLREQ_TMR_GET_CAPT_HOOK;

%End

const ctlreq_id_t CTLREQ_TMR_GET_EXTCLK_HOOK;
const ctlreq_id_t CTLREQ_TMR_GET_CAPT_HOOK;


//=======================================================================================

struct ArchAVR_TimerConfig {
%TypeHeaderCode
#include "arch_avr_timer.h"
%End

    enum COM {
        COM_NoChange        /PyName=NoChange/,
        COM_Toggle          /PyName=Toggle/,
        COM_Clear           /PyName=Clear/,
        COM_Set             /PyName=Set/,
        COM_ToggleA         /PyName=ToggleA/,
    };

    enum Top {
        Top_OnMax           /PyName=OnMax/,
        Top_OnFixed         /PyName=OnFixed/,
        Top_OnCompA         /PyName=OnCompA/,
        Top_OnIC            /PyName=OnIC/,
    };

    enum OCR {
        OCR_Unbuffered      /PyName=Unbuffered/,
        OCR_UpdateOnTop     /PyName=UpdateOnTop/,
        OCR_UpdateOnBottom  /PyName=UpdateOnBottom/,
    };

    enum OVF {
        OVF_SetOnMax        /PyName=SetOnMax/,
        OVF_SetOnTop        /PyName=SetOnTop/,
        OVF_SetOnBottom     /PyName=SetOnBottom/,
    };

    struct clock_config_t : base_reg_config_t {
        TimerCounter::TickSource source;
        unsigned long div;
    };

    struct vector_config_t {
        int_vect_t num;
        unsigned char bit /PyInt/;
    };

    struct OC_config_t : base_reg_config_t {
        reg_addr_t reg_oc;
        vector_config_t vector;
        regbit_t rb_mode;
        regbit_t rb_force;
    };

    struct COM_config_t : base_reg_config_t {
        ArchAVR_TimerConfig::COM up;
        ArchAVR_TimerConfig::COM down;
        ArchAVR_TimerConfig::COM bottom;
        ArchAVR_TimerConfig::COM top;
    };

    struct mode_config_t : base_reg_config_t {
        ArchAVR_TimerConfig::OVF ovf;
        ArchAVR_TimerConfig::Top top;
        unsigned int fixed_top_exp;
        ArchAVR_TimerConfig::OCR ocr;
        bool double_slope;
        bool disable_foc;
        unsigned int com_variant;
    };

    bool is_16bits;
    std::vector<ArchAVR_TimerConfig::clock_config_t> clocks;
    std::vector<ArchAVR_TimerConfig::mode_config_t> modes;
    std::vector<ArchAVR_TimerConfig::OC_config_t> oc_channels;
    regbit_t rb_clock;
    regbit_compound_t rbc_mode;
    reg_addr_t reg_cnt;
    reg_addr_t reg_icr;
    reg_addr_t reg_int_enable;
    reg_addr_t reg_int_flag;
    ArchAVR_TimerConfig::vector_config_t vect_ovf;
    ArchAVR_TimerConfig::vector_config_t vect_icr;

    SIP_PYLIST com_modes {
    %GetCode
        PyObject* l = PyList_New(sipCpp->com_modes.size());
        ArchAVR_TimerConfig::COM_variant_t* COM_variant;
        for (size_t i = 0; i < sipCpp->com_modes.size(); ++i) {
            COM_variant = &sipCpp->com_modes[i];
            PyObject* ll = PyList_New(COM_variant->size());
            PyList_SET_ITEM(l, i, ll);
            for (size_t j = 0; j < COM_variant->size(); ++j) {
                ArchAVR_TimerConfig::COM_config_t* cfg = &(*COM_variant)[j];
                PyObject* pycfg = sipConvertFromType(cfg,
                                                     sipType_ArchAVR_TimerConfig_COM_config_t,
                                                     SIP_NULLPTR);
                PyList_SET_ITEM(ll, j, pycfg);
            }
        }
        sipPy = l;
    %End
    %SetCode
        PyObject* iter1 = PyObject_GetIter(sipPy);
        if (iter1) {
            std::vector<std::vector<ArchAVR_TimerConfig::COM_config_t>> com_modes;
            PyObject* item1;
            while ((item1 = PyIter_Next(iter1)) != NULL) {
                PyObject* iter2 = PyObject_GetIter(item1);
                if (iter2) {
                    PyObject* item2;
                    std::vector<ArchAVR_TimerConfig::COM_config_t> com_variant;
                    while ((item2 = PyIter_Next(iter2)) != NULL) {
                        void* sipCfg = sipForceConvertToType(item2,
                                                             sipType_ArchAVR_TimerConfig_COM_config_t,
                                                             SIP_NULLPTR, SIP_NOT_NONE, SIP_NULLPTR, &sipErr);
                        if (sipErr) break;
                        com_variant.push_back(*reinterpret_cast<ArchAVR_TimerConfig::COM_config_t*>(sipCfg));
                    }
                    Py_DECREF(iter2);
                    if (sipErr) break;
                    com_modes.push_back(com_variant);
                } else {
                    sipErr = 1;
                }
                Py_DECREF(item1);
                if (sipErr) break;
            }
            Py_DECREF(iter1);
            if (!sipErr)
                sipCpp->com_modes = com_modes;
        } else {
            sipErr = 1;
        }

        if (sipErr)
            PyErr_SetString(PyExc_TypeError, "Invalid type");
    %End
    };

};


class ArchAVR_Timer : public Peripheral, public SignalHook /NoDefaultCtors/ {
%TypeHeaderCode
#include "arch_avr_timer.h"
%End

public:

    enum SignalId /BaseType=IntEnum/ {
        Signal_OVF              /PyName=OVF/,
        Signal_CompMatch        /PyName=CompMatch/,
        Signal_CompOutput       /PyName=CompOutput/,
        Signal_Capt             /PyName=Capt/
    };

    ArchAVR_Timer(int, const ArchAVR_TimerConfig& /KeepReference/);

    virtual bool init(Device&);
    virtual void reset();
    virtual bool ctlreq(ctlreq_id_t, ctlreq_data_t*);
    virtual uint8_t ioreg_read_handler(reg_addr_t, uint8_t);
    virtual void ioreg_write_handler(reg_addr_t, const ioreg_write_t&);
    virtual void raised(const signal_data_t&, int);

};
