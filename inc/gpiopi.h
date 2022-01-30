/**
 * @file gpiopi.h
 * @brief raspberry pi gpio user level access
 * @details 
 * @copyright Copyright © Alkgrove Electronics 2018 Company Confidential
 * @author Robert Alkire 
 * @date 11/28/2021
 *
 **/
#ifndef __GPIOPI_H__
#define __GPIOPI_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define GPFSEL0 0x00
#define GPFSEL1 0x04
#define GPFSEL2 0x08
#define GPFSEL3 0x0C
#define GPFSEL4 0x10
#define GPFSEL5 0x14
#define GPSET0  0x1C
#define GPSET1  0x20
#define GPCLR0  0x28
#define GPCLR1  0x2C
#define GPLEV0  0x34
#define GPLEV1  0x38
#define GPEDS0  0x40
#define GPEDS1  0x44
#define GPREN0  0x4C
#define GPREN1  0x50
#define GPFEN0  0x58
#define GPFEN1  0x5C
#define GPHEN0  0x64
#define GPHEN1  0x68
#define GPLEN0  0x70
#define GPLEN1  0x74
#define GPAREN0 0x7C
#define GPAREN1 0x80
#define GPAFEN0 0x88
#define GPAFEN1 0x8C
#define GPPUD   0x94
#define GPPUDCLK0 0x98
#define GPPUDCLK1 0x9C

#define FSEL_INPUT 0x00
#define FSEL_OUTPUT 0x01
#define FSEL_ALT0 0x04
#define FSEL_ALT1 0x05
#define FSEL_ALT2 0x06
#define FSEL_ALT3 0x07
#define FSEL_ALT4 0x03
#define FSEL_ALT5 0x02

#define PULL_DISABLED 0x00
#define PULL_DOWN_ENABLE 0x01
#define PULL_UP_ENABLE 0x02

typedef enum {NO_EDGE, POSITIVE_EDGE, NEGATIVE_EDGE, BOTH_EDGES, HIGH_LEVEL, LOW_LEVEL} EVENT_TYPE_e;

static inline uint32_t gpio_get_function_select(void *base, uint8_t pin) 
{
    uint32_t *p = (uint32_t *) base + GPFSEL0 + (pin / 10);
    int pos = (pin % 10) * 3;
    return (*p >> pos) & 7;
}

static inline void gpio_set_function_select(void *base, uint8_t pin, uint32_t function) 
{
    uint32_t *p = (uint32_t *) base + GPFSEL0 + (pin / 10);
    
    int pos = (pin % 10) * 3;
    *p &= ~(7 << pos);
    *p |= function << pos;
}

static inline void gpio_set_output(void *base, uint8_t pin) 
{
    if (pin > 31) {
       *((uint32_t *) (base + GPSET1)) = 1 << (pin - 32);
    } else {
       *((uint32_t *) (base + GPSET0)) = 1 << pin;
    }
}
 
static inline void gpio_clear_output(void *base, uint8_t pin) 
{
    if (pin > 31) {
       *((uint32_t *) (base + GPCLR1)) = 1 << (pin - 32);
    } else {
       *((uint32_t *) (base + GPCLR0)) = 1 << pin;
    }
}

static inline void gpio_set_value(void *base, uint8_t pin, bool value) 
{
    if (pin > 31) {
        if (value) {
            *((uint32_t *) (base + GPLEV1)) |= 1 << (pin - 32);
        } else {
            *((uint32_t *) (base + GPLEV1)) &= ~(1 << (pin - 32));
        }        
    } else {
        if (value) {
            *((uint32_t *) (base + GPLEV0)) |= 1 << pin;
        } else {
            *((uint32_t *) (base + GPLEV0)) &= ~(1 << pin);
        }        
    }
}
    

static inline bool gpio_get_value(void *base, uint8_t pin) 
{
    if (pin > 31) {
       return (((*((uint32_t *) (base + GPLEV1)) >> (pin - 32)) & 1) == 1) ? true : false;
    } else {
       return (((*((uint32_t *) (base + GPLEV0)) >> pin) & 1) == 1) ? true : false;
    }
}

        

void *gpio_open(void);

void gpio_set_event(void *base, uint8_t pin, EVENT_TYPE_e event, bool async_edge);


#endif /* __GPIOPI_H__ */