#include "health_monitor.h"

#define HEALTH_ALL_TASKS ((1u << HEALTH_TASK_COUNT) - 1u)

static volatile uint32_t health_flags = 0;

void health_monitor_init(void)
{
    /*
     * Preserve the existing watchdog startup semantics:
     * the first watchdog window starts in the healthy state.
     */
    health_flags = HEALTH_ALL_TASKS;
}

void health_monitor_heartbeat(health_task_id_t task)
{
    if (task < HEALTH_TASK_COUNT) {
        health_flags |= (1u << task);
    }
}

void health_monitor_clear(void)
{
    health_flags = 0;
}

uint32_t health_monitor_get_flags(void)
{
    return health_flags;
}

uint8_t health_monitor_all_healthy(void)
{
    return ((health_flags & HEALTH_ALL_TASKS) == HEALTH_ALL_TASKS) ? 1u : 0u;
}
