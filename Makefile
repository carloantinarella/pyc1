TARGET = pyc1

CC = gcc
CFLAGS += -lpthread
CFLAGS += -lrt

C_SRC = ./main.c

.PHONY: all
all: $(TARGET)

%: %.c
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm $(TARGET)
