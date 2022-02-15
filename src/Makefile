#
#       !!!! Do NOT edit this makefile with an editor which replace tabs by spaces !!!!   
#
##############################################################################################
#
# On command line:
#
# make all = Create project
#
# make clean = Clean project files.
#
# To rebuild project do "make clean" and "make all".
#
# Included originally in the yagarto projects. Original Author : Michael Fischer
# Modified to suit our purposes by Hussam Al-Hertani
# Use at your own risk!!!!!
##############################################################################################
# Start of default section
#
CC   = $(CCPREFIX)gcc
CP   = $(CCPREFIX)objcopy
AS   = $(CCPREFIX)gcc -x assembler-with-cpp
INSTALL=install

ifdef PROJECT_VERSION
override PROJECT_VERSION := -DPROJECT_VERSION=$(PROJECT_VERSION)
endif


DSTDIR = .

# List all user libraries here
LIBS = m

BINFILES = unipihostname unipicheck

# Define optimisation level here
#OPT = -Ofast # Very aggressive
#OPT = -Os # For size
OPT = -O2 # Medium

# Define debug level here
DBG = -g0 # No debug
#DBG = -g # Medium debug
#DBG = -g2 # All debug symbols 


INCDIR  = $(patsubst %,-I%, $(INCDIRS))
LIBDIR  = $(patsubst %,-L%, $(LIBDIRS))
LIB     = $(patsubst %,-l%, $(LIBS))

OBJS  = $(SRC:.c=.o)
SPIOBJS  = $(SPISRC:.c=.o)

LDFLAGS = $(LIBDIR) $(LIB)
#CPFLAGS = -g -fomit-frame-pointer -Wall -Wstrict-prototypes -fverbose-asm 
CPFLAGS = -Wall -Wstrict-prototypes $(OPT) $(DBG)

#
# makefile rules
#
all: $(BINFILES)

%.o: %.c
	$(CC) -c $(CPFLAGS) $(LDFLAGS) -I . $(INCDIR) $(PROJECT_VERSION) $< -o $@

unipihostname:  unipihostname.o unipiutil.o
	$(CC) unipihostname.o unipiutil.o -o $@
	@chmod +x $@

unipicheck:  unipicheck.o unipiutil.o
	$(CC) unipicheck.o unipiutil.o -o $@
	@chmod +x $@

install:
	$(INSTALL) -D $(BINFILES) -t $(DESTDIR)/opt/unipi/tools

clean:
	-rm -f *.o
	-rm -f $(BINFILES)
#	-rm -rf $(SRC:.c=.lst)
#	-rm -f ../bin/*

# *** EOF ***