#include "rpi.h"

#define GPFSEL0     ((volatile uint*)(GPIO_BASE + 0x00))
#define GPFSEL1     ((volatile uint*)(GPIO_BASE + 0x04))
#define GPFSEL2     ((volatile uint*)(GPIO_BASE + 0x08))
#define GPFSEL3     ((volatile uint*)(GPIO_BASE + 0x0C))
#define GPFSEL4     ((volatile uint*)(GPIO_BASE + 0x10))
#define GPFSEL5     ((volatile uint*)(GPIO_BASE + 0x14))
#define GPSET0      ((volatile uint*)(GPIO_BASE + 0x1C))
#define GPSET1      ((volatile uint*)(GPIO_BASE + 0x20))
#define GPCLR0      ((volatile uint*)(GPIO_BASE + 0x28))
#define GPCLR1      ((volatile uint*)(GPIO_BASE + 0x2C))
#define GPLEV0      ((volatile uint*)(GPIO_BASE + 0x34))
#define GPLEV1      ((volatile uint*)(GPIO_BASE + 0x38))
#define GPEDS0      ((volatile uint*)(GPIO_BASE + 0x40))
#define GPEDS1      ((volatile uint*)(GPIO_BASE + 0x44))
#define GPREN0      ((volatile uint*)(GPIO_BASE + 0x4C))
#define GPREN1      ((volatile uint*)(GPIO_BASE + 0x50))
#define GPFEN0      ((volatile uint*)(GPIO_BASE + 0x58))
#define GPFEN1      ((volatile uint*)(GPIO_BASE + 0x5C))
#define GPHEN0      ((volatile uint*)(GPIO_BASE + 0x64))
#define GPHEN1      ((volatile uint*)(GPIO_BASE + 0x68))
#define GPLEN0      ((volatile uint*)(GPIO_BASE + 0x70))
#define GPLEN1      ((volatile uint*)(GPIO_BASE + 0x74))
#define GPPUD       ((volatile uint*)(GPIO_BASE + 0x94))
#define GPPUDCLK0   ((volatile uint*)(GPIO_BASE + 0x98))
#define GPPUDCLK1   ((volatile uint*)(GPIO_BASE + 0x9C))
#define NUM_GPIO    60


static void (*gpio_handlers[NUM_GPIO])() = {0};


static void delay(uint n) {
    while (n--)
        asm volatile("nop");
}

static void gpio_setegde(uint pin, uint mod) {
    int mask;

    if (pin >= 32) {
        mask = 1 << (pin - 32);

        switch (mod) {
        case GPIO_RISING_EDGE:
            *GPREN1 |= mask;
            break;
        case GPIO_FALLING_EDGE:
            *GPFEN1 |= mask;
            break;
        case GPIO_HIGH_EDGE:
            *GPHEN1 |= mask;
            break;
        case GPIO_LOW_EDGE:
            *GPLEN1 |= mask;
            break;
        case GPIO_NO_EDGE:
            *GPREN1 &= ~mask;
            *GPFEN1 &= ~mask;
            *GPHEN1 &= ~mask;
            *GPLEN1 &= ~mask;
            break;
        }
    } else {
        mask = 1 << pin;

        switch (mod) {
        case GPIO_RISING_EDGE:
            *GPREN0 |= mask;
            break;
        case GPIO_FALLING_EDGE:
            *GPFEN0 |= mask;
            break;
        case GPIO_HIGH_EDGE:
            *GPHEN0 |= mask;
            break;
        case GPIO_LOW_EDGE:
            *GPLEN0 |= mask;
            break;
         case GPIO_NO_EDGE:
            *GPREN0 &= ~mask;
            *GPFEN0 &= ~mask;
            *GPHEN0 &= ~mask;
            *GPLEN0 &= ~mask;
            break;
        }
    }
}

static int gpio_getedge(uint pin) {
    if (pin >= 32)
        return ((*GPEDS1) >> (pin - 32)) & 1;
    else
        return ((*GPEDS0) >> pin) & 1;
}

static void gpio_clearedge(uint pin) {
    if (pin >= 32)
        *GPEDS1 = 1 << (pin - 32);
    else
        *GPEDS0 = 1 << pin;
}

void gpio_irq_handler() {
    int i;
    for (i = 0; i < NUM_GPIO; ++i){
        if (gpio_handlers[i] && gpio_getedge(i)) {
            gpio_handlers[i]();
            gpio_clearedge(i);
            return;
        }
    }
}

void gpio_mode(uint pin, uint mode) {
    uint pull;

    pull = (mode & 0xf0) >> 4;
    mode = mode & 0x0f;

    if (pin < 10)      *GPFSEL0 &= ~(7 << ((pin - 0) * 3));
    else if (pin < 20) *GPFSEL1 &= ~(7 << ((pin - 10) * 3));
    else if (pin < 30) *GPFSEL2 &= ~(7 << ((pin - 20) * 3));
    else if (pin < 40) *GPFSEL3 &= ~(7 << ((pin - 30) * 3));
    else if (pin < 50) *GPFSEL4 &= ~(7 << ((pin - 40) * 3));
    else if (pin < 60) *GPFSEL5 &= ~(7 << ((pin - 50) * 3));

    if (pin < 10)      *GPFSEL0 |= mode << ((pin - 0) * 3);
    else if (pin < 20) *GPFSEL1 |= mode << ((pin - 10) * 3);
    else if (pin < 30) *GPFSEL2 |= mode << ((pin - 20) * 3);
    else if (pin < 40) *GPFSEL3 |= mode << ((pin - 30) * 3);
    else if (pin < 50) *GPFSEL4 |= mode << ((pin - 40) * 3);
    else if (pin < 60) *GPFSEL5 |= mode << ((pin - 50) * 3);

    *GPPUD = pull;

    delay(200);
    if (pin >= 32)
        *GPPUDCLK1 = 1 << (pin - 32);
    else
        *GPPUDCLK0 = 1 << pin;

    delay(200);
    if (pin >= 32)
        *GPPUDCLK1 = 0;
    else
        *GPPUDCLK0 = 0;
}

void gpio_write(uint pin, uint val) {
    if (pin >= 32) {
        if (val)
            *GPSET1 =1 << (pin - 32);
        else
            *GPCLR1 =1 << (pin - 32);
    } else {
        if (val)
            *GPSET0 = 1 << pin;
        else
            *GPCLR0 = 1 << pin;
    }
}

uint gpio_read(uint pin) {
    if (pin >= 32)
        return ((*GPLEV1) >> (pin - 32)) & 1;
    else
        return ((*GPLEV0) >> pin) & 1;
}

void gpio_enint(uint pin, void (*h)(), uint edge) {
    if (edge)
        gpio_handlers[pin] = h;
    else
        gpio_handlers[pin] = 0;

    gpio_setegde(pin, edge);
}
