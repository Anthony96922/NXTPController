
NAME = nxtpctl
DEBUG ?= 0
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic
OFLAGS =

ifeq ($(DEBUG), 1)
	CFLAGS += -g -DDEBUG
else
	CFLAGS += -O2
	OFLAGS += -s
endif

objs = nxtpctl.o packet.o serial.o text.o

$(NAME): $(objs)
	$(CC) $(objs) $(OFLAGS) -o $(NAME) -pthread
clean:
	rm -f *.o
