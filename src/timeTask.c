/*
 * @file timeTask.c
 * @brief updates Nixie tube clock time
 * @details for raspberry pi 3B+
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
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
#include <time.h>
#include <unistd.h>
#include <pthread.h>


#include "nixieclock.h"
#include "gpiopi.h"
#include "spipi.h"

/*
 * @brief setNixie(int fd, void *map, bool colon, char *str)
 *
 * @param[in] fd - spi file descriptor
 * @param[in] map - gpio map base address
 * @param[in] pin - gpio pin number
 * @param[in] colon - true if colon is displayed, false they are off
 * @param[in] str - pointer to 6 character string or NULL if bytes are cleared
 * str must be six characters from '0' through '9'
 */

void setNixie(int fd, void *map, int pin, bool colon, char *str) {
    llconv_t nixie;
    int i;
    uint8_t dummy[8];
    nixie.ll = 0;
    if (str != NULL) {
        if (colon) nixie.ll |= 3;
        for (i = 5; i >= 3; i--) {
            nixie.ll <<= 10;
            if ((str[i] >= '0') && (str[i] <= '9')) {
                nixie.ll |= (1 << (str[i] - '0'));
        	}
        }
        nixie.ll <<= 2;
        if (colon) nixie.ll |= 3;
        for (i = 2; i >= 0; i--) {
            nixie.ll <<= 10;
            if ((str[i] >= '0') && (str[i] <= '9')) {
            	nixie.ll |= (1 << (str[i] - '0'));
        	}
        }
        nixie.ll = bswap_64(nixie.ll);
    }
    gpio_clear_output(map, pin); /* set LE low */
    spi_transfer(fd, nixie.b, dummy, sizeof(uint64_t));
    gpio_set_output(map, pin); /* set LE high */
}

/*
 * @brief testNixie(int fd, void *map, int pin)
 * Simple test of the nixie tubes. Just goes through all the numbers.
 * Where this is helpful is identifying bad pins in the socket or the tube is bad. 
 * @param[in] fd - spi file descriptor
 * @param[in] map - gpio map base address
 * @param[in] pin - gpio pin number
 */

void testNixie(int fd, void *map, int pin) {
    char display[7] = "123456";
    bool colon = false;
    struct timespec delay = {.tv_sec = 0, .tv_nsec = 500000000L };
    for (int i = 0; i < 10; i++) {
        setNixie(fd, map, pin, colon, display);
        nanosleep(&delay, NULL);
        for (int i = 0; i < 6; i++) {
            display[i]++;
            if (display[i] > '9') display[i] = '0';
        }
        colon = !colon;
    }
}
/*
 * @brief timeTask updates Nixie clock time
 *
 * @param[in] threadid unused
 */
void *timeTask(void *threadid)
{
    int rv;
    bool done = false;
    int spifd;
    void *gpiomap;
    struct tm *loctime;
    time_t UTCtime;
    struct timespec currentTime, lastTime, remTime;
    uint8_t timestr[7];

    spifd = spi_open();
    gpiomap = gpio_open();
    if (gpiomap == NULL) {
        errprint("Failed to open GPIO");
        notifyToTerminate();
        pthread_exit((void *)EXIT_FAILURE);
    }
    gpio_set_function_select(gpiomap, LE, FSEL_OUTPUT);
    setNixie(spifd, gpiomap, LE, false, NULL); // clear nixie to initialize

    testNixie(spifd, gpiomap, LE); // simple sequence of all nixies to test
    clock_gettime(CLOCK_REALTIME, &lastTime);
    while (!done) {
        /* Wait for seconds to change */
        do {
            clock_gettime(CLOCK_REALTIME, &currentTime);
        } while (currentTime.tv_sec == lastTime.tv_sec);
        lastTime = currentTime;
        /* convert to local time and a string to send to nixie */
        loctime = localtime(&currentTime.tv_sec);
        strftime(timestr, sizeof(timestr), "%H%M%S", loctime);
        setNixie(spifd, gpiomap, LE, true, timestr);
        /* we are done get time stamp and wait close to end of the second */
        clock_gettime(CLOCK_REALTIME, &currentTime);        
        remTime.tv_sec = currentTime.tv_sec;
        remTime.tv_nsec = (currentTime.tv_nsec > CONSIDERATE_SLEEP) ? 0L : CONSIDERATE_SLEEP - currentTime.tv_nsec; 
        do {
            currentTime = remTime;
            rv = clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME, &currentTime, &remTime);
            /* This assertion is if rv returns an error for a bad value in currentTime */
            /* This shouldn't happen except for programming bug */
            assert((rv == 0) || (rv == EINTR));
        } while (rv == EINTR);    
        done = isTerminate();
    } 
    setNixie(spifd, gpiomap, LE, false, NULL); // clear nixie to clean up

}