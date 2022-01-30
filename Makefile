# makefile for testing

TARGET = pixie
#DEBUG=1
REV="1.00"
APP="Pixie Daemon"

CSRC = main.c
CSRC += spipi.c
CSRC += gpiopi.c
CSRC += parseconfig.c
CSRC += ledTask.c
CSRC += timeTask.c

OBJDIR=bin/
INCDIR=inc/
INC = ./inc
INC += /usr/local/include/ws2811
VPATH = ./src

INCLUDES=${patsubst %,-I%,${INC}}
CFLAGS = -c 
CFLAGS += -D'REV=${REV}' -D'APP=${APP}'

OBJ = $(notdir $(CSRC:.c=.o))

ifdef DEBUG
DEFS += -DDEBUG
endif

CC=gcc
RM=rm
CP=cp

LIB = -pthread -lm /usr/local/lib/libws2811.a

all: ${OBJDIR} ${OBJDIR}${TARGET}

${OBJDIR}:
	@test -d ${OBJDIR} || mkdir ${OBJDIR}
	@test -d ~/jsmn || echo "Can't find jsmn directory needed for this project, see readme."
	@cp -f ~/jsmn/jsmn.h ${INCDIR}

${OBJDIR}%.o : %.c
	${CC} ${CFLAGS} ${DEFS} ${INCLUDES} $< -o ${@}

${OBJDIR}${TARGET}: $(addprefix ${OBJDIR},${OBJ})
	${CC}  $(filter %.o %.a, ${^})  ${LIB} -o ${@}

install:
	${CP} -f bin/pixie /usr/local/bin/
	${CP} -f assets/pixied.service /etc/systemd/system
	@test -f /usr/local/etc/LEDcolor.json || ${CP} -f assets/default.json /usr/local/etc/LEDcolor.json

clean:
	${RM} -rf ${OBJDIR} ${wildcard *~}

print-%:
	@echo $* = $($*)

help:
	${CC} --version
	@make --version
	@make --print-data-base --question | awk '/^[^.%][-A-Za-z0-9_]*:/ { print substr($$1,1,length($$1)-1) }' 

# include dependencies
ifneq "${MAKECMDGOALS}" "clean"
-include ${wildcard ${OBJDIR}*.d}
endif
