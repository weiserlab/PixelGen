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

#ifndef HM01B0_PLATFORM_H
#define HM01B0_PLATFORM_H

#ifdef __cplusplus
extern "C"
{
#endif

// Micromod
#define HM01B0_PIN_D0                   AM_BSP_GPIO_G0
#define HM01B0_PIN_D1                   AM_BSP_GPIO_G1
#define HM01B0_PIN_D2                   AM_BSP_GPIO_G2
#define HM01B0_PIN_D3                   AM_BSP_GPIO_G3
#define HM01B0_PIN_D4                   AM_BSP_GPIO_G4
#define HM01B0_PIN_D5                   AM_BSP_GPIO_G5
#define HM01B0_PIN_D6                   AM_BSP_GPIO_G6
#define HM01B0_PIN_D7                   AM_BSP_GPIO_G7
#define HM01B0_PIN_VSYNC                AM_BSP_GPIO_G10
#define HM01B0_PIN_HSYNC                AM_BSP_GPIO_G9
#define HM01B0_PIN_PCLK                 AM_BSP_GPIO_AUD_IN
#define HM01B0_PIN_SCL                  AM_BSP_GPIO_SCL
#define HM01B0_PIN_SDA                  AM_BSP_GPIO_SDA

#define HM01B0_PIN_TRIG                 AM_BSP_GPIO_D1
#define HM01B0_PIN_INT                  AM_BSP_GPIO_I2C_INT

// Some boards do not support TRIG or INT pins
// #ifdef AM_BSP_GPIO_CAMERA_HM01B0_TRIG

// #endif // AM_BSP_GPIO_CAMERA_HM01B0_TRIG

// #ifdef AM_BSP_GPIO_CAMERA_HM01B0_INT

// #endif // AM_BSP_GPIO_CAMERA_HM01B0_INT


// Define AP3B's CTIMER and output pin for HM01B0 MCLK generation
#define HM01B0_MCLK_GENERATOR_MOD       1
#define HM01B0_MCLK_GENERATOR_SEG       AM_HAL_CTIMER_TIMERA
#define HM01B0_PIN_MCLK                 AM_BSP_GPIO_CAM_MCLK

// Deifne I2C controller and SCL(pin8)/SDA(pin9) are configured automatically.
#define HM01B0_IOM_MODE                 AM_HAL_IOM_I2C_MODE
#define HM01B0_IOM_MODULE               4
#define HM01B0_I2C_CLOCK_FREQ           AM_HAL_IOM_100KHZ


#ifdef __cplusplus
}
#endif

#endif // HM01B0_PLATFORM_H