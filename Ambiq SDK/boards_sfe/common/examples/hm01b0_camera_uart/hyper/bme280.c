/*bme280 barometric pressure, temperature and humidity sensor C Driver*/
/*Reza Ebrahimi - https://github.com/ebrezadev */
/*Version 1.0*/

#include "bme280.h"

static calibration_param_t dig1;        /*dig1 and dig2 are based on calibration_param_t struct, which encapsulates all calibration values*/
static calibration_param_t dig2;
static int32_t t_fine;        /*used for pressure compensation, changes with temperature*/
static float altitudeOffsetHypsometric = 0;

/*static function prototypes*/
static void bme280_set_bits_in_register(uint8_t deviceAddress, uint8_t registerAddress, uint8_t fieldData, uint8_t fieldStartBitAddress, uint8_t fieldLength);
static void bme280_get_calibration(uint8_t deviceAddress, calibration_param_t *dig);
static calibration_param_t *bme280_which_dig(uint8_t deviceAddress);
static int32_t bme280_raw_temperature_data(uint8_t deviceAddress);
static int32_t bme280_raw_pressure_data(uint8_t deviceAddress);
static uint32_t bme280_raw_humidity_data(uint8_t deviceAddress);
static uint16_t bme280_read_calibration_word_unsigned(uint8_t deviceAddress, uint8_t startRegisterAddress);
static int16_t bme280_read_calibration_word_signed(uint8_t deviceAddress, uint8_t startRegisterAddress);
static int8_t bme280_read_calibration_byte_signed(uint8_t deviceAddress, uint8_t registerAddress);
static uint8_t bme280_read_calibration_byte_unsigned(uint8_t deviceAddress, uint8_t registerAddress);
static void bme280_get_bits_in_register(uint8_t deviceAddress, uint8_t registerAddress, uint8_t *fieldData, uint8_t fieldStartBitAddress, uint8_t fieldLength);
void artemis_bme280_initialize(void);

extern void am_util_delay_ms(uint32_t ui32MilliSeconds);

// artemis_i2c_t i2c;
//low level

/*writes an array (data[]) of arbitrary size (dataLength) to I2C address (deviceAddress), starting from an internal register address (startRegisterAddress)*/
void bme280_write_array(uint8_t deviceAddress, uint8_t startRegisterAddress, uint8_t *data, uint8_t dataLength)
{
    uint8_t buffer[dataLength + 1];
    artemis_stream_t txstream = {0};

    artemis_stream_setbuffer(&txstream, buffer, dataLength + 1);
    artemis_stream_put(&txstream, startRegisterAddress);

    for (size_t i = 0; i < dataLength; i++) {
      artemis_stream_put(&txstream, data[i]);
    }
    artemis_i2c_send(&i2c_all, 1, &txstream);
}

/*reads an array (data[]) of arbitrary size (dataLength) from I2C address (deviceAddress), starting from an internal register address (startRegisterAddress)*/
void bme280_read_array (uint8_t deviceAddress, uint8_t startRegisterAddress, uint8_t *data, uint8_t dataLength)
{
  artemis_stream_t rxstream = {0};       // Create an instance of the stream structure.

  bme280_write_array(0 ,startRegisterAddress, NULL, 0); //send the register u want to read
  uint8_t buffer[dataLength];
  artemis_stream_setbuffer(&rxstream, buffer, dataLength);
  artemis_i2c_receive(&i2c_all, 1, &rxstream, dataLength); 
  for (size_t i = 0; i < dataLength ; i ++) {
    data[i] = rxstream.buffer[i];
  }
}

//high level
void set_i2c_bme280(void) {
  i2c_all.address = I2C_ADDRESS_1;
}

void artemis_bme280_initialize(void) {
  i2c_all.address = I2C_ADDRESS_1;
  // artemis_i2c_t *i2c = &i2c;
  // i2c->address = I2C_ADDRESS_1;

  // i2c->iom.module = ARTEMIS_IOM_MODULE_I2C0; // QWIIC
  // i2c->iom.config.eInterfaceMode = AM_HAL_IOM_I2C_MODE;
  // i2c->iom.config.ui32ClockFreq = AM_HAL_IOM_400KHZ;
  // artemis_iom_initialize(&i2c->iom);

  // am_hal_gpio_pinconfig(AM_BSP_GPIO_IOM4_SCL, g_AM_BSP_GPIO_IOM4_SCL);
  // am_hal_gpio_pinconfig(AM_BSP_GPIO_IOM4_SDA, g_AM_BSP_GPIO_IOM4_SDA); 

  bme280_reset(I2C_ADDRESS_1);
  am_util_delay_ms(STARTUP_DELAY_IN_MS);
  bme280_get_calibration(I2C_ADDRESS_1, &dig1);
  bme280_set_default(I2C_ADDRESS_1);
}

/*initializer, detects the connected chips automatically and resets all of them. returns the detected addresses (if any),
   and automatically gets the calibration data for further calculations. also sets the default values.
*/

/*checks chip id to see if it really is a bme280 module with correct wiring and address*/
output_status_t bme280_check_id(uint8_t deviceAddress)
{
  uint8_t data;
  bme280_read_array (deviceAddress, ID_ADDRESS, &data, 1);

  if (data == BME280_DEFAULT_CHIP_ID)
  {
    return BME280_TRUE;
  }
  else
  {
    return BME280_FALSE;
  }
}

/*discovers which of two i2c addresses are used, if any or both*/
i2c_address_t bme280_check_connected_address()
{
  output_status_t id1 = bme280_check_id(I2C_ADDRESS_1);
  output_status_t id2 = bme280_check_id(I2C_ADDRESS_2);

  if ((id1 == BME280_TRUE) && (id2 == BME280_TRUE))
  {
    return BOTH_DETECTED;
  }
  else if (id1 == BME280_TRUE)
  {
    return I2C_ADDRESS_1;
  }
  else if (id2 == BME280_TRUE)
  {
    return I2C_ADDRESS_2;
  }
  else
  {
    return NONE_DETECTED;
  }
}

/*soft resets bm280 using special reset register*/
void bme280_reset(uint8_t deviceAddress)
{
  uint8_t data = BME280_RESET_VALUE;
  bme280_write_array(deviceAddress, RESET_ADDRESS, &data, 1);
}

/*setting bme280 mode, NORMAL_MODE, FORCED_MODE, SLEEP_MODE*/
void bme280_set_mode(uint8_t deviceAddress, operation_mode_t operationMode)
{
  bme280_set_bits_in_register(deviceAddress, CTRL_MEAS_ADDRESS, operationMode, MODE_BIT, MODE_LENGTH);
}

/*setting pressure oversampling from 0 (skip) to 16x*/
void bme280_set_pressure_accuracy(uint8_t deviceAddress, over_sampling_t osValue)
{
  bme280_set_bits_in_register(deviceAddress, CTRL_MEAS_ADDRESS, osValue, OSRS_P_BIT, OSRS_P_LENGTH);
}

/*setting temperature oversampling from 0 (skip) to 16x*/
void bme280_set_temperature_accuracy(uint8_t deviceAddress, over_sampling_t osValue)
{
  bme280_set_bits_in_register(deviceAddress, CTRL_MEAS_ADDRESS, osValue, OSRS_T_BIT, OSRS_T_LENGTH);
}

/*setting humidity oversampling from 0 (skip) to 16x*/
void bme280_set_humidity_accuracy(uint8_t deviceAddress, over_sampling_t osValue)
{
  bme280_set_bits_in_register(deviceAddress, CTRL_HUM_ADDRESS, osValue, OSRS_H_BIT, OSRS_H_LENGTH);
}

/*sets standby time between measurements in normal mode. lower standby time means higher power consumption*/
void bme280_set_standby_time(uint8_t deviceAddress, standby_time_t standbyTime)
{
  bme280_set_bits_in_register(deviceAddress, CONFIG_ADDRESS, standbyTime, T_SB_BIT, T_SB_LENGTH);
}

/*sets low pass internal filter coefficient for bme280. used in noisy environments*/
void bme280_set_filter_coefficient(uint8_t deviceAddress, iir_filter_t filterCoefficient)
{
  bme280_set_bits_in_register(deviceAddress, CONFIG_ADDRESS, filterCoefficient, FILTER_BIT, FILTER_LENGTH);
}

/*one function to completely set up bme280 mode, temperature, pressure and humidity oversampling, normal mode standby time and IIR filter coefficient*/
void bme280_set(uint8_t deviceAddress, operation_mode_t operationMode, over_sampling_t tempOS, over_sampling_t pressureOS, over_sampling_t humidityOS, standby_time_t standbyTime, iir_filter_t filterCoefficient)
{
  bme280_set_mode(deviceAddress, MODE_SLEEP);
  am_util_delay_ms(50);
  bme280_set_pressure_accuracy(deviceAddress, pressureOS);
  bme280_set_temperature_accuracy(deviceAddress, tempOS);
  bme280_set_humidity_accuracy(deviceAddress, humidityOS);
  bme280_set_standby_time(deviceAddress, standbyTime);
  bme280_set_filter_coefficient(deviceAddress, filterCoefficient);
  bme280_set_mode(deviceAddress, operationMode);
}

/*sets bme280 to defaults*/
void bme280_set_default(uint8_t deviceAddress)
{
  bme280_set_mode(deviceAddress, MODE_SLEEP);
  am_util_delay_ms(50);
  bme280_set_pressure_accuracy(deviceAddress, PRESSURE_OVERSAMPLING_DEFAULT);
  bme280_set_temperature_accuracy(deviceAddress, TEMPERATURE_OVERSAMPLING_DEFAULT);
  bme280_set_humidity_accuracy(deviceAddress, HUMIDITY_OVERSAMPLING_DEFAULT);
  bme280_set_standby_time(deviceAddress, STANDBY_TIME_DEFAULT);
  bme280_set_filter_coefficient(deviceAddress, FILTER_DEFAULT);
  bme280_set_mode(deviceAddress, MODE_DEFAULT);
}

/*reads temperature value from internal bme280 registers in centigrade*/
int32_t bme280_get_temperature(uint8_t deviceAddress)
{
  calibration_param_t *dig = bme280_which_dig(deviceAddress);       /*automatically relating device address and calibration data*/

  int32_t adc_T = bme280_raw_temperature_data(deviceAddress);      /*per Bosch guideline*/
  int32_t var1, var2, T;

  var1 = ((((adc_T >> 3) - ((int32_t)dig->T1 << 1))) * ((int32_t)dig->T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)dig->T1)) * ((adc_T >> 4) - ((int32_t)dig->T1))) >> 12) * ((int32_t)dig->T3)) >> 14;

  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T;
}

/*reads pressure value from internal bme280 registers in pascal*/
uint32_t bme280_get_pressure(uint8_t deviceAddress)
{
  calibration_param_t *dig = bme280_which_dig(deviceAddress);

  int32_t adc_P = bme280_raw_pressure_data(deviceAddress);
  int32_t var1, var2;
  uint32_t p;

  var1 = (((int32_t) t_fine) / 2) - (int32_t) 64000;
  var2 = (((var1 / 4) * (var1 / 4)) / 2048) * ((int32_t) dig->P6);
  var2 = var2 + ((var1 * ((int32_t) dig->P5)) * 2);
  var2 = (var2 / 4) + (((int32_t) dig->P4) * 65536);
  var1 = (((dig->P3 * (((var1 / 4) * (var1 / 4)) / 8192)) / 8) + ((((int32_t) dig->P2) * var1) / 2)) / 262144;
  var1 = ((((32768 + var1)) * ((int32_t) dig->P1)) / 32768);
  p = (uint32_t)(((int32_t)(1048576 - adc_P) - (var2 / 4096)) * 3125);

  /* Avoid exception caused by division with zero */
  if (var1 != 0)
  {
    /* Check for overflows against UINT32_MAX/2; if pres is left-shifted by 1 */
    if (p < 0x80000000)
    {
      p = (p << 1) / ((uint32_t) var1);
    }
    else
    {
      p = (p / (uint32_t) var1) * 2;
    }
    var1 = (((int32_t) dig->P9) * ((int32_t) (((p / 8) * (p / 8)) / 8192))) / 4096;
    var2 = (((int32_t) (p / 4)) * ((int32_t) dig->P8)) / 8192;
    p = (uint32_t) ((int32_t)p + ((var1 + var2 + dig->P7) / 16));
  }
  else
  {
    p = 0;
  }
  return p;
}

uint32_t bme280_get_humidity(uint8_t deviceAddress)
{
  calibration_param_t *dig = bme280_which_dig(deviceAddress);

  uint32_t adc_H =(uint32_t)bme280_raw_humidity_data(deviceAddress);

  int32_t v_x1_u32r = (t_fine - ((int32_t)76800));

  v_x1_u32r = (((((adc_H << 14) - (((int32_t)dig->H4) << 20) - (((int32_t)dig->H5) * 
    v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * 
    ((int32_t)dig->H6)) >> 10) * (((v_x1_u32r * ((int32_t)dig->H3)) >> 11) + 
    ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)dig->H2) +
    8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
    ((int32_t)dig->H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  return (uint32_t)(v_x1_u32r >> 12);
}

/*calculates altitude from barometric pressure without temperature as an argument*/
float bme280_calculate_altitude_quick(uint8_t deviceAddress, uint32_t barometricPressure)
{
  float altitudeFloat;

  altitudeFloat = 44307.69396 * (1 - 0.111555816 * power_function(barometricPressure, 0.190284));       /*calculating altitude from barometric formula*/

  return altitudeFloat;
}

/*calculates altitude from barometric pressure and temperature as arguments*/
float bme280_calculate_altitude_hypsometric(uint8_t deviceAddress, uint32_t barometricPressure, float ambientTemperatureInC)
{
  float altitudeFloat;

  altitudeFloat = (((ambientTemperatureInC + 273.15) * (power_function((float)SEA_LEVEL_PRESSURE / (float)barometricPressure, (float)1 / 5.257) - 1)) / 0.0065) + altitudeOffsetHypsometric;
  //altitudeFloat = ((ambientTemperatureInC + 273.15) * (power_function((float)SEA_LEVEL_PRESSURE / (float)barometricPressure, 0.19022256) - 1)) * 153.8461538;
  return altitudeFloat;
}

/*returns a complete set of sensor readings and altitude calculation (quick). if bme280 is in sleep mode, gets data by setting bme280 to forced mode then back to sleep mode*/
sensors_t bme280_get_all(uint8_t deviceAddress)
{
  sensors_t sensors_value;

  operation_mode_t currentMode = bme280_get_mode(deviceAddress);

  if (currentMode == MODE_SLEEP)      /*gets a complete reading of sensors even if the chip is in sleep mode*/
  {
    bme280_set_mode(deviceAddress, MODE_FORCED);
    am_util_delay_ms(10);
  }

  sensors_value.temperature = (float)bme280_get_temperature(deviceAddress) / 100;
  sensors_value.pressure = bme280_get_pressure(deviceAddress);
  // sensors_value.altitude = bme280_calculate_altitude_quick(deviceAddress, sensors_value.pressure);
  sensors_value.humidity = (float)bme280_get_humidity(deviceAddress) / 1024;
  if (currentMode == MODE_SLEEP)
  {
    bme280_set_mode(deviceAddress, MODE_SLEEP);
  }

  return sensors_value;
}

/*returns bme280 mode of operation: sleep, normal or forced*/
operation_mode_t bme280_get_mode(uint8_t deviceAddress)
{
  operation_mode_t operationMode;
  uint8_t readMode;

  bme280_get_bits_in_register(deviceAddress, CTRL_MEAS_ADDRESS, &readMode, MODE_BIT, MODE_LENGTH);

  switch (readMode)
  {
    case 0X00:
      operationMode = MODE_SLEEP;
      break;
    case 0X01:
      operationMode = MODE_FORCED;
      break;
    case 0X02:
      operationMode = MODE_FORCED;
      break;
    case 0X03:
      operationMode = MODE_NORMAL;
      break;
    default:
      operationMode = MODE_ERROR;
      break;
  }
  return operationMode;
}

/*returns the current temperature oversampling*/
over_sampling_t bme280_get_temperature_oversampling(uint8_t deviceAddress)
{
  over_sampling_t tempOversampling;
  uint8_t readOS;

  bme280_get_bits_in_register(deviceAddress, CTRL_MEAS_ADDRESS, &readOS, OSRS_T_BIT, OSRS_T_LENGTH);

  if (readOS < OVERSAMPLING_ERROR)
  {
    tempOversampling = (over_sampling_t)readOS;
  }
  else
  {
    tempOversampling = OVERSAMPLING_ERROR;
  }
  return tempOversampling;
}

/*returns the current pressure oversampling*/
over_sampling_t bme280_get_pressure_oversampling(uint8_t deviceAddress)
{
  over_sampling_t pressureOversampling;
  uint8_t readOS;

  bme280_get_bits_in_register(deviceAddress, CTRL_MEAS_ADDRESS, &readOS, OSRS_P_BIT, OSRS_P_LENGTH);

  if (readOS < OVERSAMPLING_ERROR)
  {
    pressureOversampling = (over_sampling_t)readOS;
  }
  else
  {
    pressureOversampling = OVERSAMPLING_ERROR;
  }

  return pressureOversampling;
}

over_sampling_t bme280_get_humidity_oversampling(uint8_t deviceAddress)
{
  over_sampling_t humidityOversampling;
  uint8_t readOS;

  bme280_get_bits_in_register(deviceAddress, CTRL_HUM_ADDRESS, &readOS, OSRS_H_BIT, OSRS_H_LENGTH);

  if (readOS < OVERSAMPLING_ERROR)
  {
    humidityOversampling = (over_sampling_t)readOS;
  }
  else
  {
    humidityOversampling = OVERSAMPLING_ERROR;
  }

  return humidityOversampling;
}

/*returns the current standby time (for normal mode)*/
standby_time_t bme280_get_standby_time(uint8_t deviceAddress)
{
  standby_time_t standbyTime;
  uint8_t readTime;

  bme280_get_bits_in_register(deviceAddress, CONFIG_ADDRESS, &readTime, T_SB_BIT, T_SB_LENGTH);

  if (readTime < T_STANDBY_ERROR)
  {
    standbyTime = (standby_time_t)readTime;
  }
  else
  {
    standbyTime = T_STANDBY_ERROR;
  }
  return standbyTime;
}

/*returns the current IIR filter coefficient*/
iir_filter_t bme280_get_filter_coefficient(uint8_t deviceAddress)
{
  iir_filter_t filterCoefficient;
  uint8_t readFilter;

  bme280_get_bits_in_register(deviceAddress, CONFIG_ADDRESS, &readFilter, FILTER_BIT, FILTER_LENGTH);

  if (readFilter < FILTER_ERROR)
  {
    filterCoefficient = (iir_filter_t)readFilter;
  }
  else
  {
    filterCoefficient = FILTER_ERROR;
  }

  return filterCoefficient;
}

/*raw reading of temperature registers, uncompensated*/
static int32_t bme280_raw_temperature_data(uint8_t deviceAddress)
{
  uint8_t temp[3];
  bme280_read_array(deviceAddress, TEMP_MSB, temp, 3);
  return (int32_t)((((uint32_t)temp[0]) << 12) + (((uint32_t)temp[1]) << 4) + (((uint32_t)temp[2]) >> 4));
}

/*raw reading of pressure registers, uncompensated*/
static int32_t bme280_raw_pressure_data(uint8_t deviceAddress)
{
  uint8_t pressure[3];
  bme280_read_array(deviceAddress, PRESSURE_MSB, pressure, 3);
  return (int32_t)((((uint32_t)pressure[0]) << 12) + (((uint32_t)pressure[1]) << 4) + (((uint32_t)pressure[2]) >> 4));
}

/*raw reading of humidity registers, uncompensated*/
static uint32_t bme280_raw_humidity_data(uint8_t deviceAddress)
{
  uint8_t humidity[2];
  bme280_read_array(deviceAddress, HUM_MSB, humidity, 2);
  return (uint16_t)((((uint16_t)humidity[0]) << 8) + (uint16_t)humidity[1]);
}

/*a maximum number of two bme280 chips could be used on one i2c line
  this function returns a pointer to the complete struct of calibrations parameters for one chip,
  base on device address
*/
static calibration_param_t *bme280_which_dig(uint8_t deviceAddress)
{
  switch (deviceAddress)
  {
    case I2C_ADDRESS_1:
      return &dig1;
      break;
    case I2C_ADDRESS_2:
      return &dig2;
      break;
    default:
      return NULL;
      break;
  }
}

/*extracting calibration data in chip's "non volatile memory". we need to do this only once for each chip*/
static void bme280_get_calibration(uint8_t deviceAddress, calibration_param_t *dig)
{
  dig->T1 = bme280_read_calibration_word_unsigned(deviceAddress, T1_ADDRESS);
  dig->T2 = bme280_read_calibration_word_signed(deviceAddress, T2_ADDRESS);
  dig->T3 = bme280_read_calibration_word_signed(deviceAddress, T3_ADDRESS);
  dig->P1 = bme280_read_calibration_word_unsigned(deviceAddress, P1_ADDRESS);
  dig->P2 = bme280_read_calibration_word_signed(deviceAddress, P2_ADDRESS);
  dig->P3 = bme280_read_calibration_word_signed(deviceAddress, P3_ADDRESS);
  dig->P4 = bme280_read_calibration_word_signed(deviceAddress, P4_ADDRESS);
  dig->P5 = bme280_read_calibration_word_signed(deviceAddress, P5_ADDRESS);
  dig->P6 = bme280_read_calibration_word_signed(deviceAddress, P6_ADDRESS);
  dig->P7 = bme280_read_calibration_word_signed(deviceAddress, P7_ADDRESS);
  dig->P8 = bme280_read_calibration_word_signed(deviceAddress, P8_ADDRESS);
  dig->P9 = bme280_read_calibration_word_signed(deviceAddress, P9_ADDRESS);
  dig->H1 = bme280_read_calibration_byte_unsigned(deviceAddress, H1_ADDRESS);
  dig->H2 = bme280_read_calibration_word_signed(deviceAddress, H2_ADDRESS);
  dig->H3 = bme280_read_calibration_byte_unsigned(deviceAddress, H3_ADDRESS);

  uint8_t temporaryData[3];       /*temporaryData[0] 0XE4, temporaryData[1] 0XE5, temporaryData[2] 0XE6*/
  bme280_read_array (deviceAddress, 0XE4, temporaryData, 3);
  
  dig->H4 = ((int16_t)(temporaryData[1] & 0X0F)) | (((int16_t)((int8_t)temporaryData[0])) << 4);
  dig->H5 = (((int16_t)((int8_t)temporaryData[2])) << 4) | (int16_t)(temporaryData[1] >> 4);
  dig->H6 = bme280_read_calibration_byte_signed(deviceAddress, H6_ADDRESS); 
}

/*only used in get_calibration function*/
static uint16_t bme280_read_calibration_word_unsigned(uint8_t deviceAddress, uint8_t startRegisterAddress)
{
  uint8_t dataLSB, dataMSB;
  bme280_read_array (deviceAddress, startRegisterAddress, &dataLSB, 1);
  bme280_read_array (deviceAddress, startRegisterAddress + 1, &dataMSB, 1);
  return (uint16_t)((((uint16_t)dataMSB) << 8) + (uint16_t)dataLSB);
}

/*only used in get_calibration function*/
static int16_t bme280_read_calibration_word_signed(uint8_t deviceAddress, uint8_t startRegisterAddress)
{
  uint8_t dataLSB, dataMSB;
  bme280_read_array (deviceAddress, startRegisterAddress, &dataLSB, 1);
  bme280_read_array (deviceAddress, startRegisterAddress + 1, &dataMSB, 1);
  return (int16_t)((((int16_t)dataMSB) << 8) + (int16_t)dataLSB);
}

/*only used in get_calibration function*/
static int8_t bme280_read_calibration_byte_signed(uint8_t deviceAddress, uint8_t registerAddress)
{
  uint8_t data;
  bme280_read_array (deviceAddress, registerAddress, &data, 1);
  return (int8_t)data;
}

/*only used in get_calibration function*/
static uint8_t bme280_read_calibration_byte_unsigned(uint8_t deviceAddress, uint8_t registerAddress)
{
  uint8_t data;
  bme280_read_array (deviceAddress, registerAddress, &data, 1);
  return data;
}

/*sets a field of bits in a register*/
static void bme280_set_bits_in_register(uint8_t deviceAddress, uint8_t registerAddress, uint8_t fieldData, uint8_t fieldStartBitAddress, uint8_t fieldLength)
{
  uint8_t currentRegisterValue;
  uint8_t newRegisterValue;

  bme280_read_array(deviceAddress, registerAddress, &currentRegisterValue, 1);

  currentRegisterValue &= (((~0) << (fieldStartBitAddress + fieldLength)) | (~(~(0) << fieldStartBitAddress)));
  newRegisterValue = currentRegisterValue | (fieldData << fieldStartBitAddress);

  bme280_write_array(deviceAddress, registerAddress, &newRegisterValue, 1);
}

/*gets a field of bits from a register*/
static void bme280_get_bits_in_register(uint8_t deviceAddress, uint8_t registerAddress, uint8_t *fieldData, uint8_t fieldStartBitAddress, uint8_t fieldLength)
{
  uint8_t registerValue;

  bme280_read_array(deviceAddress, registerAddress, &registerValue, 1);

  registerValue &= ~(((~0) << (fieldStartBitAddress + fieldLength)) | (~(~(0) << fieldStartBitAddress)));
  *fieldData = registerValue >> fieldStartBitAddress;
}
