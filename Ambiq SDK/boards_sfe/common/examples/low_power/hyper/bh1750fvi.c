/*BH1750FVI C Driver*/

#include "bh1750fvi.h"
#include "artemis_i2c.h"

extern void am_util_delay_ms(uint32_t ui32MilliSeconds);

static uint8_t sensor_address;        /*accepts one of two possible addresses for bh1750fvi i2c*/
static uint16_t measure_result;       
static uint8_t current_mode;        /*current mode of operation: POWER_DOWN, POWER_ON, CONTINUOUS_H_1_MODE, CONTINUOUS_H_2_MODE, etc*/
static uint8_t auto_or_manual;        /*automatic or manual resolution selecting indicator: AUTOMATIC_RESOLUTION and MANUAL_RESOLUTION*/
static uint8_t continuous_or_onetime;       /*CONTINUOUS_METER or ONE_TIME_METER indicator*/

struct s_delay_time {
  uint16_t continuous_h_1;
  uint16_t continuous_h_2;
  uint16_t continuous_l;
  uint16_t one_time_h_1;
  uint16_t one_time_h_2;
  uint16_t one_time_l;
};

struct s_delay_time delay_time = {
  CONTINUOUS_H_1_DELAY_DEFAULT,
  CONTINUOUS_H_2_DELAY_DEFAULT,
  CONTINUOUS_L_DELAY_DEFAULT,
  ONE_TIME_H_1_DELAY_DEFAULT,
  ONE_TIME_H_2_DELAY_DEFAULT,
  ONE_TIME_L_DELAY_DEFAULT
};

typedef struct s_module_t
{
    // artemis_i2c_t i2c;
    module_tx_buffer_t txbuffer;
    module_rx_buffer_t rxbuffer;
} module_t;

static module_t module;

static void lightSensor_readBuffer();
static void lightSensor_automaticResolutionSet(uint16_t meter_value);

//low level functions
/*transmits one byte of command to bh1750fvi. in case of bh1750 sensor, there's no internal register map. each 'write' to i2c is a 'command'*/
void light_i2c_write_single(uint8_t device_address, uint8_t command)
{
    artemis_stream_t txstream = {0};
    
    artemis_stream_setbuffer(&txstream, module.txbuffer, ARTEMIS_BH1750FVI_TX_BUFFER_LENGTH);
    artemis_stream_put(&txstream, command);
    artemis_i2c_send(&i2c_all, 1, &txstream); //1 as true
}

/*function to read two bytes of measured illuminance from bh1750fvi. note: there's no register address, just request two bytes of data from bh1750 (slave)*/
void light_i2c_read(uint8_t device_address, uint8_t *data_array)
{
    artemis_stream_t rxstream = {0};       // Create an instance of the stream structure.

    artemis_stream_setbuffer(&rxstream, module.rxbuffer, ARTEMIS_BH1750FVI_RX_BUFFER_LENGTH);
    // Use artemis_i2c_receive to read 2 bytes from the sensor
    artemis_i2c_receive(&i2c_all, 1, &rxstream, 2); //1 as true
    // After the function call, data_array will contain the 2 bytes of illuminance data from the BH1750FVI sensor.
    data_array[0] = rxstream.buffer[0];
    data_array[1] = rxstream.buffer[1];

}

// /*function to initialize I2C peripheral in 100khz or 400khz*/
// void bh1750_I2C_init()
// {
// }


//high level functions
void set_i2c_bh1750(void) {
  i2c_all.address = ADDRESS1;
}

/*this should be called prior to lightSensor_meter(). 
 *address could be either ADDRESS1 or ADDRESS2, mode could be POWER_DOWN, CONTINUOUS_H_1_MODE... in manual or CONTINUOUS_AUTO or ONE_TIME_AUTO*/
void lightSensor_begin(uint8_t address, uint8_t mode_of_operation)
{
  // bh1750_I2C_init();
  sensor_address = address;
  lightSensor_mode(POWER_DOWN);
  lightSensor_mode(RESET);

  switch (mode_of_operation)
  {
    case CONTINUOUS_AUTO:
      lightSensor_mode(CONTINUOUS_L_MODE);        /*both auto modes start with low resolution mode*/
      auto_or_manual = AUTOMATIC_RESOLUTION;
      break;
    case ONE_TIME_AUTO:
      lightSensor_mode(ONE_TIME_L_MODE);        /*both auto modes start with low resolution mode*/
      auto_or_manual = AUTOMATIC_RESOLUTION;
      break;
    default:
      lightSensor_mode(mode_of_operation);        /*contains 3 continuous, 3 one-time modes and wrong commands. wrong commands are treated as one time H1 resolution mode*/
      auto_or_manual = MANUAL_RESOLUTION;
      break;
  }
}

/*function to set modes*/
void lightSensor_mode(uint8_t command)
{
  switch (command)
  {
    case POWER_DOWN:
      light_i2c_write_single(sensor_address, POWER_DOWN);
      break;
    case POWER_ON:
      light_i2c_write_single(sensor_address, POWER_ON);
      break;
    case RESET:
      light_i2c_write_single(sensor_address, RESET);
      break;
    case CONTINUOUS_H_1_MODE:
      light_i2c_write_single(sensor_address, CONTINUOUS_H_1_MODE);
      am_util_delay_ms(delay_time.continuous_h_1);
      continuous_or_onetime = CONTINUOUS_METER;
      break;
    case CONTINUOUS_H_2_MODE:
      light_i2c_write_single(sensor_address, CONTINUOUS_H_2_MODE);
      am_util_delay_ms(delay_time.continuous_h_2);
      continuous_or_onetime = CONTINUOUS_METER;
      break;
    case CONTINUOUS_L_MODE:
      light_i2c_write_single(sensor_address, CONTINUOUS_L_MODE);
      am_util_delay_ms(delay_time.continuous_l);
      continuous_or_onetime = CONTINUOUS_METER;
      break;
    case ONE_TIME_H_1_MODE:
      light_i2c_write_single(sensor_address, ONE_TIME_H_1_MODE);
      am_util_delay_ms(delay_time.one_time_h_1);
      continuous_or_onetime = ONE_TIME_METER;
      break;
    case ONE_TIME_H_2_MODE:
      light_i2c_write_single(sensor_address, ONE_TIME_H_2_MODE);
      am_util_delay_ms(delay_time.one_time_h_2);
      continuous_or_onetime = ONE_TIME_METER;
      break;
    case ONE_TIME_L_MODE:
      light_i2c_write_single(sensor_address, ONE_TIME_L_MODE);
      am_util_delay_ms(delay_time.one_time_l);
      continuous_or_onetime = ONE_TIME_METER;
      break;
    default:
      light_i2c_write_single(sensor_address, ONE_TIME_H_1_MODE);
      am_util_delay_ms(delay_time.one_time_h_1);
      command = ONE_TIME_H_1_MODE;        /*since the default mode contains possible wrong commands*/
      continuous_or_onetime = ONE_TIME_METER;
      break;
  }
  current_mode = command;
}

/*main function that commands BH1750FVI to meter light, either in one time or continuous modes, and in auto or manual modes*/
uint16_t lightSensor_meter()
{
  switch (auto_or_manual)
  {
    case MANUAL_RESOLUTION:
      if (continuous_or_onetime == CONTINUOUS_METER)
      {
        lightSensor_readBuffer();
        return measure_result;
      }
      else if (continuous_or_onetime == ONE_TIME_METER)
      {
        lightSensor_mode(current_mode);
        lightSensor_readBuffer();
        return measure_result;
      }
      break;
    case AUTOMATIC_RESOLUTION:
      if (continuous_or_onetime == CONTINUOUS_METER)
      {
        lightSensor_readBuffer();
        lightSensor_automaticResolutionSet(measure_result);
        return measure_result;
      }
      else if (continuous_or_onetime == ONE_TIME_METER)
      {
        lightSensor_mode(current_mode);
        lightSensor_readBuffer();
        lightSensor_automaticResolutionSet(measure_result);
        return measure_result;
      }
      break;
  }
  return 0xFFFF;  // Default return value indicating an error or unhandled scenario
}

static void lightSensor_readBuffer()
{
  uint8_t data_array[2];
  light_i2c_read(sensor_address, data_array);
  measure_result =  (uint16_t)(data_array[1]) +  (uint16_t)(data_array[0] << 8);
  if ((current_mode == ONE_TIME_H_2_MODE) || (current_mode == CONTINUOUS_H_2_MODE))
  {
    measure_result >>= 1;       /*since H2 mode has a resolution of 0.5 lux*/
  }
}

/*function to set resolution based on current metered value*/
static void lightSensor_automaticResolutionSet(uint16_t meter_value)
{
  uint8_t change_flag = 0;
  if ((meter_value < LIMIT1) && (current_mode != CONTINUOUS_H_2_MODE) && (current_mode != ONE_TIME_H_2_MODE))
  {
    if (continuous_or_onetime == CONTINUOUS_METER)
    {
      current_mode = CONTINUOUS_H_2_MODE;
    }
    else if (continuous_or_onetime == ONE_TIME_METER)
    {
      current_mode = ONE_TIME_H_2_MODE;
    }
    change_flag = 1;
  }
  else if ((meter_value >= LIMIT1) && (meter_value < LIMIT2) && (current_mode != CONTINUOUS_H_1_MODE) && (current_mode != ONE_TIME_H_1_MODE))
  {
    if (continuous_or_onetime == CONTINUOUS_METER)
    {
      current_mode = CONTINUOUS_H_1_MODE;
    }
    else if (continuous_or_onetime == ONE_TIME_METER)
    {
      current_mode = ONE_TIME_H_1_MODE;
    }
    change_flag = 1;
  }
  else if ((meter_value >= LIMIT2) && (current_mode != CONTINUOUS_L_MODE) && (current_mode != ONE_TIME_L_MODE))
  {
    if (continuous_or_onetime == CONTINUOUS_METER)
    {
      current_mode = CONTINUOUS_L_MODE;
    }
    else if (continuous_or_onetime == ONE_TIME_METER)
    {
      current_mode = ONE_TIME_L_MODE;
    }
    change_flag = 1;
  }
  if (change_flag)
  {
    lightSensor_mode(current_mode);
  }
}
