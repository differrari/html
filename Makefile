ARCH       ?= aarch64-none-elf-
CC         := $(ARCH)gcc
CXX        := $(ARCH)g++
LD         := $(ARCH)ld
DUMP	   := $(ARCH)objdump

STDINC ?= ../os/shared/
STDLIB ?= ../os/shared/libshared.a
CFLAGS ?= -std=c99 -g -I$(STDINC) -O0
EXEC_NAME ?= $(notdir $(CURDIR))
PKG ?= $(EXEC_NAME).red
OUT ?= $(PKG)/$(EXEC_NAME).elf
FS_PATH ?= ../os/fs/redos/user/
OBJ := $(shell find . -name '*.o')

ifeq ($(ARCH), aarch64-none-elf-)
	CFLAGS += -nostdlib -ffreestanding
	# LDFLAGS  := -X-T$(shell ls *.ld)
	LDFLAGS := -Wl,-Ttext=0x1000,-emain
else
	CFLAGS += -I$(INCLUDES)
endif

.PHONY: dump

all: prepare compile
	echo "Finished build"

prepare:
	mkdir -p $(PKG)
	mkdir -p resources
	cp -r resources $(PKG)
	#cp package.info $(EXEC_NAME).red

compile: $(OBJ)
	echo "ARCH = $(ARCH)"
	$(CC) $(LDFLAGS) $(CFLAGS) main.c -I$(STDINC) $(CROSSLIB) ../os/shared/libshared.a -o $(OUT)
	chmod +x $(OUT)

run: all
ifeq ($(ARCH), aarch64-none-elf-)
	cp -r $(PKG) $(FS_PATH)
	make -C ../os run
else
	$(OUT)
endif

clean:
	rm $(OUT)
	rm -r $(EXEC_NAME).red

dump:
	$(DUMP) -D $(OUT) > dump
