/**
 * @file gpiopi.h
 * @brief 
 * @details 
 * @copyright Copyright © Alkgrove Electronics 2018 Company Confidential
 * @author Robert Alkire 
 * @date 
 *
 **/
#include "gpiopi.h"

void *gpio_open(void) {
    int fd;
    void *map;

    if ((fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) return NULL;
    /* BCM2837 has 180 bytes of GPIO registers */
    map = mmap(NULL, 180, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (map == MAP_FAILED) return NULL;
    return map;
}

void gpio_set_event(void *base, uint8_t pin, EVENT_TYPE_e event, bool async_edge) 
{
    int re0 = GPREN0;
    int re1 = GPREN1;
    int fe0 = GPFEN0;
    int fe1 = GPFEN1;
    if (async_edge) {
        re0 = GPAREN0;
        re1 = GPAREN1;
        fe0 = GPAFEN0;
        fe1 = GPAFEN1;
    }
    if (pin > 31) {
       *((uint32_t *) (base + GPEDS1)) = 1 << (pin - 32);
    } else {
       *((uint32_t *) (base + GPEDS0)) = 1 << pin;
    }
    if ((event == POSITIVE_EDGE) || (event == BOTH_EDGES)) {
        if (pin > 31) {
           *((uint32_t *) (base + re1)) = 1 << (pin - 32);
        } else {
           *((uint32_t *) (base + re0)) = 1 << pin;
        }
    }
    if ((event == NEGATIVE_EDGE) || (event == BOTH_EDGES)) {
        if (pin > 31) {
           *((uint32_t *) (base + fe1)) = 1 << (pin - 32);
        } else {
           *((uint32_t *) (base + fe0)) = 1 << pin;
        }
    }
    if (event == HIGH_LEVEL) {
        if (pin > 31) {
           *((uint32_t *) (base + GPHEN1)) = 1 << (pin - 32);
        } else {
           *((uint32_t *) (base + GPHEN0)) = 1 << pin;
        }
    }
    if (event == LOW_LEVEL) {
        if (pin > 31) {
           *((uint32_t *) (base + GPLEN1)) = 1 << (pin - 32);
        } else {
           *((uint32_t *) (base + GPLEN0)) = 1 << pin;
        }
    }
    
        
}    