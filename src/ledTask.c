/*
 * @file ledTask.c
 * @brief takes configuration data and controls Nixie clock LEDs
 * @details  for raspberry pi 3B+
 * @copyright Copyright © Alkgrove Electronics 2018 Company Confidential
 * @author Robert Alkire 
 * @date  1/1/2022
 *
 * @par Redistribution and use in source and binary forms, with or without modification, are permitted 
 * provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions   
 * and the following disclaimer.  
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions  
 * and the following disclaimer in the documentation and/or other materials provided with the 
 * distribution.  
 * 3. Neither the name of the copyright holder nor the names of its contributors may be 
 * used to endorse or promote products derived from this software without specific prior written 
 * permission.  
 * 
 * @par THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */
 
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <assert.h>
#include <sys/stat.h>
#include <byteswap.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h> 
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>

#include "nixieclock.h"

#include "ws2811.h"

ws2811_t ledmodule = {
    .freq = WS2811_TARGET_FREQ,
    .dmanum = 10,
    .channel = {
        [0] = {
            .gpionum = PWM1,
            .count = LEDCOUNT,
            .invert = 0,
            .brightness = 255,
            .strip_type = SK6812_STRIP,
        },
        [1] = {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
            .brightness = 0,
        },
    },
};

uint8_t interpolateColor(uint8_t color, uint8_t nextcolor, int pos, int max) 
{
    return (((uint32_t) color * (max - pos)) + ((uint32_t) nextcolor * pos))/max;
}

int32_t interpolateRGB(uint32_t color, uint32_t nextcolor, int pos, int max)
{
    uint8_t r, g, b;
    r = interpolateColor((uint8_t) (color >> 16), (uint8_t) (nextcolor >> 16), pos, max);
    g = interpolateColor((uint8_t) (color >> 8), (uint8_t) (nextcolor >> 8), pos, max);
    b = interpolateColor((uint8_t) (color >> 0), (uint8_t) (nextcolor >> 0), pos, max);
    return ((((uint32_t) r) << 16) | (((uint32_t) g) << 8) | (((uint32_t) b) << 0));
}

void *ledTask(void *threadid)
{
    int rv;
    bool done = false;
    ledroll_t *p;
    ledroll_t *nextp;
    int max;
    struct timespec stepDelay = {.tv_sec = 0, .tv_nsec = (INTERPOLATE_STEP * 1000000L) }; 
    struct timespec delay;
    ledrollhead_t *ledrollhead;
    
    if ((rv = ws2811_init(&ledmodule)) != WS2811_SUCCESS) {
        fprintf(stderr,"ws2811_init failed: %s\n", ws2811_get_return_t_str(rv));
        notifyToTerminate();
        pthread_exit((void *)EXIT_FAILURE);
    } 
	ledrollhead = parseconfig();
    if (ledrollhead == NULL) {
        fprintf(stderr,"configuration file not valid\n");
        notifyToTerminate();
        pthread_exit((void *)EXIT_FAILURE);   
    }
	setColon(ledrollhead->colon);
    while(!done) {
        p = &(ledrollhead->roll[ledrollhead->pos++]);
        if (ledrollhead->pos >= ledrollhead->count) ledrollhead->pos = 0; /* looping roll */
        nextp = &(ledrollhead->roll[ledrollhead->pos]);
        if (p->isFast) {
            delay.tv_sec = p->delay/1000;
            delay.tv_nsec = (p->delay % 1000) * 1000000L;
            for (int i = 0; i < LEDCOUNT; i++) ledmodule.channel[0].leds[i] = p->color[i];
            if ((rv = ws2811_render(&ledmodule)) != WS2811_SUCCESS) {
    	        fprintf(stderr,"ws2811_render failed: %s\n", ws2811_get_return_t_str(rv));
                notifyToTerminate();
                break;
            }
            nanosleep(&delay, NULL);
        } else {
            max = p->delay/INTERPOLATE_STEP;
            for (int i = 0; i < max; i++) {
                for (int j = 0; j < LEDCOUNT; j++) {
                    ledmodule.channel[0].leds[j] = interpolateRGB(p->color[j], nextp->color[j], i, max);
                }
                if ((rv = ws2811_render(&ledmodule)) != WS2811_SUCCESS) {
    	            fprintf(stderr,"ws2811_render failed: %s\n", ws2811_get_return_t_str(rv));
                    notifyToTerminate();
                    break;
                }
                nanosleep(&stepDelay, NULL);
            }
        }
        done = isTerminate();
    }
    /* to finish, turn off all LEDs */
    for (int i = 0; i < LEDCOUNT; i++) ledmodule.channel[0].leds[i] = 0;
    if ((rv = ws2811_render(&ledmodule)) != WS2811_SUCCESS) {
    	fprintf(stderr,"ws2811_render failed: %s\n", ws2811_get_return_t_str(rv));
        notifyToTerminate();
    }
    ws2811_fini(&ledmodule);
    free(ledrollhead->roll);
    free(ledrollhead);
    return NULL;
}