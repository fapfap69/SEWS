CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lpthread

SRCS = src/main.c src/http_server.c src/websocket.c src/utils.c
OBJS = $(SRCS:.c=.o)
TARGET = sews

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
$(CC) $(CFLAGS) -c $< -o $@

clean:
rm -f $(OBJS) $(TARGET)
