AM_CFLAGS = $(OTAIINC)
AM_CXXFLAGS = $(OTAIINC) -I$(top_srcdir)/lib

lib_LTLIBRARIES = libotaimeta.la libotaimetadata.la

BUILT_SOURCES = $(top_srcdir)/OTAI/meta/otaimetadata.c $(top_srcdir)/OTAI/meta/otaimetadata.h

$(top_srcdir)/OTAI/meta/otaimetadata.c $(top_srcdir)/OTAI/meta/otaimetadata.h: otaimeta-gen

otaimeta-gen:
	make -C $(top_srcdir)/OTAI/meta

clean-local:
	make -C $(top_srcdir)/OTAI/meta clean

libotaimeta_la_SOURCES =  \
					  $(top_srcdir)/OTAI/meta/otaimetadata.c \
					  $(top_srcdir)/OTAI/meta/otaimetadatautils.c \
					  $(top_srcdir)/OTAI/meta/otaiserialize.c

libotaimeta_la_CFLAGS = $(DBGFLAGS) $(AM_CFLAGS)

libotaimetadata_la_SOURCES = \
							Globals.cpp \
							Meta.cpp \
							Notification.cpp \
							NotificationFactory.cpp \
							NotificationLinecardStateChange.cpp \
							NotificationOcmNotify.cpp \
							OtaiAttributeList.cpp \
							OtaiInterface.cpp \
							OtaiSerialize.cpp 


libotaimetadata_la_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON)
libotaimetadata_la_LIBADD = -lhiredis -lswsscommon libotaimeta.la

