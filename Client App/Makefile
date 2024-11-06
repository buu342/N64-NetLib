CODEFILES   = app.cpp serverbrowser.cpp clientwindow.cpp romdownloader.cpp packets.cpp helper.cpp sha256.cpp
LIBFILES    = Include/flashcart.a Include/ftd2xx.lib
BUILDDIR = build
CODEOBJECTS = $(CODEFILES:%.cpp=${BUILDDIR}/%.o)
PROGNAME = NetLibBrowser
OS_NAME := $(shell uname -s)

# -------------------------------------------------------------------------
# These are configurable options:
# -------------------------------------------------------------------------

# C++ compiler 
CXX = `$(WX_CONFIG) --cxx`

# Standard flags for C++ 
CXXFLAGS ?= -D LINUX
ifeq ($(OS_NAME),Darwin)
	CXXFLAGS += -D MACOS
endif

# Standard preprocessor flags (common for CC and CXX) 
CPPFLAGS ?= 

# Standard linker flags 
ifeq ($(OS_NAME),Darwin)
	HOMEBREW_PREFIX=$(shell brew --prefix)
	HOMEBREW_CELLAR=$(shell brew --cellar)
	CXXFLAGS += -I$(HOMEBREW_PREFIX)/include
	LDFLAGS ?= $(shell pkg-config --libs-only-other --static libftdi1) $(HOMEBREW_CELLAR)/libusb/1.0.27/lib/libusb-1.0.a $(HOMEBREW_CELLAR)/libftdi/1.5_2/lib/libftdi1.a
else
	LDFLAGS ?= -l:libftdi1.a -l:libusb-1.0.a -ludev
endif

# Location and arguments of wx-config script 
WX_CONFIG ?= wx-config

LIBRARIES ?= core,net,base,richtext,html,xml

# Port of the wx library to build against [gtk1,gtk2,msw,x11,motif,osx_cocoa,osx_carbon,dfb]
WX_PORT ?= $(shell $(WX_CONFIG) --query-toolkit)

# Use DLL build of wx library to use? [0,1]
WX_SHARED ?= $(shell if test -z `$(WX_CONFIG) --query-linkage`; then echo 1; else echo 0; fi)

# Compile Unicode build of wxWidgets? [0,1]
WX_UNICODE ?= $(shell $(WX_CONFIG) --query-chartype | sed 's/unicode/1/;s/ansi/0/')

# Version of the wx library to build against. 
WX_VERSION ?= $(shell $(WX_CONFIG) --query-version | sed -e 's/\([0-9]*\)\.\([0-9]*\)/\1\2/')



# -------------------------------------------------------------------------
# Do not modify the rest of this file!
# -------------------------------------------------------------------------

### Variables: ###
CPPDEPS = -MT$@ -MF`echo $@ | sed -e 's,\.o$$,.d,'` -MD -MP
WX_VERSION_MAJOR = $(shell echo $(WX_VERSION) | cut -c1,1)
WX_VERSION_MINOR = $(shell echo $(WX_VERSION) | cut -c2,2)
WX_CONFIG_FLAGS = $(WX_CONFIG_UNICODE_FLAG) $(WX_CONFIG_SHARED_FLAG) \
	--toolkit=$(WX_PORT) --version=$(WX_VERSION_MAJOR).$(WX_VERSION_MINOR)
PROGRAM_CXXFLAGS = -I. `$(WX_CONFIG) --cxxflags $(WX_CONFIG_FLAGS)` $(CPPFLAGS) \
	$(CXXFLAGS)

### Conditionally set variables: ###

ifeq ($(WX_UNICODE),0)
	WX_CONFIG_UNICODE_FLAG = --unicode=no
endif
ifeq ($(WX_UNICODE),1)
	WX_CONFIG_UNICODE_FLAG = --unicode=yes
endif
ifeq ($(WX_SHARED),0)
	WX_CONFIG_SHARED_FLAG = --static=yes
endif
ifeq ($(WX_SHARED),1)
	WX_CONFIG_SHARED_FLAG = --static=no
endif

ifeq ($(DEBUG),1)
	CFLAGS += -Wall -Wextra -ggdb -O0 -g
endif


### Targets: ###

all: test_for_selected_wxbuild $(PROGNAME)

install: 

uninstall: 

clean: 
	rm -f -r ${BUILDDIR}

test_for_selected_wxbuild: 
	@$(WX_CONFIG) $(WX_CONFIG_FLAGS)

$(PROGNAME): $(CODEOBJECTS)
	$(CXX) -o ${BUILDDIR}/$@ $(CODEOBJECTS) $(LIBFILES) $(LDFLAGS) `$(WX_CONFIG) $(WX_CONFIG_FLAGS) --libs $(LIBRARIES)` -no-pie

build/%.o: %.cpp | ${BUILDDIR}
	$(CXX) -c $(CFLAGS) -o $@ $(PROGRAM_CXXFLAGS) $(CPPDEPS) $<

${BUILDDIR}:
	mkdir -p $@

.PHONY: all install uninstall clean


# Dependencies tracking:
-include ./*.d