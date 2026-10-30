=======================================
Supported Devices and Peripheral Models
=======================================

.. include:: globals.rst


`yasimavr` contains a number of builtin simulation models for peripherals and devices.
They can be used in simulation scripts simply with two lines, as the example below:

.. code-block:: py

    from yasimavr.device_library import load_device
    device_model = load_device('atmega4809')


Builtin XT models
*****************

.. list-table::
    :header-rows: 1

    * - Family
      - Devices

    * - ATMega 0 series
      - ATMega808 / 809 / 1608 / 1609 / 3208 / 3209 / 4808 / 4809

    * - ATTiny 0 series
      - ATTiny202 / 204 / 402 / 404 / 406 / 804 / 806 / 807 / |br|
        1604 / 1606 / 1607

    * - ATTiny 1 series
      - ATTiny212 / 214 / 412 / 414 / 416 / 814 / 816 / 817 / |br|
        1614 / 1616 / 1617 / 3216 / 3217

    * - ATTiny 2 series
      - ATTiny424 / 426 / 427 / 824 / 826 / 827 / |br|
        1624 / 1626 / 1627 / 3226 / 3227


XT Peripherals & memories
*************************

This section details the XT peripherals and memories that are supported by the simulation models.

Refer to the library reference of each implementation class for details and model limitations.

.. list-table::
    :header-rows: 1

    * - Peripheral/Memory
      - Devices [#f1]_
      - Model

    * - CPU - CPU General Registers
      - all
      - :ref:`ArchXT_MiscRegCtrl <xt_miscreg_ref>`

    * - FUSE - Configuration and User Fuses
      - all
      - :ref:`ArchXT_Fuses <xt_fuse_ref>`

    * - SIGROW - Signature Row
      - all
      - :ref:`ArchXT_MiscRegCtrl <xt_miscreg_ref>`

    * - NVMCTRL - Non Volatile Memory |br|
        Controller
      - all
      - :ref:`ArchXT_NVM <xt_nvmctrl_ref>`

    * - CLKCTRL - Clock Controller
      - all
      - :ref:`ArchXT_ClkCtrl <xt_clkctrl_ref>`

    * - SLPCTRL - Sleep Controller
      - all
      - :ref:`SleepController <core_slpctrl_ref>`

    * - RSTCTRL - Reset Controller
      - all
      - :ref:`ArchXT_ResetCtrl <xt_rstctrl_ref>`

    * - CPUINT - CPU Interrupts
      - all
      - :ref:`ArchXT_IntCtrl <xt_cpuint_ref>`

    * - EVSYS - Event System
      - no
      - *Not supported*

    * - PORTMUX - Port Multiplexer
      - all
      - :ref:`ArchXT_PortMuxCtrl <xt_portmux_ref>`

    * - PORT - I/O Pin Configuration
      - all
      - :ref:`ArchXT_Port <xt_port_ref>`

    * - BOD - Brown-Out Detector
      - no
      - *Not supported*

    * - VREF - Voltage Reference
      - all
      - :ref:`ArchXT_VREF <xt_vref_ref>`

    * - WDT - Watchdog Timer
      - all
      - :ref:`ArchXT_WDT <xt_wdt_ref>`

    * - TCA - Timer/Counter type A
      - all
      - :ref:`ArchXT_TimerA <xt_tca_ref>`

    * - TCB - Timer/Counter type B
      - all
      - :ref:`ArchXT_TimerB <xt_tcb_ref>`

    * - TCD - Timer/Counter type D
      - t1, t2
      - :ref:`ArchXT_TimerD <xt_tcd_ref>`

    * - RTC - Real Time Clock
      - all
      - :ref:`ArchXT_RTC <xt_rtc_ref>`

    * - USART - Universal Synchronous/|br|
        Asynchronous Receiver/Transmitter
      - all
      - :ref:`ArchXT_USART <xt_usart_ref>`

    * - SPI - Serial Peripheral Interface
      - all
      - :ref:`ArchXT_SPI <xt_spi_ref>`

    * - TWI - Two Wire Interface
      - all
      - :ref:`ArchXT_TWI <xt_twi_ref>`

    * - CRCSCAN - Cyclic Redundancy Check |br|
        Memory Scan
      - no
      - *Not supported*

    * - CCL - Configurable Custom Logic
      - no
      - *Not supported*

    * - AC - Analog Comparator
      - all
      - :ref:`ArchXT_ACP <xt_acp_ref>`

    * - ADC - Analog-to-Digital Converter
      - m0, t0, t1 |br|
        t2
      - :ref:`ArchXT_ADC <xt_adc1_ref>` |br|
        :ref:`ArchXT_ADC2 <xt_adc2_ref>`

    * - DAC - Digital-to-Analog Converter
      - t1
      - :ref:`ArchXT_DAC <xt_dac_ref>`

    * - UPDI - Unified Program and Debug |br|
        Interface
      - no
      - *Not supported*


.. [#f1] m0 = ATMega 0 series, t0 = ATTiny 0 series, t1 = ATTiny 1 series, |br|
         t2 = ATTiny 2 series


Builtin AVR models
******************

Here, the term 'AVR' means devices based on AVR, AVRe, AVRrc architectures.


.. list-table::
    :header-rows: 1

    * - Family
      - Devices

    * - ATMega xx8 series
      - ATMega48 / 48p / 88 / 88p / 168 / 168p / 328 / 328p

    * - ATTiny x4 series
      - ATTiny24 / 44 / 84


AVR Peripherals & memories
**************************

This section details the AVR peripherals and memories that are supported by the simulation models.

Refer to the library reference of each implementation class for details and model limitations.

.. list-table::
    :header-rows: 1

    * - Peripheral/Memory
      - Model

    * - CPU - CPU General Registers
      - :ref:`ArchAVR_MiscRegCtrl <avr_miscreg_ref>`

    * - FUSE - Configuration and User Fuses
      - :ref:`ArchAVR_Fuses <avr_fuse_ref>`

    * - NVMCTRL - Non Volatile Memory |br|
        Controller
      - :ref:`ArchAVR_NVM <avr_nvmctrl_ref>`

    * - CLKCTRL - Clock Controller
      - :ref:`ArchAVR_ClkCtrl <avr_clkctrl_ref>`

    * - SLPCTRL - Sleep Controller
      - :ref:`SleepController <core_slpctrl_ref>`

    * - RSTCTRL - Reset Controller
      - :ref:`ArchAVR_ResetCtrl <avr_rstctrl_ref>`

    * - CPUINT - CPU Interrupts
      - :ref:`ArchAVR_IntCtrl <avr_cpuint_ref>`

    * - PORT - I/O Pin Configuration
      - :ref:`ArchAVR_Port <avr_port_ref>`

    * - EXTINT - External Interrupts
      - :ref:`ArchAVR_ExtInt <avr_extint_ref>`

    * - BOD - Brown-Out Detector
      - *Not supported*

    * - VREF - Voltage Reference
      - :ref:`ArchAVR_VREF <avr_vref_ref>`

    * - WDT - Watchdog Timer
      - :ref:`ArchAVR_WDT <avr_wdt_ref>`

    * - TCx - Timer/Counter
      - :ref:`ArchAVR_Timer <avr_tc_ref>`

    * - USART - Universal Synchronous/|br|
        Asynchronous Receiver/Transmitter
      - :ref:`ArchAVR_USART <avr_usart_ref>`

    * - SPI - Serial Peripheral Interface
      - :ref:`ArchAVR_SPI <avr_spi_ref>`

    * - USI - Universal Serial Interface
      - :ref:`ArchAVR_USI <avr_usi_ref>`

    * - TWI - Two Wire Interface
      - :ref:`ArchAVR_TWI <avr_twi_ref>`

    * - AC - Analog Comparator
      - :ref:`ArchAVR_ACP <avr_acp_ref>`

    * - ADC - Analog-to-Digital Converter
      - :ref:`ArchAVR_ADC <avr_adc_ref>`

