=======================
AVR Architecture models
=======================

.. include:: ../globals.rst


This section describes the simulation models defined for the AVR architecture
They are accessible by the following import :

.. code-block:: py

   import yasimavr.lib.arch_avr


.. sip:specification:: yasimavr.lib.arch_avr



Top level models
################


.. sip:class:: ArchAVR_Device
   :members:


.. sip:class:: ArchAVR_Core
   :members:



Controllers
###########


.. _avr_clkctrl_ref:

Clock Controller
----------------

.. sip:class:: ArchAVR_ClkCtrl
   :no-header:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_CLK_SET_EXTCLK`

Reference
*********

.. sip:class:: ArchAVR_ClkCtrl
   :no-doc:
   :members:


.. _avr_fuse_ref:

Fuses Controller
----------------

.. sip:class:: ArchAVR_Fuses
   :no-header:

Control Request Codes definition
********************************

.. sip:macro-constant:: CTLREQ_FUSE_VALUE AVR_CTLREQ_FUSE_VALUE

Reference
*********

.. sip:class:: ArchAVR_Fuses
   :no-doc:
   :members: undoc


.. _avr_cpuint_ref:

Interrupt Controller
--------------------

.. sip:class:: ArchAVR_IntCtrl
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_IntCtrlConfig
   :members:

Signalling
**********

See :py:class:`yasimavr.lib.core.InterruptController` for signal ids.

Reference
*********

.. sip:class:: ArchAVR_IntCtrl
   :no-doc:
   :members: undoc


.. _avr_miscreg_ref:

Misc Register Controller
------------------------

.. sip:class:: ArchAVR_MiscRegCtrl
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_MiscConfig
   :members:

Reference
*********

.. sip:class:: ArchAVR_MiscRegCtrl
   :no-doc:
   :members: undoc


.. _avr_nvmctrl_ref:

NVM Controller
--------------

.. sip:class:: ArchAVR_NVM
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_NVMConfig
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_NVM_REQUEST`

Reference
*********

.. sip:class:: ArchAVR_NVM
   :no-doc:
   :members:


.. _avr_rstctrl_ref:

Reset Controller
----------------

.. sip:class:: ArchAVR_ResetCtrl
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_ResetCtrlConfig
   :members:

Reference
*********

.. sip:class:: ArchAVR_ResetCtrl
   :no-doc:
   :members:



Analog peripherals
##################


.. _avr_acp_ref:

Analog Comparator
-----------------

.. sip:class:: ArchAVR_ACP
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_ACPConfig
   :members:

Signalling
**********

See :py:class:`yasimavr.lib.core.ACP.SignalId` for signal ids.

Reference
*********

.. sip:class:: ArchAVR_ACP
   :no-doc:
   :members:


.. _avr_adc_ref:

Analog-to-Digital Converter
---------------------------

.. sip:class:: ArchAVR_ADC
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_ADCConfig
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_ADC_TRIGGER`

Signalling
**********

See :py:class:`yasimavr.lib.core.ADC.SignalId` for signal ids.

Reference
*********

.. sip:class:: ArchAVR_ADC
   :no-doc:
   :members:


.. _avr_vref_ref:

Voltage Reference Controller
----------------------------

.. sip:class:: ArchAVR_VREF
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_VREFConfig
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

.. sip:class:: ArchAVR_VREF
   :no-doc:
   :members:



Input/Outputs
#############


.. _avr_port_ref:

Port Controller
---------------

.. sip:class:: ArchAVR_Port
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_PortConfig
   :members:

Reference
*********

.. sip:class:: ArchAVR_Port
   :no-doc:
   :members:


.. _avr_extint_ref:

External Interrupt Controller
-----------------------------

.. sip:class:: ArchAVR_ExtInt
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_ExtIntConfig
   :members:

Reference
*********

.. sip:class:: ArchAVR_ExtInt
   :no-doc:
   :members:


.. _avr_spi_ref:

Serial Peripheral Interface
---------------------------

.. sip:class:: ArchAVR_SPI
   :no-header:

**See also** :

- Common definitions in namespace: :py:class:`yasimavr.lib.core.SPI`
- Peripheral-side utilities: :py:mod:`yasimavr.utils.spi`

Configuration
*************

.. sip:class:: ArchAVR_SPIConfig
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_SPI_TRANSFER`

Reference
*********

.. sip:class:: ArchAVR_SPI
   :no-doc:
   :members:


.. _avr_twi_ref:

Two Wire Interface
------------------

.. sip:class:: ArchAVR_TWI
   :no-header:

**See also**:

- Common definitions in namespace :py:class:`yasimavr.lib.core.TWI`
- Peripheral-side utilities: :py:mod:`yasimavr.utils.twi`

Configuration
*************

.. sip:class:: ArchAVR_TWIConfig
   :members:

Reference
*********

.. sip:class:: ArchAVR_TWI
   :no-doc:
   :members:


.. _avr_usart_ref:

Universal Synchronous/Asynchronous Receiver/Transmitter
-------------------------------------------------------

.. sip:class:: ArchAVR_USART
   :no-header:

**See also**:

- Common definitions in namespace :py:class:`yasimavr.lib.core.UART`
- Peripheral-side utilities: :py:mod:`yasimavr.utils.uart`

Configuration
*************

.. sip:class:: ArchAVR_USARTConfig
   :members:

Control Request Codes definition
********************************

See :py:data:`yasimavr.lib.core.CTLREQ_USART_BYTES`

Reference
*********

.. sip:class:: ArchAVR_USART
   :no-doc:
   :members:



.. _avr_usi_ref:

Universal Serial Interface
--------------------------

.. sip:class:: ArchAVR_USI
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_USIConfig
   :members:

Reference
*********

.. sip:class:: ArchAVR_USI
   :no-doc:
   :members:



Timers/Counters
###############


.. _avr_tc_ref:

Timer Counter
-------------

.. sip:class:: ArchAVR_Timer
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_TimerConfig
   :members:

Control Request Codes definition
********************************

.. sip:macro-constant:: CTLREQ_TMR_GET_EXTCLK_HOOK  AVR_CTLREQ_TMR_GET_EXTCLK_HOOK

.. sip:macro-constant:: CTLREQ_TMR_GET_CAPT_HOOK  AVR_CTLREQ_TMR_GET_CAPT_HOOK

Reference
*********

.. sip:class:: ArchAVR_Timer
   :no-doc:
   :members:


.. _avr_wdt_ref:

Watchdog Timer
--------------

.. sip:class:: ArchAVR_WDT
   :no-header:

Configuration
*************

.. sip:class:: ArchAVR_WDTConfig
   :members:

Reference
*********

.. sip:class:: ArchAVR_WDT
   :no-doc:
   :members:
