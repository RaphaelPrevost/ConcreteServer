 ###############################################################################
 #  Concrete Server                                                            #
 #  Copyright (c) 2005-2019 Raphael Prevost <raph@el.bzh>                      #
 #                                                                             #
 #  This software is a computer program whose purpose is to provide a          #
 #  framework for developing and prototyping network services.                 #
 #                                                                             #
 #  This software is governed by the CeCILL  license under French law and      #
 #  abiding by the rules of distribution of free software.  You can  use,      #
 #  modify and/ or redistribute the software under the terms of the CeCILL     #
 #  license as circulated by CEA, CNRS and INRIA at the following URL          #
 #  "http://www.cecill.info".                                                  #
 #                                                                             #
 #  As a counterpart to the access to the source code and  rights to copy,     #
 #  modify and redistribute granted by the license, users are provided only    #
 #  with a limited warranty  and the software's author,  the holder of the     #
 #  economic rights,  and the successive licensors  have only  limited         #
 #  liability.                                                                 #
 #                                                                             #
 #  In this respect, the user's attention is drawn to the risks associated     #
 #  with loading,  using,  modifying and/or developing or reproducing the      #
 #  software by the user in light of its specific status of free software,     #
 #  that may mean  that it is complicated to manipulate,  and  that  also      #
 #  therefore means  that it is reserved for developers  and  experienced      #
 #  professionals having in-depth computer knowledge. Users are therefore      #
 #  encouraged to load and test the software's suitability as regards their    #
 #  requirements in conditions enabling the security of their systems and/or   #
 #  data to be ensured and,  more generally, to use and operate it in the      #
 #  same conditions as regards security.                                       #
 #                                                                             #
 #  The fact that you are presently reading this means that you have had       #
 #  knowledge of the CeCILL license and that you accept its terms.             #
 #                                                                             #
 ###############################################################################

PROJECT = CONCRETE

# Concrete Server configuration flags
# Available CONFIG flags:
# -DDEBUG                         : enable debug messages
# -DDEBUG_SQL                     : display every SQL queries
# -D_ENABLE_DB                    : enable the database API
# -D_ENABLE_MYSQL                 : enable MySQL driver
# -D_ENABLE_SQLITE                : enable SQLite driver
# -D_ENABLE_HTTP                  : enable native HTTP handling
# -D_ENABLE_UDP                   : allow use of UDP sockets
# -D_ENABLE_SSL                   : allow use of SSL/TLS secure sockets
# -D_ENABLE_SERVER                : enable the Mammouth Server
# -D_ENABLE_RANDOM                : enable builtin PRNG
# -D_ENABLE_HASHTABLE             : enable builtin hashtable implementation
# -D_ENABLE_PRIVILEGE_SEPARATION  : drop privileges in the server process
# -D_ENABLE_BUILTIN_PLUGIN        : embed a default plugin
# -D_ENABLE_CONFIG                : XML configuration file
# -D_USE_BIG_FDS=<int>            : enable the use of more than FD_SETSIZE fds

CONFIG  = -D_ENABLE_SERVER -D_ENABLE_UDP -D_ENABLE_SSL -D_ENABLE_HTTP \
          -D_ENABLE_RANDOM -D_ENABLE_HASHTABLE -D_ENABLE_TRIE \
          -D_ENABLE_PCRE \
          -D_ENABLE_DB -D_ENABLE_MYSQL -D_ENABLE_SQLITE \
          -D_ENABLE_CONFIG  \
          -D_BUILTIN_PLUGIN -D_USE_BIG_FDS=4095

# Plugins configuration flags
# Available PLGCONF flags:
# -D_DIALMSN_ENABLE_BOT           : enable the bot in the DialMessenger plugin

PLGCONF = -D_DIALMSN_ENABLE_BOT

# Files
CC      = gcc
OBJBIN  = $(addsuffix .o, $(basename $(wildcard *.c)))
OBJLIB  = $(addsuffix .o, $(basename $(wildcard lib/*.c))) \
          $(addsuffix .o, $(basename $(wildcard lib/util/*.c))) \
          lib/ports/m_ports.o
OBJTEST = $(addsuffix .o, $(basename $(wildcard test/*.c)))
OBJPROF = $(addsuffix .gcno, $(basename $(wildcard lib/*.c))) \
          $(addsuffix .gcno, $(basename $(wildcard lib/util/*.c))) \
          $(addsuffix .gcno, $(basename $(wildcard test/*.c))) \
          $(addsuffix .gcda, $(basename $(wildcard lib/*.c))) \
          $(addsuffix .gcda, $(basename $(wildcard lib/util/*.c))) \
          $(addsuffix .gcda, $(basename $(wildcard test/*.c))) \
          lib/ports/m_ports.gcno lib/ports/m_ports.gcda *.gcno *.gcda \
          gmon.out
LIBS    = -lpthread
BIN     = concrete
LIB     = concrete
PLUGINS = $(shell find plugins/* -type d | grep -v .svn)

# Build options:
# You can select the build options with the make target.
# make       : enable FINAL build options
# make all   : "
# make plugin: "
# make debug : enable DEBUG build options
# make test  : run the unit tests and enable the DEBUG build options

# Build settings:
# FINAL corresponds to production settings
# DEBUG is used for debug builds
# FLAGS will be passed as CFLAGS to the compiler
# SHARED is the set of required compiler options to produce a shared object
# LIBFLAGS is other specific flags used to build a library

BUILD    =
FINAL    = -O2 -pipe
DEBUG    = -O0 -g -pg -fprofile-generate -fstack-protector -DDEBUG -DDEBUG_SQL
TRACE    = -O0 -g -pg -fprofile-generate
FLAGS    = -std=c99 -W -Wall $(BUILD) -Wpointer-arith

SHARED   =
LIBFLAGS =

# Installation
#PREFIX  = /usr
PREFIX =
CONFDIR =
DESTDIR =
SHAREDIR =

# system features
OS           = $(shell uname)
GCC_ALIASED  = $(shell gcc --version | head -1 | cut -d\  -f1)
GCC_MAJ      = $(shell gcc --version | head -1 | cut -d\  -f3 | cut -d. -f1)
GCC_MIN      = $(shell gcc --version | head -1 | cut -d\  -f3 | cut -d. -f2)
HAS_DL       = $(shell ls /usr/include/dlfcn.h 2> /dev/null)
HAS_RL       = $(shell ls /usr/include/readline/readline.h 2> /dev/null)
HAS_SSL      = $(shell ls /usr/include/openssl/ssl.h 2> /dev/null)
HAS_SHADOW   = $(shell ls /usr/include/shadow.h 2> /dev/null)
HAS_ZLIB     = $(shell ls /usr/include/zlib.h 2> /dev/null)
HAS_POLL     = $(shell ls /usr/include/poll.h 2> /dev/null)
HAS_PCRE     = $(shell ls /usr/include/pcre.h 2> /dev/null)
HAS_LIBXML   = $(shell which xml2-config 2> /dev/null)
HAS_MYSQL    = $(shell which mysql_config 2> /dev/null)
HAS_SQLITE   = $(shell ls /usr/include/sqlite3.h 2> /dev/null)
HAS_LIBMAGIC = $(shell ls /usr/include/magic.h 2> /dev/null)
HAS_ICONV    = $(shell ls /usr/include/iconv.h 2> /dev/null)
ARCH         = $(shell uname -m)

# GCC options
ifeq ($(CC),gcc)
ifeq ($(GCC_ALIASED),gcc)
FINAL   += -finline -mpreferred-stack-boundary=4 -minline-all-stringops
FLAGS   += -findirect-inlining -posix
# disable some warnings
ifeq ($(GCC_MAJ), 6)
FLAGS += -Wno-misleading-indentation
endif
endif
endif

ifneq ($(PREFIX), )
CONFIG += -DPREFIX=$(PREFIX)
endif

ifneq ($(CONFDIR), )
CONFIG += -DCONFDIR=$(CONFDIR)
endif

ifneq ($(SHAREDIR), )
CONFIG += -DSHAREDIR=$(SHAREDIR)
endif

# position independent code is required for x86_64 libraries
ifeq ($(ARCH), x86_64)
LIBFLAGS += -fPIC
endif

# check for shadow.h
ifeq ($(HAS_SHADOW),/usr/include/shadow.h)
CONFIG += -DHAS_SHADOW
endif

# check for zlib
ifeq ($(HAS_ZLIB),/usr/include/zlib.h)
CONFIG += -DHAS_ZLIB
LIBS += -lz
endif

# check for poll(2)
ifeq ($(HAS_POLL),/usr/include/poll.h)
CONFIG += -DHAS_POLL
endif

# check for libxml2
ifneq ($(HAS_LIBXML), )
LIBS += $(shell xml2-config --libs)
FLAGS += $(shell xml2-config --cflags)
CONFIG += -DHAS_LIBXML
endif

# check for libmagic
#ifneq ($(HAS_LIBMAGIC), )
#LIBS += -lmagic
#CONFIG += -DHAS_LIBMAGIC
#endif

# check for iconv
ifeq ($(HAS_ICONV),/usr/include/iconv.h)
CONFIG += -DHAS_ICONV
endif

PCRE_ENABLED=$(shell echo $(CONFIG) | grep -c D_ENABLE_PCRE)

# check for PCRE
ifeq ($(PCRE_ENABLED),1)
ifneq ($(HAS_PCRE), )
LIBS += -lpcre
CONFIG += -DHAS_PCRE
endif
endif

# check if SSL support was enabled
SSL_ENABLED=$(shell echo $(CONFIG) | grep -c D_ENABLE_SSL)

# check for OpenSSL
ifeq ($(SSL_ENABLED),1)
ifeq ($(HAS_SSL),/usr/include/openssl/ssl.h)
CONFIG += -DHAS_SSL
LIBS += -lcrypt -lssl
endif
endif

# check if SQLite support was enabled
SQLITE_ENABLED=$(shell echo $(CONFIG) | grep -c D_ENABLE_SQLITE)

# check for SQLite
ifeq ($(SQLITE_ENABLED),1)
ifeq ($(HAS_SQLITE),/usr/include/sqlite3.h)
CONFIG += -DHAS_SQLITE
LIBS += -lsqlite3
endif
endif

# check if MySQL support was enabled
MYSQL_ENABLED=$(shell echo $(CONFIG) | grep -c D_ENABLE_MYSQL)

# check for MySQL
ifeq ($(MYSQL_ENABLED),1)
ifneq ($(HAS_MYSQL), )
CONFIG += -DHAS_MYSQL
LIBS += $(shell mysql_config --libs_r)
endif
endif

# Linux specific settings
ifeq ($(OS),Linux)
LIBS   += -ldl -lrt
# required for clang
FLAGS  += -fgnu89-inline
ifeq ($(CC),gcc)
FLAGS  += -rdynamic
endif
SHARED += -shared
PLUGIN += $(SHARED)
endif

# Mac OS X specific settings
ifeq ($(OS),Darwin)
# APPLE gcc default preprocessor can't cope with variadic macros
FLAGS += -no-cpp-precomp
ifeq ($(HAS_DL), /usr/include/dlfcn.h)
# Mac OS >= 10.3
LIBS   += -ldl
SHARED += -dynamiclib
PLUGIN += -bundle -undefined dynamic_lookup
else
# Mac OS 10.2
SHARED += -bundle
PLUGIN += -bundle -bundle_loader $(BIN)
endif
# iconv needs to be explicitly linked on Mac OS X
ifeq ($(HAS_ICONV),/usr/include/iconv.h)
LIBS += -liconv
endif
endif

.PHONY: all debug lib dbglib server dbgserver plugin test install clean

# Build targets

# build the server, the library and the plugins with optimizations enabled
all: BUILD = $(FINAL)
all: plugin
# build server, library and plugins with debug enabled
debug: BUILD = $(DEBUG)
debug: plugin
# build server, library and plugins with tracing support
trace: BUILD = $(TRACE)
trace: plugin

# build only the library, with optimizations
lib: BUILD = $(FINAL)
lib: $(LIB)
# library only, with debug
dbglib: BUILD = $(DEBUG)
dbglib: $(LIB)

# build the server and the library with optimizations enabled
server: BUILD = $(FINAL)
server: $(BIN)
# server and library with debug
dbgserver: BUILD = $(DEBUG)
dbgserver: $(BIN)

# build server, library and plugins, and install them in the DESTDIR folder
install: BUILD = $(FINAL)

# build server, library and plugins, and run the unit tests suite
test: BUILD = $(DEBUG)

# same than test, but with optimizations enabled
testfinal: BUILD = $(FINAL)

# Build rules
.c.o:
	@echo "CC: $@"
	@$(CC) -c $< -o $@ $(CFLAGS) $(CONFIG) -Ilib
	@$(CC) -MM $< $(CFLAGS) $(CONFIG) -Ilib > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
     sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

$(BIN): CFLAGS = $(FLAGS)
$(BIN): $(OBJBIN) lib$(LIB)
	@echo "LD: $(BIN)"
	@$(CC) $(CFLAGS) $(CONFIG) $(OBJBIN) -L. -l$(LIB) -o $(BIN)

lib$(LIB): CFLAGS = $(FLAGS) $(LIBFLAGS)
lib$(LIB): $(OBJLIB)
	@echo "LD: lib$(LIB).so"
	@$(CC) $(SHARED) $(CFLAGS) $(CONFIG) $(OBJLIB) $(LIBS) -o lib$(LIB).so

$(OBJTEST): lib$(LIB)

plugin: CFLAGS = $(FLAGS) $(LIBFLAGS)
plugin: $(BIN) lib$(LIB)
	@for PLG in $(PLUGINS); do \
		echo "LD: $${PLG}"; \
		$(CC) $(PLUGIN) $(CFLAGS) $(CONFIG) $(PLGCONF) $${PLG}/*.c \
		-L. -l$(LIB) -Ilib -o $${PLG}.so; \
	done;

test: CFLAGS = $(FLAGS)
test: $(OBJTEST)
	@$(CC) $(OBJTEST) $(CFLAGS) $(CONFIG) $(LIBS) -L. -l$(LIB) -o test.out
	@echo "TEST"
	@-(LD_LIBRARY_PATH=. gdb -silent ./test.out -ex 'handle SIGPIPE nostop' -ex r)

testfinal: CFLAGS = $(FLAGS)
testfinal: $(OBJTEST)
	@$(CC) $(OBJTEST) $(CFLAGS) $(CONFIG) $(LIBS) -L. -l$(LIB) -o test.out
	@echo "TEST"
	@-(LD_LIBRARY_PATH=. ./test.out)

install: plugin
	@echo "INSTALL"
	@mkdir -p $(DESTDIR)$(PREFIX)/bin
	@mkdir -p $(DESTDIR)$(PREFIX)/lib/concrete/plugins
	@strip $(BIN) lib$(LIB).so plugins/*.so
	@cp $(BIN) $(DESTDIR)$(PREFIX)/bin
	@cp lib$(LIB).so $(DESTDIR)$(PREFIX)/lib/concrete/
	@cp plugins/*.so $(DESTDIR)$(PREFIX)/lib/concrete/plugins/

clean:
	@echo "CLEAN"
	@rm -f $(BIN) lib$(LIB).so $(PLG).so $(OBJBIN) $(OBJLIB) $(OBJPLG) *~ \
	$(OBJTEST) $(OBJPROF) lib/*~ lib/util/*~ lib/ports/*~ test/*~ test.out \
	plugins/*.so plugins/*/*~ lib/*.o lib/util/*.o lib/ports/*.o test/*.o \
	*.d lib/*.d lib/util/*.d lib/ports/*.d test/*.d plugins/*/*.d

-include $(OBJBIN:.o=.d)
-include $(OBJLIB:.o=.d)
-include $(OBJTEST:.o=.d)
