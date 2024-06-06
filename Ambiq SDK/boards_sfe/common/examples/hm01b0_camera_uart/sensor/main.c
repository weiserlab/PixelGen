///
/// @file artemis_main.c
///
#include "artemis_debug.h"
#include "artemis_mcu.h"
#include "artemis_scheduler.h"
#include "artemis_time.h"
#include <stdlib.h> //standard library for general purpose functions
#include "am_util_stdio.h"


int main(void)
{
    artemis_mcu_initialize();
    artemis_time_initialize();
    artemis_scheduler_run();
    return(EXIT_SUCCESS);
}
