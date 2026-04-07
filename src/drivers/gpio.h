#ifndef GPIO_H
#define GPIO_H

void gpio_init(void);
void gpio_write(int pin, int value);
void gpio_set_led(int on); // Την κρατάμε ως wrapper για συμβατότητα

#endif