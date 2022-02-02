/*
 *
 *  main.c
 *  C file for updating Nixie clock and managing colors
 *  
 *  Copyright © Bob Alkire 10/25/2021
 *  raspberry pi
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
#include "nixieclock.h"
#include "spipi.h"

#include "ws2811.h"

#define PID_FILENAME "/run/pixie.pid"
/* CONSIDERATE_SLEEP is the time in nanoseconds that this daemon should yield before waking up again. 
 * it needs to wake up prior to next second or the clock misses a beat
 * I measured about 130usec is actually how long a raspberry pi took to wake up so this should be a healthy margin
 * your mileage may vary. If it skips seconds, this may be adjusted down. 
 */
 /* global - not to be change anywhere except in main */
terminate_t terminate = {.mutex = PTHREAD_MUTEX_INITIALIZER, .kill = false};
ledrollhead_t *ledrollhead = NULL;
/* non-global */
pthread_attr_t attributes;
pthread_t timeThread;
pthread_t ledThread;



    
void terminator_handler(int signum)
{
    notifyToTerminate();
}

static struct sigaction new_action, old_action;

int main(int argc, char *argv[])
{	        
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
    /* get config file - if can't find file or has errors, program will exit from parseconfig() */
    if (pthread_create(&timeThread, &attributes, timeTask, NULL)){
        fprintf(stderr,"clock time unable to create thread\n");
  	} else if (pthread_create(&ledThread, &attributes, ledTask, NULL)) {
        fprintf(stderr,"clock LED unable to create thread\n");
  	} else {
		pthread_join(timeThread, NULL);
    	pthread_join(ledThread, NULL);
  	}
    closelog();
    pthread_attr_destroy(&attributes);
    return 0;
}

