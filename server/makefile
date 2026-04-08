################################################################################
# GNU makefile
################################################################################

CC ?= $(CROSS_COMPILE)gcc
PKG_CONFIG ?= pkg-config

CFLAGS ?= -g -Wall -Werror
TARGET ?= aesdsocket

# Where you installed libnvme
LIBNVME_PREFIX ?= ../libnvme/install

# Let pkg-config find libnvme.pc
LIBNVME_PKGCONFIG := $(LIBNVME_PREFIX)/lib/pkgconfig:$(LIBNVME_PREFIX)/lib64/pkgconfig

NVME_CFLAGS := $(shell PKG_CONFIG_PATH=$(LIBNVME_PKGCONFIG):$(PKG_CONFIG_PATH) $(PKG_CONFIG) --cflags libnvme)
NVME_LIBS   := $(shell PKG_CONFIG_PATH=$(LIBNVME_PKGCONFIG):$(PKG_CONFIG_PATH) $(PKG_CONFIG) --libs libnvme)

CFLAGS += $(NVME_CFLAGS)
LDLIBS += $(NVME_LIBS) -lpthread -lrt

BINDIR := bin
OBJS := $(BINDIR)/aesdsocket.o \
        $(BINDIR)/commandQueue.o \
        $(BINDIR)/nvmeInterface.o \
        $(BINDIR)/perfInterface.o

.PHONY: all clean print-vars

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/%.o: %.c | $(BINDIR)
	$(CC) $(CFLAGS) -c $< -o $@

print-vars:
	@echo "CC=$(CC)"
	@echo "NVME_CFLAGS=$(NVME_CFLAGS)"
	@echo "NVME_LIBS=$(NVME_LIBS)"

clean:
	rm -rf $(BINDIR) *.out $(TARGET)