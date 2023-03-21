AM_CPPFLAGS = -I$(top_srcdir)/lib/inc -I$(top_srcdir)/vslib/inc -I$(top_srcdir)/LAI/inc -I$(top_srcdir)/LAI/meta

bin_PROGRAMS = syncd syncd_request_shutdown dump_asic_db 

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g
endif

if LAIVS
LAILIB=-L$(top_srcdir)/vslib/src/.libs -llaivs
else
LAILIB=-llai
endif

noinst_LIBRARIES = libSyncd.a libSyncdRequestShutdown.a
libSyncd_a_SOURCES = \
				LaiLinecardInterface.cpp \
				RedisSelectableChannel.cpp \
				SelectableChannel.cpp \
				RedisNotificationProducer.cpp \
				Syncd.cpp \
				RedisClient.cpp \
				RequestShutdownCommandLineOptions.cpp \
				GlobalLinecardId.cpp \
				MetadataLogger.cpp \
				ServiceMethodTable.cpp \
				LinecardNotifications.cpp \
				VirtualOidTranslator.cpp \
				NotificationProcessor.cpp \
				NotificationHandler.cpp \
				FlexCounterReiniter.cpp \
				SingleReiniter.cpp \
				HardReiniter.cpp \
				SoftReiniter.cpp \
				LaiDiscovery.cpp \
				LaiLinecard.cpp \
				FlexCounterManager.cpp \
				FlexCounter.cpp \
				VidManager.cpp \
				AsicOperation.cpp \
				LaiObj.cpp \
				LaiAttr.cpp \
				VendorLai.cpp \
				syncd_main.cpp \
				TimerWatchdog.cpp \
				NotificationQueue.cpp \
				CommandLineOptions.cpp \
				CommandLineOptionsParser.cpp \
				pm/Collector.cpp \
				pm/LaiAttrCollector.cpp \
				pm/LaiStatCollector.cpp \
				pm/LaiGaugeCollector.cpp

libSyncd_a_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON) $(LAIFLAGS) -std=c++14

syncd_SOURCES = main.cpp
syncd_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON) $(LAIFLAGS)
syncd_LDADD = libSyncd.a ../lib/src/libLaiRedis.a -L$(top_srcdir)/meta/.libs -llaimetadata -llaimeta -ldl -lhiredis -lswsscommon $(LAILIB) -lpthread

libSyncdRequestShutdown_a_SOURCES = \
								 RequestShutdown.cpp \
								 RequestShutdownCommandLineOptions.cpp \
								 RequestShutdownCommandLineOptionsParser.cpp

libSyncdRequestShutdown_a_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)

syncd_request_shutdown_SOURCES = syncd_request_shutdown.cpp
syncd_request_shutdown_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
syncd_request_shutdown_LDADD = libSyncdRequestShutdown.a ../lib/src/libLaiRedis.a -lhiredis -lswsscommon -lpthread

dump_asic_db_SOURCES = dump_asic_db.cpp
dump_asic_db_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
dump_asic_db_LDADD = libSyncd.a ../lib/src/libLaiRedis.a -L$(top_srcdir)/meta/.libs -llaimetadata -llaimeta -ldl -lhiredis -lswsscommon -lpthread
