# Version of booster
GIT_VERSION := $(shell git describe --abbrev=10 --dirty --always --tags)

UNAME := $(shell uname)

#Use -DDEBUG to print debug output.  Use -DNDEBUG to ignore asserts.

# Profiling:
#CFLAGS = -Wall -pg -g -DVERSION=\"$(GIT_VERSION)\" -fno-stack-protector
#CFLAGS_OMP = -Wall -pg -g -fopenmp

# Debug output:
#CFLAGS = -Wall -g -DVERSION=\"$(GIT_VERSION)\" -DDEBUG -fno-stack-protector
#CFLAGS_OMP = -Wall -g -fopenmp -DDEBUG

# Debug without output:
#CFLAGS = -Wall -g -DVERSION=\"$(GIT_VERSION)\"
#CFLAGS_OMP = -Wall -g -fopenmp

# Production with asserts:
#CFLAGS = -Wall -O3 -DVERSION=\"$(GIT_VERSION)\"
#CFLAGS_OMP = -Wall -O3 -fopenmp

# Production without asserts:
CFLAGS = -Wall -O3 -DVERSION=\"$(GIT_VERSION)\" -DNDEBUG
CFLAGS_OMP = -Wall -O3 -fopenmp -DNDEBUG


# Compiler: gcc
ifeq ($(cross),win32)
        CC = i686-w64-mingw32-gcc
else
	ifeq ($(cross),win64)
		CC = x86_64-w64-mingw32-gcc
	else
		ifeq ($(cross),linux32)
			CFLAGS_OMP += -m32
			CFLAGS += -m32
		else
			CC = gcc
		endif
	endif
endif

ifeq ($(UNAME),Darwin)
	CFLAGS_OMP += -static-libgcc
#else
#	CFLAGS_OMP += -static
endif

LIBS = -lm
OBJS = hashtables_bfields.o  tree.o stats.o prng.o hashmap.o version.o sort.o io.o tree_utils.o bitset_index.o rapid_transfer.o heavy_paths.o debug.o kludge.o node_stack.o

# default target
ALL = booster

INSTALL_PATH=$$HOME/bin/

all : $(ALL)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

# ****
# the "booster" supports. Needs ref tree and bt trees.
# ****
booster: $(OBJS) booster.c
	$(CC) $(CFLAGS_OMP) -o $@ $^ $(LIBS)


# ****
# TESTS
# ****
tests: $(OBJS) test.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test : tests
	./tests

.PHONY: clean

clean:
	rm -f *~ *.o $(ALL) tests 
	rm -rf *.dSYM

install: all
	mkdir -p $(INSTALL_PATH)
	cp $(ALL) $(INSTALL_PATH)

uninstall:
	rm $(addprefix $(INSTALL_PATH),$(ALL))
