################################################################################
# GNU makefile
# File Name: makefile
################################################################################

CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt
INCLUDES += -I../libnvme/src

BINDIR := bin
OBJS = $(BINDIR)/aesdsocket.o \
       $(BINDIR)/commandQueue.o \
       $(BINDIR)/nvmeInterface.o \
       $(BINDIR)/perfInterface.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/aesdsocket.o: aesdsocket.c commandQueue.h | $(BINDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c aesdsocket.c -o $(BINDIR)/aesdsocket.o

$(BINDIR)/commandQueue.o: commandQueue.c commandQueue.h | $(BINDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c commandQueue.c -o $(BINDIR)/commandQueue.o

$(BINDIR)/nvmeInterface.o: nvmeInterface.c | $(BINDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c nvmeInterface.c -o $(BINDIR)/nvmeInterface.o

$(BINDIR)/perfInterface.o: perfInterface.c | $(BINDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c perfInterface.c -o $(BINDIR)/perfInterface.o

clean:
	rm -rf $(BINDIR) *.out $(TARGET)