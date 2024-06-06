///
/// @file artemis_env.c
///

#include "bme280.h"
#include "artemis_env.h"
#include <am_util_stdio.h>
///
///
///
void artemis_env_initialize(void)
{
    artemis_bme280_initialize();
}

void artemis_env_update(void)
{
    set_i2c_bme280();
    sensors_t sensorData = bme280_get_all(I2C_ADDRESS_1);
    am_util_stdio_printf("Temperature = %.2f\n", sensorData.temperature);
    am_util_stdio_printf("Humidity = %.2f\n", sensorData.humidity);
    delay_function(2000);
}