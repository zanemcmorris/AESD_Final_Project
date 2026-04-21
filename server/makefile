################################################################################
# GNU makefile
################################################################################

CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
PKG_CONFIG ?= pkg-config
AR := $(CROSS_COMPILE)ar
LD := $(CROSS_COMPILE)ld
STRIP := $(CROSS_COMPILE)strip

CFLAGS ?= -g -Wall -Werror
TARGET ?= aesdsocket

# Where you installed libnvme
LIBNVME_PREFIX ?= ../libnvme/install

PARTED_LOCAL_INC ?= ../parted/include
PARTED_LIBDIR ?= ../parted/libparted/.libs

# Directory containing libparted.pc
PARTED_PC_DIR ?= ../parted

# Perf submodule
PERF_DIR ?= ../final-project-LJ0704/perf_cmd
PERF_INC ?= $(PERF_DIR)

# Let pkg-config find libnvme.pc
LIBNVME_PKGCONFIG := $(LIBNVME_PREFIX)/lib/pkgconfig:$(LIBNVME_PREFIX)/lib64/pkgconfig

NVME_CFLAGS := $(shell PKG_CONFIG_PATH=$(LIBNVME_PKGCONFIG):$(PKG_CONFIG_PATH) $(PKG_CONFIG) --cflags libnvme)
NVME_LIBS   := $(shell PKG_CONFIG_PATH=$(LIBNVME_PKGCONFIG):$(PKG_CONFIG_PATH) $(PKG_CONFIG) --libs libnvme)

# Let pkg-config find libparted.pc
PARTED_PKGCONFIG := $(PARTED_PC_DIR):$(PKG_CONFIG_PATH)

PARTED_CFLAGS := $(shell PKG_CONFIG_PATH=$(PARTED_PKGCONFIG) $(PKG_CONFIG) --cflags libparted)
PARTED_LIBS   := $(shell PKG_CONFIG_PATH=$(PARTED_PKGCONFIG) $(PKG_CONFIG) --libs libparted)

CFLAGS += $(NVME_CFLAGS) $(PARTED_CFLAGS) -I$(PARTED_LOCAL_INC) -I$(PERF_INC)
LDLIBS += $(NVME_LIBS) $(PARTED_LIBDIR)/libparted.a -lpthread -lrt -ldevmapper -luuid

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

$(BINDIR)/perfInterface.o: $(PERF_DIR)/perfInterface.c | $(BINDIR)
	$(CC) $(CFLAGS) -c $< -o $@

print-vars:
	@echo "CC=$(CC)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "LDLIBS=$(LDLIBS)"
	@echo "NVME_CFLAGS=$(NVME_CFLAGS)"
	@echo "NVME_LIBS=$(NVME_LIBS)"
	@echo "PARTED_CFLAGS=$(PARTED_CFLAGS)"
	@echo "PARTED_LIBS=$(PARTED_LIBS)"
	@echo "PARTED_LOCAL_INC=$(PARTED_LOCAL_INC)"
	@echo "PARTED_PC_DIR=$(PARTED_PC_DIR)"
	@echo "PERF_DIR=$(PERF_DIR)"
	@echo "PERF_INC=$(PERF_INC)"

clean:
	rm -rf $(BINDIR) *.out $(TARGET)