AM_CPPFLAGS = -I$(top_srcdir)/lib/inc -I$(top_srcdir)/vslib/inc -I$(top_srcdir)/OTAI/inc -I$(top_srcdir)/OTAI/meta

bin_PROGRAMS = syncd syncd_request_shutdown dump_asic_db 

if DEBUG
DBGFLAGS = -ggdb -DDEBUG
else
DBGFLAGS = -g
endif

if OTAIVS
OTAILIB=-L$(top_srcdir)/vslib/src/.libs -lotaivs
else
OTAILIB=-lotai
endif

noinst_LIBRARIES = libSyncd.a libSyncdRequestShutdown.a
libSyncd_a_SOURCES = \
				OtaiLinecardInterface.cpp \
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
				OtaiDiscovery.cpp \
				OtaiLinecard.cpp \
				FlexCounterManager.cpp \
				FlexCounter.cpp \
				VidManager.cpp \
				AsicOperation.cpp \
				OtaiObj.cpp \
				OtaiAttr.cpp \
				VendorOtai.cpp \
				syncd_main.cpp \
				TimerWatchdog.cpp \
				NotificationQueue.cpp \
				CommandLineOptions.cpp \
				CommandLineOptionsParser.cpp \
				pm/Collector.cpp \
				pm/OtaiAttrCollector.cpp \
				pm/OtaiStatCollector.cpp \
				pm/OtaiGaugeCollector.cpp

libSyncd_a_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON) $(OTAIFLAGS) -std=c++14

syncd_SOURCES = main.cpp
syncd_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON) $(OTAIFLAGS)
syncd_LDADD = libSyncd.a ../lib/src/libotaiRedis.a -L$(top_srcdir)/meta/.libs -lotaimetadata -lotaimeta -ldl -lhiredis -lswsscommon $(OTAILIB) -lpthread

libSyncdRequestShutdown_a_SOURCES = \
								 RequestShutdown.cpp \
								 RequestShutdownCommandLineOptions.cpp \
								 RequestShutdownCommandLineOptionsParser.cpp

libSyncdRequestShutdown_a_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)

syncd_request_shutdown_SOURCES = syncd_request_shutdown.cpp
syncd_request_shutdown_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
syncd_request_shutdown_LDADD = libSyncdRequestShutdown.a ../lib/src/libotaiRedis.a -lhiredis -lswsscommon -lpthread

dump_asic_db_SOURCES = dump_asic_db.cpp
dump_asic_db_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
dump_asic_db_LDADD = libSyncd.a ../lib/src/libotaiRedis.a -L$(top_srcdir)/meta/.libs -lotaimetadata -lotaimeta -ldl -lhiredis -lswsscommon -lpthread
