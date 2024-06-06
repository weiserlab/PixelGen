/*BH1750FVI C Driver*/

#ifndef BH1750FVI_H
#define BH1750FVI_H

#include <stdint.h>
#include "artemis_i2c.h"

#define ADDRESS1                          0X23        /*address pin not connected*/
#define ADDRESS2                          0X5C        /*address pin high*/

#define LIMIT1                            10          /*anything below LIMIT1 is H2 mode*/      
#define LIMIT2                            1000        /*anything between LIMIT1 and LIMIT2 is H1 mode, above LIMIT2 is L mode*/

#define POWER_DOWN                        0X00
#define POWER_ON                          0X01
#define RESET                             0X07
#define CONTINUOUS_H_1_MODE               0X10        /*1 lx 120ms*/
#define CONTINUOUS_H_2_MODE               0X11        /*0.5 lx 120ms*/
#define CONTINUOUS_L_MODE                 0X13        /*4 lx 16ms*/
#define ONE_TIME_H_1_MODE                 0X20
#define ONE_TIME_H_2_MODE                 0X21
#define ONE_TIME_L_MODE                   0X23

#define CONTINUOUS_H_1_DELAY_DEFAULT      120
#define CONTINUOUS_H_2_DELAY_DEFAULT      120
#define CONTINUOUS_L_DELAY_DEFAULT        16
#define ONE_TIME_H_1_DELAY_DEFAULT        120
#define ONE_TIME_H_2_DELAY_DEFAULT        120
#define ONE_TIME_L_DELAY_DEFAULT          16

#define CONTINUOUS_AUTO                   0X24
#define ONE_TIME_AUTO                     0X25
#define AUTOMATIC_RESOLUTION              0X26
#define MANUAL_RESOLUTION                 0X27
#define CONTINUOUS_METER                  0X28
#define ONE_TIME_METER                    0X29

#define NO_OPTICAL_WINDOW                 0X01

#define ARTEMIS_BH1750FVI_TX_BUFFER_LENGTH    (1)    //transmit needs 1 byte
#define ARTEMIS_BH1750FVI_RX_BUFFER_LENGTH    (2)    //receive needs 2 bytes

typedef uint8_t module_tx_buffer_t[ARTEMIS_BH1750FVI_TX_BUFFER_LENGTH];
typedef uint8_t module_rx_buffer_t[ARTEMIS_BH1750FVI_RX_BUFFER_LENGTH];


void set_i2c_bh1750(void);
void artemis_bh1750_initialize(void);

void lightSensor_begin(uint8_t address, uint8_t mode_of_operation);

void lightSensor_mode(uint8_t command);

uint16_t lightSensor_meter();

void light_i2c_write_single(uint8_t device_address, uint8_t command);
void light_i2c_read(uint8_t device_address, uint8_t *data_array);
extern void delay_function(uint32_t delay_milliseconds);

#endif
