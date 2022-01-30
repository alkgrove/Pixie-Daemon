/*
 * @file parseconfig.c
 * @brief tries to find and parses the LED color file (json)
 * @details The config file uses non-strict json and has #RRGGBB or 0xRRGGBB formats
 * It expects this format. It always must have color record. It will use the last step and delay if ommitted
 * { LED : [ { <- Preamble always an array
 *  "step" : "fast" <- step is whether to use fast or slow, fast immediately changes color, slow transitions to the next color
 *  "delay" : <integer> <- number of milliseconds before the next step. Note transition is 10ms per step so this number is divided by 10 for
 *                         the number of intermediate steps
 *  "color" :[ <hex>, ... <hex> ] }, <- eight hex numbers in an array either #RRGGBB (javascript) or 0xRRGGBB (hex C format)
 *  {"step" : "fast", ... }, <- next record
 *  {"step" : "fast", ... } <- last record
 *  ]
 * }
 * 
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
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "jsmn.h"
#include "nixieclock.h"

#define INITIAL_TOKEN_COUNT 128
#define TOKEN_COUNT_INCREMENT 128
static bool jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  return (tok->type == JSMN_STRING && strncmp(json + tok->start, s, tok->end - tok->start) == 0);
}

static uint32_t levelAdjust(uint32_t value, uint32_t level) 
{
    uint32_t r, g, b;
    r = (value >> 16) & 0xFF;
    g = (value >> 8) & 0xFF;
    b = value & 0xFF;
    r = (r * level) / 100;
    g = (g * level) / 100;
    b = (b * level) / 100;
    return ((r << 16) | (g << 8) | b);
}
    
const char *tokentypestring(jsmntype_t type) {

    if (type == JSMN_UNDEFINED) return "UNDEFINED";
    if (type == JSMN_OBJECT) return "OBJECT";
    if (type == JSMN_ARRAY) return "ARRAY";
    if (type == JSMN_STRING) return "STRING";
    if (type == JSMN_PRIMITIVE) return "PRIMITIVE";
    return "DONT KNOW";
}
    
const char *pathfilename[] = {"/etc/LEDcolor.json", "/usr/local/etc/LEDcolor.json", "./LEDcolor.json"};

ledrollhead_t *parseconfig(void)
{
    FILE *fin = NULL;
    long int filesize;
    char *filebuffer;
    int fileindex = -1;
    jsmn_parser jsonparser;
    jsmntok_t *tokenp;
    int tidx;
    int tokencount = INITIAL_TOKEN_COUNT;
    bool done = false;
    int recordcount;
    int itemcount;
    int level = 100;
    ledroll_t *ledroll;
    bool fast;
    int32_t delay;
    char *endp;
    char *p;
    int rv;
    ledrollhead_t *ledrollhead;
    
    for (int i = 0; i < sizeof(pathfilename)/sizeof(char *); i++) {
        fin = fopen(pathfilename[i], "r");
        if (fin != NULL) {
            if (fseek(fin, 0, SEEK_END) < 0) {
                fprintf(stderr, "failed to seek the open file %s\n", pathfilename[i]);
                exit(EXIT_FAILURE);
            }
            filesize = ftell(fin);
            if (filesize < 0) {
                fprintf(stderr, "bad file size returned for file: %s\n", pathfilename[i]);
                exit(EXIT_FAILURE);
            }
            if (fseek(fin, 0L, SEEK_SET) < 0) {
                fprintf(stderr, "unable to reset the open file %s\n", pathfilename[i]);
                exit(EXIT_FAILURE);
            }
            fileindex = i;
            break;
        }
    }
    if (fin == NULL) {
        fprintf(stderr, "unable to find a valid configuration file\n");
        exit(EXIT_FAILURE);
    }
    filebuffer = (char *) malloc((filesize * sizeof(char)) + 1);
    if (filebuffer < 0) {
        fprintf(stderr, "unable to allocate memory for file %s\n", pathfilename[fileindex]);
        exit(EXIT_FAILURE);
    }    
    if (fread(filebuffer, filesize, sizeof(char), fin) < 0) {
        fprintf(stderr, "unable to read config file %s\n", pathfilename[fileindex]);
        exit(EXIT_FAILURE);
    }
    filebuffer[filesize] = '\0';
    fclose(fin);
    jsmn_init(&jsonparser);
    tokenp = (jsmntok_t *) malloc(INITIAL_TOKEN_COUNT * sizeof(jsmntok_t));
    tokencount = INITIAL_TOKEN_COUNT;
    if (tokenp == NULL) {
        fprintf(stderr, "out of memory - config file to large\n");
        exit(EXIT_FAILURE);
    }
    while(!done) {
        rv = jsmn_parse(&jsonparser, filebuffer, filesize, tokenp, tokencount);
        if (rv == JSMN_ERROR_NOMEM) {
            tokencount += TOKEN_COUNT_INCREMENT;
            tokenp = realloc(tokenp, tokencount * sizeof(jsmntok_t));
            if (tokenp == NULL) {
                fprintf(stderr, "out of memory - config file to large\n");
                exit(EXIT_FAILURE);
            }
        } else if (rv == JSMN_ERROR_INVAL) {
            fprintf(stderr, "Invalid character in json\n");
            exit(EXIT_FAILURE);
        } else if (rv == JSMN_ERROR_PART) {
            fprintf(stderr, "Incomplete or invalid json structure %d\n", rv);
            exit(EXIT_FAILURE);
        } else {
            done = true;
        }
    }
    if (tokenp[0].type != JSMN_OBJECT) {
        fprintf(stderr, "Invalid json - must start as an object\n");
        exit(EXIT_FAILURE);
    }
    tidx = 1;
    for (int topobj = 0; topobj < tokenp[0].size; topobj++) {
//        fprintf(stdout, "encountered at %d '%.*s' type %s\n", tidx, tokenp[tidx].end - tokenp[tidx].start, &filebuffer[tokenp[tidx].start], tokentypestring(tokenp[tidx].type));
        if (jsoneq(filebuffer, &tokenp[tidx], "system")) {
            tidx++;
            if (tokenp[tidx].type != JSMN_OBJECT || tokenp[tidx].size == 0) {
                fprintf(stderr, "Expected object for system key\n");
                exit(EXIT_FAILURE);
            }
            recordcount = tokenp[tidx++].size;
            for (int i = 0; i < recordcount; i++) {
                if (jsoneq(filebuffer, &tokenp[tidx], "level") && tokenp[tidx].size == 1) {
                    tidx++;
                    level = strtol(&filebuffer[tokenp[tidx].start], &endp, 10);
                    if (tokenp[tidx].type != JSMN_PRIMITIVE || &filebuffer[tokenp[tidx].start] == endp || level < 0 || level > 100) {
                        fprintf(stderr, "invalid system level value\n");
                        exit(EXIT_FAILURE);
                    }
                    tidx++;
                } else {
                    fprintf(stderr, "invalid key for system\n");
                    exit(EXIT_FAILURE);
                }
            }      
        } else if (jsoneq(filebuffer,&tokenp[tidx], "roll")) {
            tidx++; //past LED key (optional)
            if (tokenp[tidx].type != JSMN_ARRAY || tokenp[tidx].size == 0) {
                fprintf(stderr, "Expected array for all color records\n");
                exit(EXIT_FAILURE);
            }
            recordcount = tokenp[tidx++].size;
            ledroll = (ledroll_t *) malloc(recordcount * sizeof(ledroll_t));
            if (ledroll == NULL) {
                fprintf(stderr, "Out of memory building config\n");
                exit(EXIT_FAILURE);
            }
            ledrollhead = (ledrollhead_t *) malloc(sizeof(ledrollhead_t));
            if (ledrollhead == NULL) {
                fprintf(stderr, "Out of memory building config\n");
                exit(EXIT_FAILURE);
            }
            ledrollhead->roll = ledroll;
            ledrollhead->pos = 0;
            ledrollhead->count = recordcount;
            fast = true;
            delay = 1000; // default is 1 second (1000ms)
            for (int i = 0; i < recordcount; i++) {
                if (tokenp[tidx].type != JSMN_OBJECT || tokenp[tidx].size == 0) {
                    fprintf(stderr, "Record #%d must be an object and have {}'s\n", i+1);
                    exit(EXIT_FAILURE);
                }
                itemcount = tokenp[tidx++].size;
                for (int j = 0; j < itemcount; j++) {
                    if (jsoneq(filebuffer, &tokenp[tidx], "step") && tokenp[tidx].size == 1) {
                        tidx++;
                        if(jsoneq(filebuffer, &tokenp[tidx], "slow")) {
                            fast = false;
                        } else if (jsoneq(filebuffer, &tokenp[tidx], "fast")) {
                            fast = true;
                        } else {
                            fprintf(stderr, "step has invalid value, should be fast or slow\n");
                            exit(EXIT_FAILURE);
                        }
                        tidx++;
                    } else if (jsoneq(filebuffer, &tokenp[tidx], "delay") && tokenp[tidx].size == 1) {
                        tidx++;
                        delay = strtol(&filebuffer[tokenp[tidx].start], &endp, 10);
                        if (tokenp[tidx].type != JSMN_PRIMITIVE || &filebuffer[tokenp[tidx].start] == endp) {
                            fprintf(stderr, "invalid delay value for record #%d\n", i);
                            exit(EXIT_FAILURE);
                        }
                        if (delay < INTERPOLATE_STEP) {
                            fprintf(stderr, "Record #%d delay is too small, should be >= %dms\n", i+1, INTERPOLATE_STEP);
                            exit(EXIT_FAILURE);
                        }
                         
                        tidx++;
                    } else if (jsoneq(filebuffer, &tokenp[tidx], "color")) {
                        tidx++;
                        if (tokenp[tidx].type != JSMN_ARRAY) {
                            fprintf(stderr, "expected an array for color record #%d\n", i+1);
                            exit(EXIT_FAILURE);
                        }
                        if (tokenp[tidx].size != LEDCOUNT) {
                            fprintf(stderr, "array size for LED is wrong for record #%d\n", i+1);
                            exit(EXIT_FAILURE);
                        }
            
                        tidx++;
                        for(int k = 0; k < LEDCOUNT; k++) {
                            if (tokenp[tidx].type != JSMN_STRING) {
                                fprintf(stderr, "record #%d color array #%d needs to be hexadecimal string\n", i+1, k+1);
                                exit(EXIT_FAILURE);
                            }
                            p = &filebuffer[tokenp[tidx].start];
                            ledroll[i].color[k] = levelAdjust(strtol((*p == '#') ? p+1 : p, &endp, 16), level);
                            tidx++;
                        }
                    }
                    ledroll[i].isFast = fast;
                    ledroll[i].delay = delay;
                }
            }
        } else {
            fprintf(stderr, "expected roll key or system key\n");
            exit(EXIT_FAILURE);
        }
    }
    free(filebuffer);
    free(tokenp);
    return ledrollhead;
}
                
    
    
        
        
            
