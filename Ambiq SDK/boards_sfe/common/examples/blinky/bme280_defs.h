/*bme280 barometric pressure and temperature sensor C Driver*/
/*Reza Ebrahimi - https://github.com/ebrezadev */
/*Version 1.0*/

#ifndef __BME280_DEFS_H__
#define __BME280_DEFS_H__

/*configurable definitions*/
#define STARTUP_DELAY_IN_MS       1000
/*end of configurable definitions*/

/*constant definitions*/
#define BME280_DEFAULT_CHIP_ID    0X60
#define BME280_RESET_VALUE        0XB6

#define SEA_LEVEL_PRESSURE        101325

#define ID_ADDRESS                0XD0
#define RESET_ADDRESS             0XE0
#define CTRL_HUM_ADDRESS          0XF2
#define STATUS_ADDRESS            0XF3
#define CTRL_MEAS_ADDRESS         0XF4
#define CONFIG_ADDRESS            0XF5
#define PRESSURE_MSB              0XF7
#define PRESSURE_LSB              0XF8
#define PRESSURE_XLSB             0XF9
#define TEMP_MSB                  0XFA
#define TEMP_LSB                  0XFB
#define TEMP_XLSB                 0XFC
#define HUM_MSB                   0XFD
#define HUM_LSB                   0XFE

#define OSRS_H_BIT                0X00
#define IM_UPDATE_BIT             0X00
#define MEASURING_BIT             0X01
#define MODE_BIT                  0X00
#define OSRS_P_BIT                0X02
#define OSRS_T_BIT                0X05
#define SPI3W_EN_BIT              0X00
#define FILTER_BIT                0X02
#define T_SB_BIT                  0X05

#define OSRS_H_LENGTH             0X03
#define IM_UPDATE_LENGTH          0X01
#define MEASURING_LENGTH          0X03
#define MODE_LENGTH               0X02
#define OSRS_P_LENGTH             0X03
#define OSRS_T_LENGTH             0X03
#define SPI3W_EN_LENGTH           0X01
#define FILTER_LENGTH             0X03
#define T_SB_LENGTH               0X03

#define T1_ADDRESS                0X88
#define T2_ADDRESS                0X8A
#define T3_ADDRESS                0X8C
#define P1_ADDRESS                0X8E
#define P2_ADDRESS                0X90
#define P3_ADDRESS                0X92
#define P4_ADDRESS                0X94
#define P5_ADDRESS                0X96
#define P6_ADDRESS                0X98
#define P7_ADDRESS                0X9A
#define P8_ADDRESS                0X9C
#define P9_ADDRESS                0X9E
#define H1_ADDRESS                0XA1
#define H2_ADDRESS                0XE1
#define H3_ADDRESS                0XE3
#define H4_ADDRESS                0XE4        /*0XE5[3:0] LSB, 0XE4[7:0] MSB*/
#define H5_ADDRESS                0XE5        /*0XE6[7:0] MSB, 0XE5[7:4] LSB*/
#define H6_ADDRESS                0XE7
/*end of constant definitions*/

#define PRESSURE_OVERSAMPLING_DEFAULT       OVERSAMPLING_1X
#define TEMPERATURE_OVERSAMPLING_DEFAULT    OVERSAMPLING_1X
#define HUMIDITY_OVERSAMPLING_DEFAULT       OVERSAMPLING_1X
#define STANDBY_TIME_DEFAULT                T_STANDBY_125MS
#define FILTER_DEFAULT                      FILTER_OFF
#define MODE_DEFAULT                        MODE_SLEEP

#endif
