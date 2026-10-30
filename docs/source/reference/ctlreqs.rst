=================================
Peripheral Control Requests codes
=================================

This section describes the requests codes usable with the ctlreq API.

.. seealso::

   Explanation about the concept: :ref:`ctlreq_concept`


.. sip:specification:: yasimavr.lib.core


.. sip:class:: ctlreq_data_t
   :members:


Generic request codes
---------------------

.. sip:macro-constant:: CTLREQ_BASE              AVR_CTLREQ_BASE


.. sip:macro-constant:: CTLREQ_GET_SIGNAL        AVR_CTLREQ_GET_SIGNAL


CPU request codes (internal use)
--------------------------------

.. sip:macro-constant:: CTLREQ_CORE_BREAK        AVR_CTLREQ_CORE_BREAK


.. sip:macro-constant:: CTLREQ_CORE_SLEEP        AVR_CTLREQ_CORE_SLEEP


.. sip:macro-constant:: CTLREQ_CORE_WAKEUP       AVR_CTLREQ_CORE_WAKEUP


.. sip:macro-constant:: CTLREQ_CORE_SHORTING     AVR_CTLREQ_CORE_SHORTING


.. sip:macro-constant:: CTLREQ_CORE_CRASH        AVR_CTLREQ_CORE_CRASH


.. sip:macro-constant:: CTLREQ_CORE_RESET        AVR_CTLREQ_CORE_RESET


.. sip:macro-constant:: CTLREQ_CORE_RESET_FLAG   AVR_CTLREQ_CORE_RESET_FLAG


.. sip:macro-constant:: CTLREQ_CORE_NVM          AVR_CTLREQ_CORE_NVM


.. sip:macro-constant:: CTLREQ_CORE_HALT         AVR_CTLREQ_CORE_HALT


.. sip:macro-constant:: CTLREQ_CORE_SECTIONS     AVR_CTLREQ_CORE_SECTIONS


.. sip:macro-constant:: CTLREQ_WATCHDOG_RESET    AVR_CTLREQ_WATCHDOG_RESET


.. sip:macro-constant:: CTLREQ_NVM_REQUEST       AVR_CTLREQ_NVM_REQUEST


.. sip:macro-constant:: CTLREQ_SLEEP_CALL        AVR_CTLREQ_SLEEP_CALL


.. sip:macro-constant:: CTLREQ_SLEEP_PSEUDO      AVR_CTLREQ_SLEEP_PSEUDO


Peripheral request codes
------------------------

.. sip:macro-constant:: CTLREQ_CLK_SET_EXTCLK        AVR_CTLREQ_CLK_SET_EXTCLK


.. sip:macro-constant:: CTLREQ_INTR_REGISTER         AVR_CTLREQ_INTR_REGISTER


.. sip:macro-constant:: CTLREQ_INTR_RAISE            AVR_CTLREQ_INTR_RAISE


.. sip:macro-constant:: CTLREQ_ADC_TRIGGER           AVR_CTLREQ_ADC_TRIGGER

   See also: :ref:`ADC_ref`


.. sip:macro-constant:: CTLREQ_SPI_TRANSFER          AVR_CTLREQ_SPI_TRANSFER

   See also: :ref:`SPI_ref`


.. sip:macro-constant:: CTLREQ_TWI_BUS_ERROR         AVR_CTLREQ_TWI_BUS_ERROR

   See also: :ref:`TWI_ref`


.. sip:macro-constant:: CTLREQ_USART_BYTES           AVR_CTLREQ_USART_BYTES

   See also :ref:`UART_ref`


.. sip:macro-constant:: CTLREQ_VREF_GET              AVR_CTLREQ_VREF_GET


.. sip:macro-constant:: CTLREQ_VREF_SET_REF          AVR_CTLREQ_VREF_SET_REF


.. sip:macro-constant:: CTLREQ_VREF_SET_MUX          AVR_CTLREQ_VREF_SET_MUX


.. sip:macro-constant:: CTLREQ_VREF_SET_TEMP         AVR_CTLREQ_VREF_SET_TEMP

