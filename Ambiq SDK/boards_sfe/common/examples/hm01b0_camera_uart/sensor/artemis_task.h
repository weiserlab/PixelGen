///
/// @file artemis_task.h
///

#ifndef ARTEMIS_TASK_H
#define ARTEMIS_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum e_artemis_task_id_t
{
    ARTEMIS_TASK_ID_LUX,
    ARTEMIS_TASK_ID_ENV,
    ARTEMIS_TASK_ID_COUNT
} artemis_task_id_t;

typedef struct s_artemis_task_t
{
    const char *name;
    void (*initialize)(void);
    void (*run)(); //original below
    //uint64_t elapsed_us
} artemis_task_t;

artemis_task_t *artemis_task_get(artemis_task_id_t id);

#ifdef __cplusplus
}
#endif

#endif
