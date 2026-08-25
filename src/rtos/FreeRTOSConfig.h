#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* --- Βασικές Ρυθμίσεις Πυρήνα --- */
#define configUSE_PREEMPTION                    1
#define configCPU_CLOCK_HZ                      ( ( unsigned long ) 20000000 ) 
#define configTICK_RATE_HZ                      ( ( TickType_t ) 100 )
#define configMAX_PRIORITIES                    ( 5 )
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 256 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 10 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 ( 16 )

/* --- V2.0 ΕΝΕΡΓΟΠΟΙΗΣΕΙΣ (ΓΙΑ PS & LOGGING) --- */
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1
#define configCHECK_FOR_STACK_OVERFLOW          2 

#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1

/* --- Interrupt Priority Settings (Cortex-M3) --- */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* --- API Functions (ΕΔΩ ΕΓΙΝΑΝ ΟΙ ΔΙΟΡΘΩΣΕΙΣ) --- */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1  /* ΑΠΑΡΑΙΤΗΤΟ ΓΙΑ UART_PUTS_SAFE */
#define INCLUDE_xTaskGetIdleTaskHandle          1  /* ΒΟΗΘΑΕΙ ΣΤΟ vTaskList (PS) */
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_uxTaskGetStackHighWaterMark2    1

/* --- Cortex-M3 Interrupt Vector Definitions --- */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */