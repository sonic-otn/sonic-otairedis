# since we have LAI sumbodule we will use submodule headers
AM_CPPFLAGS = -I$(top_srcdir)/LAI/inc -I$(top_srcdir)/LAI/meta -I$(top_srcdir)/lib/inc

if DEBUG
DBGFLAGS = -ggdb -D_DEBUG_
else
DBGFLAGS = -g
endif

lib_LTLIBRARIES = liblaimeta.la liblaimetadata.la

WARN = \
	-ansi \
	-Wall \
	-Wcast-align \
	-Wcast-qual \
	-Wconversion \
	-Wdisabled-optimization \
	-Werror \
	-Wextra \
	-Wextra \
	-Wfloat-equal \
	-Wformat=2 \
	-Wformat-nonliteral \
	-Wformat-security \
	-Wformat-y2k \
	-Wimport \
	-Winit-self \
	-Winline \
	-Winvalid-pch \
	-Wmissing-field-initializers \
	-Wmissing-format-attribute \
	-Wmissing-include-dirs \
	-Wmissing-noreturn \
	-Wno-aggregate-return \
	-Wno-padded \
	-Wno-switch-enum \
	-Wno-unused-parameter \
	-Wpacked \
	-Wpointer-arith \
	-Wredundant-decls \
	-Wshadow \
	-Wstack-protector \
	-Wstrict-aliasing=3 \
	-Wswitch \
	-Wswitch-default \
	-Wunreachable-code \
	-Wunused \
	-Wvariadic-macros \
	-Wwrite-strings

# we are not adding CPPFLAGS since they are added
# automatic to each C preprocessor, and currently
# we add -std=c++11 flag to CFLAGS_COMMON which
# will cause compilaton error on C files
# and we already confirmed that LAI/meta files
# can be compiled with success
#
# NOTE: this compilation could be moved to configure.ac

liblaimeta_la_SOURCES =  \
					  $(top_srcdir)/LAI/meta/laimetadata.c \
					  $(top_srcdir)/LAI/meta/laimetadatautils.c \
					  $(top_srcdir)/LAI/meta/laiserialize.c

liblaimeta_la_CFLAGS = $(DBGFLAGS) $(WARN)

$(top_srcdir)/LAI/meta/laimetadata.h $(top_srcdir)/LAI/meta/laimetadata.c: laimeta-gen

laimeta-gen:
	make -C $(top_srcdir)/LAI/meta

clean-local:
	make -C $(top_srcdir)/LAI/meta clean

liblaimetadata_la_SOURCES = \
							../lib/src/LaiInterface.cpp \
							LaiAttributeList.cpp \
							laiserialize.cpp \
							LaiAttrWrapper.cpp \
							OidRefCounter.cpp \
							AttrKeyMap.cpp \
							Globals.cpp \
							LaiObject.cpp \
							LaiObjectCollection.cpp \
							PortRelatedSet.cpp \
                                                        MetaKeyHasher.cpp \
							Meta.cpp


BUILT_SOURCES = $(top_srcdir)/LAI/meta/laimetadata.h $(top_srcdir)/LAI/meta/laimetadata.c

liblaimetadata_la_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
liblaimetadata_la_LIBADD = -lhiredis -lswsscommon liblaimeta.la

