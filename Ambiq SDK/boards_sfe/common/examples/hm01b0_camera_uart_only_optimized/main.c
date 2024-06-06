// based on demo from Himax

/*
Copyright (c) 2019 SparkFun Electronics

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "HM01B0.h"
#include "HM01B0_RAW8_QVGA_8bits_lsb_5fps.h"
#include "platform.h"

// RTC time structure.
am_hal_rtc_time_t g_sTime;

#define BUTTON_GPIO_PIN 16

//*****************************************************************************
//
// RTC ISR
//
//*****************************************************************************
// static uint32_t g_RTCseconds = 0;

//*****************************************************************************
//
// BUTTON0 pin configuration settings.
//
//*****************************************************************************
//
// Set up the configuration for BUTTON0.
//
const am_hal_gpio_pincfg_t g_deepsleep_button0 =
{
    .uFuncSel = 3,
    .eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
};

//*****************************************************************************
//
// GPIO ISR
//
//*****************************************************************************
// void
// am_gpio_isr(void)
// {
//     //
//     // Delay for debounce.
//     //
//     am_util_delay_ms(200);

//     //
//     // Clear the GPIO Interrupt (write to clear).
//     //
//     AM_HAL_GPIO_MASKCREATE(GpioIntMask);
//     am_hal_gpio_interrupt_clear(AM_HAL_GPIO_MASKBIT(pGpioIntMask, BUTTON_GPIO_PIN));
// }
// void
// am_rtc_isr(void)
// {
//     //
//     // Clear the RTC alarm interrupt.
//     //
//     am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

//     //
//     // Check the desired number of seconds until LED is toggled.
//     //
//     // if ( ++g_RTCseconds >= 5 )
//     // {
//     //     //
//     //     // Reset the seconds counter.
//     //     //
//     //     g_RTCseconds = 0;

//     // }
// }

// #define DEMO_HM01B0_TEST_MODE_ENABLE
#define DEMO_HM01B0_FRAMEBUFFER_DUMP_ENABLE

//*****************************************************************************
//
// HM01B0 Configuration
//
//*****************************************************************************
static hm01b0_cfg_t         s_HM01B0Cfg =
{
    // i2c settings
    .ui16SlvAddr                = HM01B0_DEFAULT_ADDRESS,
    .eIOMMode                   = HM01B0_IOM_MODE,
    .ui32IOMModule              = HM01B0_IOM_MODULE,
    .sIOMCfg                    =
        {
            .eInterfaceMode     = HM01B0_IOM_MODE,
            .ui32ClockFreq      = HM01B0_I2C_CLOCK_FREQ,
        },
    .pIOMHandle                 = NULL,
    .ui8PinSCL                  = HM01B0_PIN_SCL,
    .ui8PinSDA                  = HM01B0_PIN_SDA,

    // MCLK settings
    .ui32CTimerModule           = HM01B0_MCLK_GENERATOR_MOD,
    .ui32CTimerSegment          = HM01B0_MCLK_GENERATOR_SEG,
    .ui32CTimerOutputPin        = HM01B0_PIN_MCLK,

    // data interface
    .ui8PinD0                   = HM01B0_PIN_D0,
    .ui8PinD1                   = HM01B0_PIN_D1,
    .ui8PinD2                   = HM01B0_PIN_D2,
    .ui8PinD3                   = HM01B0_PIN_D3,
    .ui8PinD4                   = HM01B0_PIN_D4,
    .ui8PinD5                   = HM01B0_PIN_D5,
    .ui8PinD6                   = HM01B0_PIN_D6,
    .ui8PinD7                   = HM01B0_PIN_D7,
    .ui8PinVSYNC                = HM01B0_PIN_VSYNC,
    .ui8PinHSYNC                = HM01B0_PIN_HSYNC,
    .ui8PinPCLK                 = HM01B0_PIN_PCLK,

#ifdef HM01B0_PIN_TRIG
    .ui8PinTrig                 = HM01B0_PIN_TRIG,
#endif // HM01B0_PIN_TRIG

#ifdef HM01B0_PIN_INT
    .ui8PinInt                  = HM01B0_PIN_INT,
#endif // HM01B0_PIN_INT

    .pfnGpioIsr                 = NULL,
};

static uint8_t s_ui8FrameBuffer[HM01B0_PIXEL_X_NUM * HM01B0_PIXEL_Y_NUM];

//*****************************************************************************
//
// Forward declarations
//
//*****************************************************************************
void boost_mode_enable(bool bEnable);
void am_gpio_isr(void);
void framebuffer_dump(uint8_t *pui8Buffer, uint32_t ui32BufferLen);


//*****************************************************************************
//
// High Speed UART Configuration
//
//*****************************************************************************
static const am_hal_uart_config_t g_sBspUartConfigHS =
{
    //
    // Standard UART settings: 115200-8-N-1
    //
    .ui32BaudRate = 115200,
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





//*****************************************************************************
//
// Main function.
//
//*****************************************************************************
int main(void)
{

    uint32_t    ui32Err     = HM01B0_ERR_OK;
    uint16_t    ui16ModelId = 0x0000;
    uint8_t     ui8Mode     = 0xFF;

    // Setting clock to 192 MHz
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

    //
    // Set the default cache configuration
    //
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    //
    //
    // Enable the XT for the RTC.
    //
    // am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);

    // Enable oscillator and clock source for RTC Interrupt /////////////////////
    // Select XT for RTC clock source
    //
    // am_hal_rtc_osc_select(AM_HAL_RTC_OSC_XT);

    //
    // Enable the RTC.
    //
    // am_hal_rtc_osc_enable();
    /////////////////////////////////////////////////////////////////////
    //
    // Configure the board for low power operation.
    //
    am_bsp_low_power_init();

    // Optimizations ******************************************************************
    // Disable UART prints
    // am_bsp_uart_printf_disable(); // cannot disable

    // Disable Debug prints
    // am_bsp_debug_printf_disable(); // cannot disable

    // Disable the debugger GPIOs saves about 1.2 uA total
    am_hal_gpio_pinconfig(20 /* SWDCLK */, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(21 /* SWDIO */, g_AM_HAL_GPIO_DISABLE);

    // These two GPIOs are critical: the TX/RX connections between the Artemis module and the CH340S on the Blackboard
    // are prone to backfeeding each other. To stop this from happening, we must reconfigure those pins as GPIOs
    // and then disable them completely:
    am_hal_gpio_pinconfig(48 /* TXO-0 */, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(49 /* RXI-0 */, g_AM_HAL_GPIO_DISABLE);
    // //*****************************************************************************

    // // Disable MCU peripherals
    // //*****************************************************************************
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOS);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM0);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM1);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM2);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM3);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM4);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_IOM5);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART0);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_UART1);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_ADC);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_SCARD);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_MSPI);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_PDM);
    am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_BLEL);
    //*****************************************************************************

#ifdef AM_BSP_GPIO_CAMERA_HM01B0_DVDDEN
    //
    // Turn on the camera regulator
    //
    am_hal_gpio_pinconfig(AM_BSP_GPIO_CAMERA_HM01B0_DVDDEN, g_AM_HAL_GPIO_OUTPUT_12);
    am_hal_gpio_output_set(AM_BSP_GPIO_CAMERA_HM01B0_DVDDEN);
#endif // AM_BSP_GPIO_CAMERA_HM01B0_DVDDEN

    //
    // Enable the UART print interface at high speed
    //
    am_bsp_uart_printf_enable_custom(&g_sBspUartConfigHS);

    // //
    // // Clear the terminal and print the banner.
    // //
    am_util_stdio_terminal_clear();
    // am_util_stdio_printf("HM01B0 Demo\n");
    // am_util_stdio_printf(" SCL:\tPin %d\n",  s_HM01B0Cfg.ui8PinSCL);
    // am_util_stdio_printf(" SDA:\tPin %d\n",  s_HM01B0Cfg.ui8PinSDA);
    // am_util_stdio_printf(" MCLK:\tPin %d\n",  s_HM01B0Cfg.ui32CTimerOutputPin);
    // am_util_stdio_printf(" VSYNC:\tPin %d\n", s_HM01B0Cfg.ui8PinVSYNC);
    // am_util_stdio_printf(" HSYNC\tPin %d\n",  s_HM01B0Cfg.ui8PinHSYNC);
    // am_util_stdio_printf(" PCLK:\tPin %d\n",  s_HM01B0Cfg.ui8PinPCLK);
    // am_util_stdio_printf(" D0:\tPin %d\n",    s_HM01B0Cfg.ui8PinD0);
    // am_util_stdio_printf(" D1:\tPin %d\n",    s_HM01B0Cfg.ui8PinD1);
    // am_util_stdio_printf(" D2:\tPin %d\n",    s_HM01B0Cfg.ui8PinD2);
    // am_util_stdio_printf(" D3:\tPin %d\n",    s_HM01B0Cfg.ui8PinD3);
    // am_util_stdio_printf(" D4:\tPin %d\n",    s_HM01B0Cfg.ui8PinD4);
    // am_util_stdio_printf(" D5:\tPin %d\n",    s_HM01B0Cfg.ui8PinD5);
    // am_util_stdio_printf(" D6:\tPin %d\n",    s_HM01B0Cfg.ui8PinD6);
    // am_util_stdio_printf(" D7:\tPin %d\n",    s_HM01B0Cfg.ui8PinD7);

    // to wake from deep sleep /////////////////
    // am_hal_rtc_alarm_interval_set(AM_HAL_RTC_ALM_RPT_SEC);

#if defined(AM_PART_APOLLO3) || defined(AM_PART_APOLLO3P)
    //
    // Turn OFF unneeded flash
    //
    if ( am_hal_pwrctrl_memory_enable(AM_HAL_PWRCTRL_MEM_FLASH_MIN) )
    {
        while(1);
    }

    //
    // For optimal Deep Sleep current, configure cache to be powered-down in deepsleep:
    //
    am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_CACHE);

    //
    // Power down SRAM, only 32K SRAM retained
    //
    am_hal_pwrctrl_memory_deepsleep_powerdown(AM_HAL_PWRCTRL_MEM_SRAM_MAX);
    am_hal_pwrctrl_memory_deepsleep_retain(AM_HAL_PWRCTRL_MEM_SRAM_32K_DTCM);
#endif // AM_PART_APOLLO3

// #if defined(AM_BSP_NUM_BUTTONS)
    //
    // Configure the button pin.
    //
    am_hal_gpio_pinconfig(BUTTON_GPIO_PIN, g_deepsleep_button0);

    //
    // Clear the GPIO Interrupt (write to clear).
    //
    AM_HAL_GPIO_MASKCREATE(GpioIntMask);
    am_hal_gpio_interrupt_clear(AM_HAL_GPIO_MASKBIT(pGpioIntMask, BUTTON_GPIO_PIN));

    //
    // Enable the GPIO/button interrupt.
    //
    am_hal_gpio_interrupt_enable(AM_HAL_GPIO_MASKBIT(pGpioIntMask, BUTTON_GPIO_PIN));

// #endif

    // //
    // // Clear the RTC alarm interrupt.
    // //
    // am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

    // //
    // // Enable the RTC alarm interrupt.
    // //
    // am_hal_rtc_int_enable(AM_HAL_RTC_INT_ALM);

    // Enable GPIO interrupts to the NVIC.
    //
    // NVIC_EnableIRQ(RTC_IRQn);
    NVIC_EnableIRQ(GPIO_IRQn);
 ///////////////////////////////////////////////////
    //
    // Enable interrupts so we can receive messages from the boot host.
    //
    am_hal_interrupt_master_enable();

    boost_mode_enable(true);

    hm01b0_power_up(&s_HM01B0Cfg);

    // todo: check the delay time to just fit the spec.
    am_util_delay_ms(1);

    hm01b0_mclk_enable(&s_HM01B0Cfg);

    // todo: check the delay time to just fit the spec.
    am_util_delay_ms(1);

    hm01b0_init_if(&s_HM01B0Cfg);

    hm01b0_get_modelid(&s_HM01B0Cfg, &ui16ModelId);

    // am_util_stdio_printf("HM01B0 Model ID 0x%04X\n", ui16ModelId);
      
    hm01b0_init_system(&s_HM01B0Cfg, (hm_script_t *)sHM01B0InitScript, sizeof(sHM01B0InitScript)/sizeof(hm_script_t));

#ifdef DEMO_HM01B0_TEST_MODE_ENABLE
    am_util_stdio_printf("HM01B0 Enable walking 1s test mode\n");
    hm01b0_test_walking1s(&s_HM01B0Cfg);
#else
    hm01b0_cal_ae(&s_HM01B0Cfg, 10, s_ui8FrameBuffer, sizeof(s_ui8FrameBuffer));
#endif
    am_util_stdio_printf("%c", 0x55);

    while(1)
    {
        hm01b0_ae_cfg_t sAECfg;

        // am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_WAKE);
        am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
        boost_mode_enable(true);
        hm01b0_get_mode(&s_HM01B0Cfg, &ui8Mode);

        // am_util_stdio_printf("HM01B0 current mode %d\n", ui8Mode);

        // ui32Err = hm01b0_get_ae(&s_HM01B0Cfg, &sAECfg);
        // am_util_stdio_printf("AE convergance(0x%02X) TargetMean 0x%02X, ConvergeInTh 0x%02X, AEMean 0x%02X\n", ui32Err, sAECfg.ui8AETargetMean, sAECfg.ui8ConvergeInTh, sAECfg.ui8AEMean);

        hm01b0_cmd_update(&s_HM01B0Cfg);

        hm01b0_set_mode(&s_HM01B0Cfg, HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES, 1);

        hm01b0_blocking_read_oneframe(&s_HM01B0Cfg, s_ui8FrameBuffer, sizeof(s_ui8FrameBuffer));

        boost_mode_enable(false); /////////

#ifdef DEMO_HM01B0_TEST_MODE_ENABLE
        hm01b0_test_walking1s_check_data_sanity(s_ui8FrameBuffer, sizeof(s_ui8FrameBuffer), 10);
        am_util_delay_ms(3000);
#endif

#ifdef DEMO_HM01B0_FRAMEBUFFER_DUMP_ENABLE
        framebuffer_dump(s_ui8FrameBuffer, sizeof(s_ui8FrameBuffer));
#endif
        memset(s_ui8FrameBuffer, 0x00, sizeof(s_ui8FrameBuffer));

        // am_util_delay_ms(5000);

        // while(1){
            am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_NORMAL);
        // }

        // // give some time for user to stop the external itm logging.
        am_util_delay_ms(5000);
        // break;
    }

    hm01b0_deinit_if(&s_HM01B0Cfg);

    hm01b0_mclk_disable(&s_HM01B0Cfg);

    hm01b0_power_down(&s_HM01B0Cfg);

    boost_mode_enable(false);

    // am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);

    //
    // Loop forever.
    //
    while (1)
    {
        //
        // Go to Deep Sleep.
        //
        am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
    }
}





//*****************************************************************************
//
// Helper Functions
//
//*****************************************************************************

// GPIO ISR
void
am_gpio_isr(void)
{
    if (s_HM01B0Cfg.pfnGpioIsr){
        s_HM01B0Cfg.pfnGpioIsr();
    }

        //
    // Delay for debounce.
    //
    // am_util_delay_ms(200); //

    //
    // Clear the GPIO Interrupt (write to clear).
    //
    AM_HAL_GPIO_MASKCREATE(GpioIntMask);
    am_hal_gpio_interrupt_clear(AM_HAL_GPIO_MASKBIT(pGpioIntMask, BUTTON_GPIO_PIN));
}

// frame buffer dump
void framebuffer_dump(uint8_t *pui8Buffer, uint32_t ui32BufferLen)
{
    am_util_stdio_printf("+++ frame +++");

    for (uint32_t ui32Idx = 0; ui32Idx < ui32BufferLen; ui32Idx++)
    {
        if ((ui32Idx & 0xF) == 0x00)
        {
            am_util_stdio_printf("\n0x%08X ", ui32Idx);
            // this delay is to let itm have time to flush out data.
            am_util_delay_ms(1);
        }

        am_util_stdio_printf("%02X ", *(pui8Buffer + ui32Idx));
    }

    am_util_stdio_printf("\n--- frame ---\n");
    am_util_delay_ms(1);
}

// burst mode enable
void boost_mode_enable(bool bEnable){
    am_hal_burst_avail_e          eBurstModeAvailable;
    am_hal_burst_mode_e           eBurstMode;

    // Check that the Burst Feature is available.
    if (AM_HAL_STATUS_SUCCESS == am_hal_burst_mode_initialize(&eBurstModeAvailable)){
        if (AM_HAL_BURST_AVAIL == eBurstModeAvailable){
            am_util_stdio_printf("Apollo3 Burst Mode is Available\n");
        }
        else{
            am_util_stdio_printf("Apollo3 Burst Mode is Not Available\n");
            while(1){};
        }
    }
    else{
        am_util_stdio_printf("Failed to Initialize for Burst Mode operation\n");
    }

    // Make sure we are in "Normal" mode.
    if (AM_HAL_STATUS_SUCCESS == am_hal_burst_mode_disable(&eBurstMode)){
        if (AM_HAL_NORMAL_MODE == eBurstMode){
            am_util_stdio_printf("Apollo3 operating in Normal Mode (48MHz)\n");
        }
    }
    else{
        am_util_stdio_printf("Failed to Disable Burst Mode operation\n");
    }

    // Put the MCU into "Burst" mode.
    if (bEnable)
    {
        if (AM_HAL_STATUS_SUCCESS == am_hal_burst_mode_enable(&eBurstMode)){
            if (AM_HAL_BURST_MODE == eBurstMode){
                am_util_stdio_printf("Apollo3 operating in Burst Mode (96MHz)\n");
            }
        }
        else{
            am_util_stdio_printf("Failed to Enable Burst Mode operation\n");
        }
    }
}