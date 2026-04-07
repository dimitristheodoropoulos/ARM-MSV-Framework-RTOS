#include "gpio.h"

#define RCGC2       (*(volatile unsigned int *)0x400FE108)
#define GPIOF_BASE  0x40025000
#define GPIOF_DIR   (*(volatile unsigned int *)(GPIOF_BASE + 0x400))
#define GPIOF_DEN   (*(volatile unsigned int *)(GPIOF_BASE + 0x51C))
#define GPIOF_DATA  (*(volatile unsigned int *)(GPIOF_BASE + 0x3FC))

void gpio_init(void) {
    RCGC2 |= (1 << 5);  // Ενεργοποίηση clock για το GPIOF
    volatile int delay = 1000; 
    while(delay--);
    
    GPIOF_DIR |= (1 << 3); // PF3 ως έξοδος (LED στο LM3S6965)
    GPIOF_DEN |= (1 << 3); // Ψηφιακή ενεργοποίηση
}

/**
 * Γενική συνάρτηση εγγραφής σε GPIO
 */
void gpio_write(int pin, int value) {
    if (value) {
        GPIOF_DATA |= (1 << pin);
    } else {
        GPIOF_DATA &= ~(1 << pin);
    }
}

/**
 * Συγκεκριμένη συνάρτηση για το LED (Wrapper της gpio_write)
 */
void gpio_set_led(int on) {
    gpio_write(3, on); // Το LED είναι στο Pin 3
}