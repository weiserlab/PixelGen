///
/// @file artemis_task.c
///

#include "artemis_lux.h"
#include "artemis_env.h"
#include "artemis_task.h"

#define ARTEMIS_TASK_ENTRY(_name, _initialize, _run) { \
    .name = _name, \
    .initialize = _initialize, \
    .run = _run \
}

typedef struct s_module_t
{
    artemis_task_t tasks[ARTEMIS_TASK_ID_COUNT]; //contains an array of tasks
} module_t;

static module_t module = {
    {
        [ARTEMIS_TASK_ID_LUX] = ARTEMIS_TASK_ENTRY("LUX", artemis_lux_initialize, artemis_lux_update),
        [ARTEMIS_TASK_ID_ENV] = ARTEMIS_TASK_ENTRY("ENV", artemis_env_initialize, artemis_env_update),
    }
}; //this module is an array of tasks, each element is task

///
///
///
artemis_task_t *artemis_task_get(artemis_task_id_t id)
{
    return(&module.tasks[id]);
}
