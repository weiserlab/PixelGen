///
/// @file artemis_imu.c
///

#include "bh1750fvi.h"
#include "artemis_lux.h"
#include <am_util_stdio.h>
///
///
///
void artemis_lux_initialize(void)
{
    artemis_bh1750_initialize();
}

///
///
///
void artemis_lux_update(uint64_t elapsed_us)
{
    set_i2c_bh1750();
    lightSensor_begin(ADDRESS1, ONE_TIME_AUTO);
    am_util_stdio_printf("Lux = %d\n", lightSensor_meter());
}
