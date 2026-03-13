ARCH       ?= aarch64-none-elf-
CC         := $(ARCH)gcc
CXX        := $(ARCH)g++
LD         := $(ARCH)ld
AR         := $(ARCH)ar

EXEC_NAME ?= uno.a

STDINC ?= ../redlib
CFLAGS ?= -std=c99 -I$(STDINC) -I. -O0
C_SOURCE ?= $(shell find . -name '*.c')
OBJ ?= $(C_SOURCE:%.c=%.o)

.PHONY: dump

ifeq ($(ARCH), aarch64-none-elf-)
	CFLAGS += -nostdlib -ffreestanding
else
	CFLAGS += -I$(INCLUDES)
endif

%.o : %.c
	$(CC) $(CFLAGS) $(SH_FLAGS) -c -c $< -o $@

$(EXEC_NAME): $(OBJ)
	$(AR) rcs $@ $(OBJ)

all: $(EXEC_NAME)

clean:
	rm $(OBJ)
	rm $(EXEC_NAME)

cross:
	make all ARCH= SH_FLAGS=-DCROSS

dump: all
	$(ARCH)objdump -S $(EXEC_NAME) > dump
