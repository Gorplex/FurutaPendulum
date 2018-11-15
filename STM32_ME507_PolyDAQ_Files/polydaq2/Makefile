#======================================================================================
# File:  Makefile
#     This is the Makefile for a FreeRTOS project done in Cal Poly ME405 style. The
#     project directories (lab1, lab2, etc.) are located one level under the project
#     root, and the library tree forms another set of branches from the project root.
#     This allows multiple projects to be set up using the same set of libraries.
#
# Revisions:
#     ??-??-???? M.K Original file from https://github.com/k-code/stm32f4-examples
#     06-13-2014 JRR Rewritten for local ME405 use by merging with earlier ME405 files
#     08-26-2014 JRR Application code directory moved from top to one level down
#     11-08-2018 LAT STM32F10x support added
#======================================================================================

#================================= USER'S SETTINGS ====================================
# The name of the project. The compiled files are given this name
PROJECT_NAME = polydaq2

#--------------------------------------------------------------------------------------
# The source files written by the user should be listed here. Library source files are
# not listed here; they're in sections below this one
SOURCES      = main.cpp task_leds.cpp task_user.cpp task_data_acq.cpp task_sd_daq.cpp \
               task_sd_card.cpp config_file.cpp logger_config.cpp polydaq2.cpp

# The board for which we're compiling is specified here from the following list
# BOARD_F4DISCO     - STM32F4 Discovery board, PolyDAQ 2, or STMpunk (Cal Poly ME405)
# BOARD_F401_NUCLEO - Nucleo board with STM32F401RE
# BOARD_F411_NUCLEO - Nucleo board with STM32F411RE
# BOARD_F103
BOARD        = BOARD_F103
# BOARD        = BOARD_F4DISCO

# How hard the optimizer should work: 0 means don't try to optimize the code to run
# faster; 1 through 3 try progressively harder; s means optimize for code size
OPTIM        = 2

# List of the directories in which header (*.h) files are found. This list does not
# include system headers included <like_this.h> nor library headers; it only holds
# directories with headers for user-written code. Usually this only needs to have the
# current project directory, given as "-I."
INCLUDES     = -I.

#================ USUALLY THE USER NEEDN'T CHANGE STUFF BELOW THIS LINE ===============
#
# Defines which are specific to the board being used. These include processor flags 
# and the choice of linker script
# -DSTM32F401xx    - If using an STM32F401RE, probably on a Nucleo board
# -DSTM32F40_41xxx - If using a PolyDAQ, STMpunk, or STM32F4 Discovery board
ifeq ($(BOARD), BOARD_F401_NUCLEO)
    BOARDFLAGS = -DSTM32F401xx
    LDSCRIPT  = $(DOTDOT)/$(LIBROOT)/STM32F4xx/STM32F401RE_flash.ld
    LIB_DIRS  = STM32F4xx STM32F4xx_StdPeriph_Driver
else ifeq ($(BOARD), BOARD_F411_NUCLEO)
    BOARDFLAGS = -DSTM32F411xx
    LDSCRIPT  = $(DOTDOT)/$(LIBROOT)/STM32F4xx/STM32F411RE_flash.ld
    LIB_DIRS  = STM32F4xx STM32F4xx_StdPeriph_Driver
else ifeq ($(BOARD), BOARD_F4DISCO)
    BOARDFLAGS = -DSTM32F40_41xxx
    LDSCRIPT  = $(DOTDOT)/$(LIBROOT)/STM32F4xx/stm32_flash.ld
    LIB_DIRS  = STM32F4xx STM32F4xx_StdPeriph_Driver

else ifeq ($(BOARD), BOARD_F103)
    BOARDFLAGS = -DSTM32F103xB -DSTM32F10X_MD -DSTM_GENERIC_HEADER=stm32f10
    LDSCRIPT  = $(DOTDOT)/$(LIBROOT)/STM32F1xx/STM32F103C8Tx_FLASH.ld
    LIB_DIRS  = STM32F1xx STM32F1xx_StdPeriph_Driver
endif

#================================= LIBRARY STUFF ======================================

# We need a name for the root directory under which all our project files are found
DOTDOT = ..

# Location of the root of the library part of the directory tree
LIBROOT = lib

# Name of the directory in which compiled files will be placed
BUILDDIR = build

# Specify a name and location for the library file
LIB_FILE = $(BUILDDIR)/lib_me405.a

# All the C and C++ files from each of these directories will be compiled and added
# into the library file. Headers will be found if they're on this list. Each 
# directory is specified by its relative path under the library root
LIB_DIRS += CMSIS \
            ME405/misc ME405/serial ME405/rtcpp \
            ME405/drivers ME405/drivers/sd_card ME405/drivers/i2c \
            freertos freertos/inc freertos/src freertos/ports/GCC/ARM_CM4F

# Create a list of relative path names by which the library directories can be found
LIB_FULL = $(addprefix $(DOTDOT)/$(LIBROOT)/, $(LIB_DIRS))

# Make a list of include directories, putting -I in front of each for the compiler
LIB_INC  = $(addprefix "-I", $(LIB_FULL)) -I.

# Make a list of source files from the source files in subdirectories in LIB_DIRS
LIB_SRC  = $(foreach A_DIR, $(LIB_FULL), \
             $(wildcard $(A_DIR)/*.cpp $(A_DIR)/*.c $(A_DIR)/*.S))

# The bare file names are needed by the compiler to find files in the virtual path
LIB_BARE = $(notdir $(LIB_SRC))

# Make a list of the object files which need to be compiled from the source files. The
# compiled files will go into a mirror image of the library tree which is under the
# build directory
LIB_TEMP = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(LIB_BARE))))
LIB_OBJS = $(subst $(DOTDOT), $(BUILDDIR), $(LIB_TEMP))

# Specify virtual paths in which the source files can be found
vpath %.cpp $(LIB_FULL)
vpath %.c $(LIB_FULL)
vpath %.S $(LIB_FULL)

#================================== PROGRAMS USED  ====================================

# Make a list of the object files which need to be compiled from the source files
OBJECTS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SOURCES))))

#--------------------------------------------------------------------------------------
# Give some short names to the ELF, HEX, and BIN format output files
ELF = $(BUILDDIR)/$(PROJECT_NAME).elf
HEX = $(BUILDDIR)/$(PROJECT_NAME).hex
BIN = $(BUILDDIR)/$(PROJECT_NAME).bin

#--------------------------------------------------------------------------------------
# List the various programs which are used to compile, link, archive, etc. 
ARCHIE  = arm-none-eabi
CC      = $(ARCHIE)-gcc
CXX     = $(ARCHIE)-g++
LD      = $(ARCHIE)-g++
AR      = $(ARCHIE)-ar
OBJCOPY = $(ARCHIE)-objcopy
GDB     = $(ARCHIE)-gdb
SIZER   = $(ARCHIE)-size

#================================== OPTION FLAGS  =====================================

# Flags used by the compiler and linker
BASEFLAGS = -mcpu=cortex-m3 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=soft \
            -O$(OPTIM) -g -fno-exceptions \
            -mthumb-interwork -ffreestanding -nostdlib \
            -MMD -MP -MF $(BUILDDIR)/$(*F).d \
            $(BOARDFLAGS) \
             --specs=rdimon.specs
# BASEFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
#             -O$(OPTIM) -g -fno-exceptions \
#             -mthumb-interwork -ffreestanding -nostdlib \
#             -MMD -MP -MF $(BUILDDIR)/$(*F).d \
#             $(BOARDFLAGS) \
#              --specs=rdimon.specs

# Flags which are common to the compilers but not the linker
COMPFLAGS = $(BASEFLAGS) $(INCLUDES) $(LIB_INC) \
            -Wall \
            -DUSE_STDPERIPH_DRIVER

# Flags specific to the C compiler
CFLAGS    = $(COMPFLAGS)

# Flags specific to the C++ compiler
CXXFLAGS  = $(COMPFLAGS) -std=c++11 -fno-exceptions -pedantic -fno-rtti \
            -Wextra -Winline -Wpointer-arith -Wredundant-decls -Wshadow \
            -Wcast-qual -Wcast-align

# Flags specific to the linker
LDFLAGS   = $(BASEFLAGS) -T$(LDSCRIPT) 

#=================================== THE RULES ========================================

# Rules to make downloadable binary files in ELF (which contains debugging 
# information), BIN (pure binary machine code), and HEX (text binary) formats
$(BIN): $(ELF)
	@$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	@$(OBJCOPY) -O ihex $< $@

$(ELF): $(LIB_FILE) $(OBJECTS)
	@echo "Linking:     " $(OBJECTS) $(LIB_FILE) " --> " $@
	@$(LD) $(LDFLAGS) $(OBJECTS) $(LIB_FILE) -o $@
	@$(SIZER) $@

# Auto-generate dependency info for existing .o files
-include $(OBJECTS:.o=.d)

#--------------------------------------------------------------------------------------
# Rules to compile and assemble source code into object code. An image of each source
# directory is created under the build directory and object files are put there
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling:   " $< " --> " $@
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling:   " $< " --> " $@
	@$(CXX) -c $(CXXFLAGS) $< -o $@

$(BUILDDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@echo "Assembling:  " $< " --> " $@
	@$(CC) -c $(CFLAGS) $< -o $@

#--------------------------------------------------------------------------------------
# This rule will build the library file from all the .o files in the library folders
$(LIB_FILE): $(LIB_OBJS)
	@echo "Library-ing:  (*.o) --> " $@
	@$(AR) -c -r $@ $(LIB_OBJS)

#==================================== TARGETS =========================================

# This, the first target, causes the program to be compiled and linked but not
# downloaded to the microcontroller. It's invoked when one types just 'make'
all: $(HEX) $(BIN)

#--------------------------------------------------------------------------------------
# Rule to download executable code to the microcontroller
flash: $(BIN)
	qstlink2 --cli --write --quiet $(BIN)
#	st-flash write $(BIN) 0x8000000

#--------------------------------------------------------------------------------------
# A rule to run the debugger programs. We use OpenOCD to set up a debugging interface
# through the ST-Link 2, then gdb in text user interface mode to control things
debug: $(ELF)
	@sh -c 'openocd -f ../lib/STM32F4xx/stm32f4discovery.cfg &'
	$(GDB) -tui --eval-command="target remote localhost:3333" $(ELF) 
	@killall openocd

#--------------------------------------------------------------------------------------
# 
.PHONY: clean
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILDDIR)

#--------------------------------------------------------------------------------------
# This target runs Doxygen to make documentation for the project (but not the library).
.PHONY: doc
doc:
	@doxygen doxygen.conf

#--------------------------------------------------------------------------------------
# This target produces a (large) list of all the predefined macros which are
# available with the compiler version that's being used for this program. Such
# macros can be handy for automatically compiling different code for different
# processors, checking the largest and smallest numbers for different data
# types, and so on
.PHONY: defines
defines:
	$(CC) -dM -E - < /dev/null

#--------------------------------------------------------------------------------------
# This target is used when debugging the Makefile. It shows the variables that
# hold lists of files to be compiled
.PHONY: dirs
dirs:
	@echo LIB_DIRS: $(LIB_DIRS)
	@echo
	@echo LIB_FULL: $(LIB_FULL)
	@echo
	@echo LIB_INC: $(LIB_INC)
	@echo
	@echo LIB_BARE: $(LIB_BARE)
	@echo
	@echo LIB_SRC: $(LIB_SRC)
	@echo
	@echo LIB_OBJS: $(LIB_OBJS)
