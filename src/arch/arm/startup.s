/**
 * startup.s — ARM Cortex-M3 Vector Table & Reset Logic
 */
.syntax unified
.thumb

/* --- External symbols --- */
.extern main
.extern _stack_top
.extern _bss_start
.extern _bss_end
.extern _data_load
.extern _data_start
.extern _data_end

/* Handlers defined in C */
.extern HardFault_Handler
.extern SVC_Handler
.extern PendSV_Handler
.extern SysTick_Handler

.global _start
.global _vectors

.section .vectors, "a", %progbits
.align 2

_vectors:
    .word _stack_top            /* 0x00 Initial Stack Pointer */
    .word _start                /* 0x04 Reset                 */
    .word _hang                 /* 0x08 NMI                   */
    .word HardFault_Handler     /* 0x0C HardFault             */
    .word _hang                 /* 0x10 MemManage             */
    .word _hang                 /* 0x14 BusFault              */
    .word _hang                 /* 0x18 UsageFault            */
    .word 0, 0, 0, 0            /* 0x1C - 0x28 Reserved       */
    .word SVC_Handler           /* 0x2C SVCall (FreeRTOS)     */
    .word _hang                 /* 0x30 DebugMonitor          */
    .word 0                     /* 0x34 Reserved              */
    .word PendSV_Handler        /* 0x38 PendSV (FreeRTOS)     */
    .word SysTick_Handler       /* 0x3C SysTick (FreeRTOS)    */

.section .text

/* Force Thumb mode for all handlers */
.type _hang, %function
.thumb_func
_hang:
    b _hang

.type _start, %function
.thumb_func
_start:
    /* Set stack pointer */
    ldr r0, =_stack_top
    mov sp, r0

    /* Zero-initialize .bss */
    ldr r1, =_bss_start
    ldr r2, =_bss_end
    mov r3, #0
.bss_loop:
    cmp r1, r2
    bge .bss_done
    str r3, [r1], #4
    b   .bss_loop
.bss_done:

    /* Copy .data from FLASH to RAM */
    ldr r1, =_data_load
    ldr r2, =_data_start
    ldr r3, =_data_end
.data_loop:
    cmp r2, r3
    bge .data_done
    ldr r4, [r1], #4
    str r4, [r2], #4
    b   .data_loop
.data_done:

    /* Jump to C main */
    bl main
    b _hang