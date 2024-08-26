AM_CXXFLAGS = $(OTAIINC) -I$(top_srcdir)/lib -I$(top_srcdir)/vslib

if OTAIVS
OTAILIB=-L$(top_srcdir)/vslib/.libs -lotaivs
else
OTAILIB=-lotai
endif

bin_PROGRAMS = syncd syncd_request_shutdown 

noinst_LIBRARIES = libSyncd.a libSyncdRequestShutdown.a

libSyncd_a_SOURCES = \
				RedisSelectableChannel.cpp \
				SelectableChannel.cpp \
				RedisNotificationProducer.cpp \
				Syncd.cpp \
				RedisClient.cpp \
				RequestShutdownCommandLineOptions.cpp \
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
				OtaiLinecard.cpp \
				FlexCounterManager.cpp \
				FlexCounter.cpp \
				VidManager.cpp \
				OtaiAttr.cpp \
				VendorOtai.cpp \
				syncd_main.cpp \
				NotificationQueue.cpp \
				CommandLineOptions.cpp \
				CommandLineOptionsParser.cpp \
				pm/Collector.cpp \
				pm/OtaiAttrCollector.cpp \
				pm/OtaiStatCollector.cpp \
				pm/OtaiGaugeCollector.cpp

libSyncd_a_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON)

syncd_SOURCES = main.cpp
syncd_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON)
syncd_LDADD = libSyncd.a $(top_srcdir)/lib/libOtaiRedis.a -L$(top_srcdir)/meta/.libs -lotaimetadata -lotaimeta \
			  -ldl -lhiredis -lswsscommon $(OTAILIB) -lpthread

libSyncdRequestShutdown_a_SOURCES = \
								 RequestShutdown.cpp \
								 RequestShutdownCommandLineOptions.cpp \
								 RequestShutdownCommandLineOptionsParser.cpp

libSyncdRequestShutdown_a_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON)

syncd_request_shutdown_SOURCES = syncd_request_shutdown.cpp
syncd_request_shutdown_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON)
syncd_request_shutdown_LDADD = libSyncdRequestShutdown.a $(top_srcdir)/lib/libOtaiRedis.a -lhiredis -lswsscommon -lpthread