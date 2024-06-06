///
/// @file artemis_mcu.c
///

#include "artemis_debug.h"
#include "artemis_mcu.h"
#include <am_bsp.h>
#include "am_util_stdio.h"

///
///
///
//high speed uart configuration
static const am_hal_uart_config_t g_sBspUartConfigHS =
{
    //
    // Standard UART settings: 115200-8-N-1
    //
    .ui32BaudRate = 115200,  //modify it to change the speed
    .ui32DataBits = AM_HAL_UART_DATA_BITS_8,
    .ui32Parity = AM_HAL_UART_PARITY_NONE,
    .ui32StopBits = AM_HAL_UART_ONE_STOP_BIT,
    .ui32FlowControl = AM_HAL_UART_FLOW_CTRL_NONE,

    //
    // Set TX and RX FIFOs to interrupt at half-full.
    //
    .ui32FifoLevels = (AM_HAL_UART_TX_FIFO_1_2 |
                    AM_HAL_UART_RX_FIFO_1_2),

    //
    // The default interface will just use polling instead of buffers.
    //
    .pui8TxBuffer = 0,
    .ui32TxBufferSize = 0,
    .pui8RxBuffer = 0,
    .ui32RxBufferSize = 0,
};

void artemis_mcu_initialize(void)
{
    am_hal_burst_avail_e burst_avail;

    // set the clock frequency
    ARTEMIS_DEBUG_HALSTATUS(am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0));

    // set the default cache configuration
    ARTEMIS_DEBUG_HALSTATUS(am_hal_cachectrl_config(&am_hal_cachectrl_defaults));
    ARTEMIS_DEBUG_HALSTATUS(am_hal_cachectrl_enable());

    // // enable the floating point module
    // am_hal_sysctrl_fpu_enable();

    // initialize mcu for burst mode operations
    ARTEMIS_DEBUG_HALSTATUS(am_hal_burst_mode_initialize(&burst_avail));

    // configure the board for low power operation
    am_bsp_low_power_init();

    // Enable the UART print interface at high speed
    //
    am_bsp_uart_printf_enable_custom(&g_sBspUartConfigHS);

    // enable interrupts
    ARTEMIS_DEBUG_HALSTATUS(am_hal_interrupt_master_enable());

}


///
///
///
void artemis_mcu_enableburst(void)
{
    am_hal_burst_mode_e burst_mode;

    ARTEMIS_DEBUG_HALSTATUS(am_hal_burst_mode_enable(&burst_mode));
}

///
///
///
void artemis_mcu_disableburst(void)
{
    am_hal_burst_mode_e burst_mode;

    ARTEMIS_DEBUG_HALSTATUS(am_hal_burst_mode_disable(&burst_mode));
}
