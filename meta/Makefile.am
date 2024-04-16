# since we have OTAI sumbodule we will use submodule headers
AM_CPPFLAGS = -I$(top_srcdir)/OTAI/inc -I$(top_srcdir)/OTAI/meta -I$(top_srcdir)/lib/inc

if DEBUG
DBGFLAGS = -ggdb -D_DEBUG_
else
DBGFLAGS = -g
endif

lib_LTLIBRARIES = libotaimeta.la libotaimetadata.la

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
# and we already confirmed that OTAI/meta files
# can be compiled with success
#
# NOTE: this compilation could be moved to configure.ac

libotaimeta_la_SOURCES =  \
					  $(top_srcdir)/OTAI/meta/otaimetadata.c \
					  $(top_srcdir)/OTAI/meta/otaimetadatautils.c \
					  $(top_srcdir)/OTAI/meta/otaiserialize.c

libotaimeta_la_CFLAGS = $(DBGFLAGS) $(WARN)

$(top_srcdir)/OTAI/meta/otaimetadata.h $(top_srcdir)/OTAI/meta/otaimetadata.c: otaimeta-gen

otaimeta-gen:
	make -C $(top_srcdir)/OTAI/meta

clean-local:
	make -C $(top_srcdir)/OTAI/meta clean

libotaimetadata_la_SOURCES = \
							../lib/src/OtaiInterface.cpp \
							OtaiAttributeList.cpp \
							otaiserialize.cpp \
							OtaiAttrWrapper.cpp \
							OidRefCounter.cpp \
							AttrKeyMap.cpp \
							Globals.cpp \
							OtaiObject.cpp \
							OtaiObjectCollection.cpp \
							PortRelatedSet.cpp \
                                                        MetaKeyHasher.cpp \
							Meta.cpp


BUILT_SOURCES = $(top_srcdir)/OTAI/meta/otaimetadata.h $(top_srcdir)/OTAI/meta/otaimetadata.c

libotaimetadata_la_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
libotaimetadata_la_LIBADD = -lhiredis -lswsscommon libotaimeta.la

