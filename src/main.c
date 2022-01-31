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
static bool daemonmode = false;

bool isDaemon(void) {
    return daemonmode;
}

void errprint(const char *format, ...)
{
    va_list args;
    
    va_start( args, format );
    if (daemonmode) {
        syslog(LOG_ERR, format, args);
    } else {
        fprintf(stderr, format, args );
        fprintf(stderr, "\n");
    }
}
    
void terminator_handler(int signum)
{
    notifyToTerminate();
}

static struct sigaction new_action, old_action;

int main(int argc, char *argv[])
{	
    int err;
    int rv;
    pid_t pid = 0;
    pid_t fpid;
	int fd;
    char str[128];
    int spifd;
    void *gpiomap;
    struct tm *loctime;
    time_t UTCtime;
    struct timespec currentTime, lastTime, remTime;
    uint8_t timestr[7];
         
    if ((argc == 2) && (strncmp(argv[1], "--daemon", 8) == 0)) {
        daemonmode = true;
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Unable to fork daemon process (%s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        /* exit parent */
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
        if (setsid() < 0) {
            fprintf(stderr, "Unable to create a new session (%s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        signal(SIGCHLD, SIG_IGN);
        
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Unable to fork second daemon process (%s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }

        umask(0);
        
        rv = chdir("/");
        if (rv != 0) {
            fprintf(stderr, "Unable to change directory (%s)\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        for (fd = 0; fd < sysconf(_SC_OPEN_MAX); fd++) close(fd);
        
        stdin = fopen("/dev/null", "r");
        stdout = fopen("/dev/null", "w+");
        stderr = fopen("/dev/null", "w+");

        openlog (APP, LOG_PERROR | LOG_PID | LOG_ODELAY, LOG_USER);
        setlogmask (LOG_UPTO (LOG_INFO));
        
        fpid = getpid();
        snprintf(str, sizeof(str), "%d", fpid);
        if ((fd = open(PID_FILENAME, O_CREAT | O_EXCL | O_WRONLY, 0640)) < 0) {
            syslog(LOG_ERR, "unable to open file %s (%s)", PID_FILENAME, strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (lockf(fd, F_TLOCK, 0) < 0) {
            syslog(LOG_ERR, "unable to lock file %s (%s)", PID_FILENAME, strerror(errno));
			exit(EXIT_FAILURE);
		}
        rv = write(fd, str, strlen(str));
        if (rv < 0) {
            syslog(LOG_ERR, "unable to write file %s (%s)", PID_FILENAME, strerror(errno));
			exit(EXIT_FAILURE);
		}
    } else {
        daemonmode = false;
    }
 	new_action.sa_handler = terminator_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    
    sigaction (SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) sigaction (SIGINT, &new_action, NULL);
    sigaction (SIGHUP, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) sigaction (SIGHUP, &new_action, NULL);
    sigaction (SIGTERM, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN) sigaction (SIGTERM, &new_action, NULL);
        
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);
    /* get config file - if can't find file or has errors, program will exit from parseconfig() */
    if (daemonmode) syslog(LOG_INFO, "Starting nixie clock daemon");
    if (pthread_create(&timeThread, &attributes, timeTask, NULL)){
        errprint("clock time unable to create thread");
  	} else if (pthread_create(&ledThread, &attributes, ledTask, NULL)) {
        errprint("clock LED unable to create thread");
  	} else {
    if (daemonmode) syslog(LOG_INFO, "Starting threads");
		pthread_join(timeThread, NULL);
    	pthread_join(ledThread, NULL);
  	}
    closelog();
    pthread_attr_destroy(&attributes);
    unlink(PID_FILENAME);
    if (daemonmode) {
        syslog(LOG_INFO, "Nixie Clock Exit");
    } else {
        fprintf(stdout, "Nixie Clock Exit\n");
    }

}

