======================
XT Architecture models
======================

.. include:: ../globals.rst


This section describes the simulation models defined for the XT architecture
They are accessible by the following import :

.. code-block:: py

   import yasimavr.lib.arch_xt


.. sip:specification:: yasimavr.lib.arch_xt



Top level models
################


.. sip:class:: ArchXT_Device
   :members:


.. sip:class:: ArchXT_Core
   :members:



Controllers
###########


.. _xt_clkctrl_ref:

Clock Controller
----------------

.. sip:class:: ArchXT_ClkCtrl
   :no-header:

Control Request Codes definition
********************************

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_CLK_SET_EXTCLK`

Reference
*********

.. sip:class:: ArchXT_ClkCtrl
   :no-doc:
   :members:


.. _xt_fuse_ref:

Fuses Controller
----------------

.. sip:class:: ArchXT_Fuses
   :no-header:

Reference
*********

.. sip:class:: ArchXT_Fuses
   :no-doc:
   :members: undoc


.. _xt_cpuint_ref:

Interrupt Controller
--------------------

.. sip:class:: ArchXT_IntCtrl
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_IntCtrlConfig
   :members:

Signalling
**********

See :py:class:`yasimavr.lib.core.InterruptController` for signal ids.

Reference
*********

.. sip:class:: ArchXT_IntCtrl
   :no-doc:
   :members: undoc


.. _xt_miscreg_ref:

Misc Register Controller
------------------------

.. sip:class:: ArchXT_MiscRegCtrl
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_MiscConfig
   :members:

Reference
*********

.. sip:class:: ArchXT_MiscRegCtrl
   :no-doc:
   :members: undoc


.. _xt_nvmctrl_ref:

NVM Controller
--------------

.. sip:class:: ArchXT_NVM
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_NVMConfig
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_NVM_REQUEST`

Reference
*********

.. sip:class:: ArchXT_NVM
   :no-doc:
   :members:


.. _xt_rstctrl_ref:

Reset Controller
----------------

.. sip:class:: ArchXT_ResetCtrl
   :no-header:

Reference
*********

.. sip:class:: ArchXT_ResetCtrl
   :no-doc:
   :members:



Analog peripherals
##################


.. _xt_acp_ref:

Analog Comparator
-----------------

.. sip:class:: ArchXT_ACP
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_ACPConfig
   :members:

Control Request Codes definition
********************************

.. sip:macro-constant:: CTLREQ_ACP_GET_DAC  AVR_CTLREQ_ACP_GET_DAC

Signalling
**********

See :py:class:`yasimavr.lib.core.ACP.SignalId` for signal ids.

Reference
*********

.. sip:class:: ArchXT_ACP
   :no-doc:
   :members:


.. _xt_adc1_ref:

Analog-to-Digital Converter (1)
-------------------------------

.. sip:class:: ArchXT_ADC
   :no-header:

This model is suitable for ATMega 0 series, ATTiny 0/1 series.


Configuration
*************

.. sip:class:: ArchXT_ADCConfig
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_ADC_TRIGGER`

Signalling
**********

See :py:class:`yasimavr.lib.core.ADC.SignalId` for signal ids.

Reference
*********

.. sip:class:: ArchXT_ADC
   :no-doc:
   :members:


.. _xt_adc2_ref:

Analog-to-Digital Converter (2)
-------------------------------

.. sip:class:: ArchXT_ADC2
   :no-header:

This model is suitable for ATTiny 2 series.

Configuration
*************

.. sip:class:: ArchXT_ADC2Config
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_ADC_TRIGGER`

Signalling
**********

See :py:class:`yasimavr.lib.core.ADC.SignalId` for signal ids.

Reference
*********

.. sip:class:: ArchXT_ADC2
   :no-doc:
   :members:


.. _xt_dac_ref:

Digital-to-Analog Converter
---------------------------

.. sip:class:: ArchXT_DAC
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_DACConfig
   :members:

Reference
*********

.. sip:class:: ArchXT_DAC
   :no-doc:
   :members: undoc


.. _xt_vref_ref:

Voltage Reference Controller
----------------------------

.. sip:class:: ArchXT_VREF
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_VREFConfig
   :members:

Control Request Codes definition
********************************

See :
   :py:data:`yasimavr.lib.core.CTLREQ_VREF_GET`

   :py:data:`yasimavr.lib.core.CTLREQ_VREF_SET_REF`

   :py:data:`yasimavr.lib.core.CTLREQ_VREF_SET_MUX`

   :py:data:`yasimavr.lib.core.CTLREQ_VREF_SET_TEMP`

Reference
*********

.. sip:class:: ArchXT_VREF
   :no-doc:
   :members:



Input/Outputs
#############


.. _xt_port_ref:

Port Controller
---------------

.. sip:class:: ArchXT_Port
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_PortConfig
   :members:

Reference
*********

.. sip:class:: ArchXT_Port
   :no-doc:
   :members:


.. _xt_portmux_ref:

Port Mux Controller
-------------------

.. sip:class:: ArchXT_PortMuxCtrl
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_PortMuxConfig
   :members:

Reference
*********

.. sip:class:: ArchXT_PortMuxCtrl
   :no-doc:
   :members:


.. _xt_spi_ref:

Serial Peripheral Interface
---------------------------

.. sip:class:: ArchXT_SPI
   :no-header:

**See also** :

- Common definitions in namespace: :py:class:`yasimavr.lib.core.SPI`
- Peripheral-side utilities: :py:mod:`yasimavr.utils.spi`

Configuration
*************

.. sip:class:: ArchXT_SPIConfig
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_SPI_TRANSFER`

Reference
*********

.. sip:class:: ArchXT_SPI
   :no-doc:
   :members:


.. _xt_twi_ref:

Two Wire Interface
------------------

.. sip:class:: ArchXT_TWI
   :no-header:

**See also**:

- Common definitions in namespace :py:class:`yasimavr.lib.core.TWI`
- Peripheral-side utilities: :py:mod:`yasimavr.utils.twi`

Configuration
*************

.. sip:class:: ArchXT_TWIConfig
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_TWI_BUS_ERROR`

Reference
*********

.. sip:class:: ArchXT_TWI
   :no-doc:
   :members:


.. _xt_usart_ref:

Universal Synchronous/Asynchronous Receiver/Transmitter
-------------------------------------------------------

.. sip:class:: ArchXT_USART
   :no-header:

**See also**:

- Common definitions in namespace :py:class:`yasimavr.lib.core.UART`
- Peripheral-side utilities: :py:mod:`yasimavr.utils.uart`

Configuration
*************

.. sip:class:: ArchXT_USARTConfig
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_USART_BYTES`

Reference
*********

.. sip:class:: ArchXT_USART
   :no-doc:
   :members:



Timers/Counters
###############


.. _xt_rtc_ref:

Real Time Clock
---------------

.. sip:class:: ArchXT_RTC
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_RTCConfig
   :members:

Reference
*********

.. sip:class:: ArchXT_RTC
   :no-doc:
   :members:


.. _xt_tca_ref:

Timer Counter Type A
--------------------

.. sip:class:: ArchXT_TimerA
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_TimerAConfig
   :members:

Control Request Codes definition
********************************

.. sip:macro-constant:: CTLREQ_TCA_REGISTER_TCB  AVR_CTLREQ_TCA_REGISTER_TCB

.. sip:macro-constant:: CTLREQ_TCA_GET_EVENT_HOOK  AVR_CTLREQ_TCA_GET_EVENT_HOOK

Reference
*********

.. sip:class:: ArchXT_TimerA
   :no-doc:
   :members:


.. _xt_tcb_ref:

Timer Counter Type B
--------------------

.. sip:class:: ArchXT_TimerB
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_TimerBConfig
   :members:

Control Request Codes definition
********************************

.. sip:macro-constant:: CTLREQ_TCB_GET_EVENT_HOOK  AVR_CTLREQ_TCB_GET_EVENT_HOOK

Reference
*********

.. sip:class:: ArchXT_TimerB
   :no-doc:
   :members:


.. _xt_tcd_ref:

Timer Counter Type D
--------------------

.. sip:class:: ArchXT_TimerD
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_TimerDConfig
   :members:

Reference
*********

.. sip:class:: ArchXT_TimerD
   :no-doc:
   :members:


.. _xt_wdt_ref:

Watchdog Timer
--------------

.. sip:class:: ArchXT_WDT
   :no-header:

Configuration
*************

.. sip:class:: ArchXT_WDTConfig
   :members:

Reference
*********

.. sip:class:: ArchXT_WDT
   :no-doc:
   :members:
