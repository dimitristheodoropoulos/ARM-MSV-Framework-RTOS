#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <stdint.h>

typedef enum {
    HEALTH_TASK_CLI = 0,
    HEALTH_TASK_AI,
    HEALTH_TASK_COUNT
} health_task_id_t;

/**
 * health_monitor_init() — initialize the health monitor.
 *
 * The initial state is considered healthy to preserve the existing
 * watchdog startup semantics.
 */
void health_monitor_init(void);

/**
 * health_monitor_heartbeat() — report activity from a monitored task.
 */
void health_monitor_heartbeat(health_task_id_t task);

/**
 * health_monitor_clear() — clear the current heartbeat window.
 */
void health_monitor_clear(void);

/**
 * health_monitor_get_flags() — return the current heartbeat flags.
 */
uint32_t health_monitor_get_flags(void);

/**
 * health_monitor_all_healthy() — return non-zero when all monitored
 * tasks have reported a heartbeat during the current watchdog window.
 */
uint8_t health_monitor_all_healthy(void);

#endif /* HEALTH_MONITOR_H */
