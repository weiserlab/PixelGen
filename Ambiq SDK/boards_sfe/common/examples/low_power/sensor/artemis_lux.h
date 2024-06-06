///
/// @file artemis_imu.h
///

#ifndef ARTEMIS_BH1750_H
#define ARTEMIS_BH1750_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


void artemis_lux_initialize(void);
void artemis_lux_update(uint64_t elapsed_us);

#ifdef __cplusplus
}
#endif

#endif