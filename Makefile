#
# Makefile REALTIME Kernel
#

PATH := /u/wbcowan/gnuarm-4.0.2/arm-elf/bin:/u/wbcowan/gnuarm-4.0.2/libexec/gcc/arm-elf/4.0.2:$(PATH)

XCC     = /u/wbcowan/gnuarm-4.0.2/arm-elf/bin/gcc
AS	= /u/wbcowan/gnuarm-4.0.2/arm-elf/bin/as
AR	= /u/wbcowan/gnuarm-4.0.2/arm-elf/bin/ar
ARFLAGS = rcs


INCLUDE = -I include -I include/kern -I include/user
BIN = bin
KER_DIR = kernel
UTIL_DIR = util
TASK_DIR = user

USER = $(shell whoami)
FILE = train2

DEBUGFLAGS = -D_ASSERT -D_DEBUG
OPTIMIZATION = 2
CFLAGS  = -c -fPIC -Wall -I. $(INCLUDE) -mcpu=arm920t -msoft-float -O$(OPTIMIZATION) $(DEBUGFLAGS)

# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -msoft-float: use software for floating point

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs-32: always create a complete stack frame

LDFLAGS = -init main -Map $(KER_DIR)/$(FILE).map -N -T $(BIN)/orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L../lib


# UTIL
UTIL_C_SRC = $(wildcard $(UTIL_DIR)/*.c)
UTIL_ASM_SRC = $(wildcard $(UTIL_DIR)/*.asm)
UTIL_C_BUILD = $(UTIL_C_SRC:.c=.o)
UTIL_ASM_BUILD = $(UTIL_ASM_SRC:.asm=.o)

# TASK
TASK_C_SRC = $(wildcard $(TASK_DIR)/*.c)
TASK_ASM_SRC = $(wildcard $(TASK_DIR)/*.asm)
TASK_C_BUILD = $(TASK_C_SRC:.c=.o)
TASK_ASM_BUILD = $(TASK_ASM_SRC:.asm=.o)

# ker
KER_C_SRC = $(wildcard $(KER_DIR)/*.c)
KER_ASM_SRC = $(wildcard $(KER_DIR)/*.asm)
KER_C_BUILD = $(KER_C_SRC:.c=.o)
KER_ASM_BUILD = $(KER_ASM_SRC:.asm=.o)

CBUILD = $(UTIL_C_BUILD) $(TASK_C_BUILD) $(KER_C_BUILD)
ASMBUILD = $(UTIL_ASM_BUILD) $(TASK_ASM_BUILD) $(KER_ASM_BUILD)
ASMFILES = $(CBUILD:.o=.s) $(ASMBUILD:.o=.asm)

all: $(ASMFILES) $(KER_DIR)/$(FILE).elf copy_ftp

.PHONY:
	copy_ftp
	clean

$(KER_DIR)/$(FILE).elf: $(CBUILD) $(ASMBUILD)
	$(LD) $(LDFLAGS) -o $@ $(CBUILD) $(ASMBUILD) -lgcc

%.s:%.c
	$(XCC) -S $(CFLAGS) -o $@ $<

%.o:%.asm
	$(AS) $(ASFLAGS) -o $@ $<

%.o:%.s
	$(AS) $(ASFLAGS) -o $@ $<

copy_ftp:
	cp $(KER_DIR)/$(FILE).elf /u/cs452/tftp/ARM/$(USER)/$(FILE).elf
	chmod 777 /u/cs452/tftp/ARM/$(USER)/$(FILE).elf

clean:
	rm -f $(KER_DIR)/*.s
	rm -f $(KER_DIR)/*.o
	rm -f $(KER_DIR)/*.elf
	rm -f $(KER_DIR)/*.map
	rm -f $(TASK_DIR)/*.s
	rm -f $(TASK_DIR)/*.o
	rm -f $(UTIL_DIR)/*.s
	rm -f $(UTIL_DIR)/*.o
