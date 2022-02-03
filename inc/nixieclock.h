/*
 * @file nixieclock.h
 * @brief defines for GRA-AFCH Nixie clock raspberry pi adapter
 * @copyright Copyright © Alkgrove Electronics 2018 Company Confidential
 * @author Robert Alkire 
 * @date 11/29/2021
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
 */
 
#ifndef __NIXIECLOCK_H__
#define __NIXIECLOCK_H__

#define SDA1 2
#define SCL1 3
// LS is an input that is level translated from 5V logic
#define LS 4
#define BUZZER 17
// SHDN is an output through 1K resistor -  not implement on Nixie Clock
#define SHDN 27
// LE latch enable output to Nixie Shifters
#define LE 22
// SPI to Nixie Shifters
#define MOSI 10
#define MISO 9
#define SCK 11
// Infrared Sensor Input - This is connected to ID_SD which is incompatible with GPIO input
#define IR 0
// Temperature sensor plug in one wire sensor
#define TMP 5
// HV5222 is an input which is unused on 6 tube Nixie Clock
#define HV5222 6
// RX is UART input
#define RX 15 
// PWM1 and UP require swapping to use DMA/PWM for accurately modulating the Smart LED chain
// PWM1 output
#define PWM1 18
// DOWN is input button active low and require internal pull up resistors enabled
#define DOWN 23
// MODE is input button active low and require internal pull up resistors enabled
#define MODE 24
// UP is input button active low and require internal pull up resistors enabled (note swap with PWM1)
#define UP 16
// PWM2 and PWM3 are not used on the six tube Nixie Clock
#define PWM2 20
#define PWM3 21

#define LEDWIDTH	8
#define LEDHEIGHT	1
#define LEDCOUNT    (LEDWIDTH * LEDHEIGHT)

#define CONSIDERATE_SLEEP 950000000L
#define INTERPOLATE_STEP 25

typedef enum {COLON_OFF = 0, COLON_BLINK, COLON_ON} colonEnum_t;

typedef union {
    uint64_t ll;
    uint8_t b[8];
} llconv_t;

typedef struct {
    uint32_t color[LEDCOUNT];
    int32_t delay;
    bool isFast;
} ledroll_t;

typedef struct {
    int32_t count;
    int32_t pos;
    colonEnum_t colon;
    ledroll_t *roll;
} ledrollhead_t;

typedef struct {
    pthread_mutex_t mutex;
    bool kill;
} terminate_t;

extern terminate_t terminate;

static inline bool isTerminate(void)
{
    bool rv;
    pthread_mutex_lock(&terminate.mutex);
	rv = terminate.kill;
    pthread_mutex_unlock(&terminate.mutex);
    return rv;
}

static inline void notifyToTerminate(void)
{
    pthread_mutex_lock(&terminate.mutex);
	terminate.kill = true;
    pthread_mutex_unlock(&terminate.mutex);
}

void setColon(bool thisColon);
bool nextColon(bool thisColon);

void *timeTask(void *threadid);
void *ledTask(void *threadid);
ledrollhead_t *parseconfig(void);
#endif /* __NIXIECLOCK_H__ */