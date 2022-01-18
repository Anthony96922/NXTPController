
NAME = nxtpctl

DEBUG = 1

CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -pedantic

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

objs = $(NAME).o packet.o serial.o text.o

$(NAME): $(objs)
	$(CC) $(objs) -o $(NAME) -s
clean:
	rm -f *.o
