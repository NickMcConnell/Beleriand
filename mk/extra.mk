PACKAGE = faangband
PACKAGE_NAME = faangband
CC = gcc
CPP = gcc -E
DC = @DC@
ERLC = @ERLC@
OBJC = @OBJC@
AR = @AR@
LD = ${CC}
CFLAGS = -g -O2 -DHAVE_CONFIG_H -fno-strength-reduce -Wall  -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
CXXFLAGS = @CXXFLAGS@
CPPFLAGS =  -I.
DFLAGS = @DFLAGS@
ERLCFLAGS = @ERLCFLAGS@
OBJCFLAGS = @OBJCFLAGS@
LDFLAGS =  -lncurses  -lSM -lICE  -lX11  -L/usr/lib -lSDL -lSDL_image -lSDL_ttf -lSDL_mixer
LIBS = 
PROG_IMPLIB_NEEDED = @PROG_IMPLIB_NEEDED@
PROG_IMPLIB_LDFLAGS = @PROG_IMPLIB_LDFLAGS@
PROG_SUFFIX = 
LIB_CPPFLAGS = @LIB_CPPFLAGS@
LIB_CFLAGS = @LIB_CFLAGS@
LIB_LDFLAGS = @LIB_LDFLAGS@
LIB_PREFIX = @LIB_PREFIX@
LIB_SUFFIX = @LIB_SUFFIX@
INSTALL_LIB = @INSTALL_LIB@
UNINSTALL_LIB = @UNINSTALL_LIB@
CLEAN_LIB = @CLEAN_LIB@
LN_S = ln -s
MKDIR_P = /bin/mkdir -p
INSTALL = /usr/bin/install -c
SHELL = /bin/bash
SETEGID = 
prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
LIBDIR = ${libdir}
plugindir ?= ${libdir}/${PACKAGE}
datarootdir = ${prefix}/share
datadir = ${datarootdir}
game_datadir = @game_datadir@ 
includedir = ${prefix}/include
includesubdir ?= ${PACKAGE}
mandir = ${datarootdir}/man
mansubdir ?= man1
pdfdir ?= ${docdir}
ECHO_N ?= -n
bindir ?= ${exec_prefix}/bin
dvidir ?= ${docdir}
CPP ?= gcc -E
datadir ?= ${datarootdir}
CPPFLAGS ?=  -I.
SHELL ?= /bin/bash
program_transform_name ?= s,x,x,
CC ?= gcc
DEFS ?= -DHAVE_CONFIG_H
LIBS ?= 
infodir ?= ${datarootdir}/info
libexecdir ?= ${exec_prefix}/libexec
localedir ?= ${datarootdir}/locale
LTLIBOBJS ?= 
LDFLAGS ?=  -lncurses  -lSM -lICE  -lX11  -L/usr/lib -lSDL -lSDL_image -lSDL_ttf -lSDL_mixer
includedir ?= ${prefix}/include
libdir ?= ${exec_prefix}/lib
CFLAGS ?= -g -O2 -DHAVE_CONFIG_H -fno-strength-reduce -Wall  -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
localstatedir ?= ${prefix}/var
docdir ?= ${datarootdir}/doc/${PACKAGE_TARNAME}
INSTALL_SCRIPT ?= ${INSTALL}
EXEEXT ?= 
MV ?= /bin/mv
GLIB_LIBS ?= 
prefix ?= /usr/local
sharedstatedir ?= ${prefix}/com
GTK_CFLAGS ?= 
OBJEXT ?= o
PKG_CONFIG ?= 
sysconfdir ?= ${prefix}/etc
htmldir ?= ${docdir}
PACKAGE_STRING ?= FAangband 1.1.x
sbindir ?= ${exec_prefix}/sbin
CP ?= /bin/cp
SET_MAKE ?= 
ECHO_C ?= 
psdir ?= ${docdir}
oldincludedir ?= /usr/include
GTK_LIBS ?= 
PACKAGE ?= faangband
mandir ?= ${datarootdir}/man
PACKAGE_NAME ?= FAangband
ECHO_T ?= 
LIBOBJS ?= 
ac_ct_CC ?= gcc
PACKAGE_VERSION ?= 1.1.x
LN_S ?= ln -s
GREP ?= /bin/grep
INSTALL_DATA ?= ${INSTALL} -m 644
datarootdir ?= ${prefix}/share
RM ?= /bin/rm
INSTALL_PROGRAM ?= ${INSTALL}
exec_prefix ?= ${prefix}
EGREP ?= /bin/grep -E
VERSION ?= 1.1.x
pdfdir ?= ${docdir}
ECHO_N ?= -n
bindir ?= ${exec_prefix}/bin
dvidir ?= ${docdir}
CPP ?= gcc -E
datadir ?= ${datarootdir}
CPPFLAGS ?=  -I.
SHELL ?= /bin/bash
program_transform_name ?= s,x,x,
CC ?= gcc
DEFS ?= -DHAVE_CONFIG_H
LIBS ?= 
infodir ?= ${datarootdir}/info
libexecdir ?= ${exec_prefix}/libexec
localedir ?= ${datarootdir}/locale
LTLIBOBJS ?= 
LDFLAGS ?=  -lncurses  -lSM -lICE  -lX11  -L/usr/lib -lSDL -lSDL_image -lSDL_ttf -lSDL_mixer
includedir ?= ${prefix}/include
libdir ?= ${exec_prefix}/lib
CFLAGS ?= -g -O2 -DHAVE_CONFIG_H -fno-strength-reduce -Wall  -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
localstatedir ?= ${prefix}/var
docdir ?= ${datarootdir}/doc/${PACKAGE_TARNAME}
INSTALL_SCRIPT ?= ${INSTALL}
EXEEXT ?= 
MV ?= /bin/mv
GLIB_LIBS ?= 
prefix ?= /usr/local
sharedstatedir ?= ${prefix}/com
GTK_CFLAGS ?= 
OBJEXT ?= o
PKG_CONFIG ?= 
sysconfdir ?= ${prefix}/etc
htmldir ?= ${docdir}
PACKAGE_STRING ?= FAangband 1.1.x
sbindir ?= ${exec_prefix}/sbin
CP ?= /bin/cp
SET_MAKE ?= 
ECHO_C ?= 
psdir ?= ${docdir}
oldincludedir ?= /usr/include
GTK_LIBS ?= 
PACKAGE ?= faangband
mandir ?= ${datarootdir}/man
PACKAGE_NAME ?= FAangband
ECHO_T ?= 
LIBOBJS ?= 
ac_ct_CC ?= gcc
PACKAGE_VERSION ?= 1.1.x
LN_S ?= ln -s
GREP ?= /bin/grep
INSTALL_DATA ?= ${INSTALL} -m 644
datarootdir ?= ${prefix}/share
RM ?= /bin/rm
INSTALL_PROGRAM ?= ${INSTALL}
exec_prefix ?= ${prefix}
EGREP ?= /bin/grep -E
VERSION ?= 1.1.x
