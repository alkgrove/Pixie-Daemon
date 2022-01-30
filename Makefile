# makefile for testing

TARGET = pixied
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
INC += ../jsmn
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
CHMOD=chmod

LIB = -pthread -lm /usr/local/lib/libws2811.a

all: ${OBJDIR} ${OBJDIR}${TARGET}

${OBJDIR}:
	@test -d ${OBJDIR} || mkdir ${OBJDIR}
	@test -d ../jsmn || echo "Can't find jsmn directory needed for this project, see readme."

${OBJDIR}%.o : %.c
	${CC} ${CFLAGS} ${DEFS} ${INCLUDES} $< -o ${@}

${OBJDIR}${TARGET}: $(addprefix ${OBJDIR},${OBJ})
	${CC}  $(filter %.o %.a, ${^})  ${LIB} -o ${@}

install:
	${CP} -f ${OBJDIR}${TARGET} /usr/local/bin/
	${CP} -f assets/pixied.service /etc/systemd/system
	${CHMOD} 664 /etc/systemd/system/pixied.service
	@test -f /usr/local/etc/LEDcolor.json || ${CP} -f assets/default.json /usr/local/etc/LEDcolor.json

uninstall:
	service pixied stop
	systemctl disable pixied
	${RM} -f /usr/local/bin/${TARGET}
	${RM} -f /etc/systemd/system/pixied.service
	${RM} -f /usr/local/etc/LEDcolor.json

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
