
#include <stdio.h>
#include <string.h>
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "HM01B0.h"
#include "HM01B0_RAW8_QVGA_8bits_lsb_5fps.h"
#include "platform.h"
#include "artemis_i2c.h"
#include "artemis_iom.h"
#include "artemis_stream.h"
#include "bme280.h"
#include "bh1750fvi.h"

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
    // Standard UART settings: 115200-8-N-1g_sBspUartConfigHS
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

    am_hal_burst_avail_e burst_avail;

    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

    //
    // Set the default cache configuration
    //
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    //
    // Configure the board for low power operation.
    //
    am_bsp_low_power_init();
    
    //camera regulator undelcared
    // am_hal_gpio_pinconfig(AM_BSP_GPIO_CAMERA_HM01B0_DVDDEN, g_AM_HAL_GPIO_OUTPUT_12);
    // am_hal_gpio_output_set(AM_BSP_GPIO_CAMERA_HM01B0_DVDDEN);

    //
    // Enable the UART print interface at high speed
    //
    am_bsp_uart_printf_enable_custom(&g_sBspUartConfigHS);


    //
    // Enable interrupts so we can receive messages from the boot host.
    //
    am_hal_interrupt_master_enable();

    am_hal_burst_mode_initialize(&burst_avail);


    //i2c configuration for board
    i2c_all.iom.module = ARTEMIS_IOM_MODULE_I2C0; // QWIIC
    i2c_all.iom.config.eInterfaceMode = AM_HAL_IOM_I2C_MODE;
    i2c_all.iom.config.ui32ClockFreq = AM_HAL_IOM_400KHZ;
    artemis_iom_initialize(&i2c_all.iom);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_IOM2_SCL, g_AM_BSP_GPIO_IOM2_SCL);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_IOM2_SDA, g_AM_BSP_GPIO_IOM2_SDA); 
    //calibrate bme sensor
    set_i2c_bme280();
    artemis_bme280_initialize();


    //initialize camera himax camera it is using iom1 for i2c
    hm01b0_power_up(&s_HM01B0Cfg);

    am_util_delay_ms(1);

    hm01b0_mclk_enable(&s_HM01B0Cfg);

    am_util_delay_ms(1);

    hm01b0_init_if(&s_HM01B0Cfg);

    hm01b0_get_modelid(&s_HM01B0Cfg, &ui16ModelId);

    hm01b0_init_system(&s_HM01B0Cfg, (hm_script_t *)sHM01B0InitScript, sizeof(sHM01B0InitScript)/sizeof(hm_script_t));

    //calibrate camera
    hm01b0_cal_ae(&s_HM01B0Cfg, 10, s_ui8FrameBuffer, sizeof(s_ui8FrameBuffer)); 
    //there are two exposure function not used

    //sync with python file
    am_util_stdio_printf("%c", 0x55);
    while(1)
    {
        hm01b0_ae_cfg_t sAECfg;

        hm01b0_get_mode(&s_HM01B0Cfg, &ui8Mode);

        hm01b0_get_ae(&s_HM01B0Cfg, &sAECfg); //check if AE is converged
        hm01b0_cmd_update(&s_HM01B0Cfg);
        hm01b0_set_mode(&s_HM01B0Cfg, HM01B0_REG_MODE_SELECT_STREAMING_NFRAMES, 1); //take one image
        hm01b0_blocking_read_oneframe(&s_HM01B0Cfg, s_ui8FrameBuffer, sizeof(s_ui8FrameBuffer)); //read data of one frame

        framebuffer_dump(s_ui8FrameBuffer, sizeof(s_ui8FrameBuffer)); //print the data
        memset(s_ui8FrameBuffer, 0x00, sizeof(s_ui8FrameBuffer));
        am_util_delay_ms(5000);

        //read from bme sensor
        set_i2c_bme280();
        sensors_t sensorData = bme280_get_all(I2C_ADDRESS_1);
        am_util_stdio_printf("Temperature = %.2f\n", sensorData.temperature);
        am_util_stdio_printf("Humidity = %.2f\n", sensorData.humidity);
        am_util_delay_ms(2000);
        //read from bh1750 sensor
        set_i2c_bh1750();
        lightSensor_begin(ADDRESS1, ONE_TIME_AUTO);
        am_util_stdio_printf("Lux = %d\n", lightSensor_meter());
        am_util_delay_ms(5000);
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
    if (s_HM01B0Cfg.pfnGpioIsr)
        s_HM01B0Cfg.pfnGpioIsr();
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
