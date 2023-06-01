AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib/inc -I$(top_srcdir)/LAI/inc -I$(top_srcdir)/LAI/meta

if DEBUG
DBGFLAGS = -ggdb -D_DEBUG_
else
DBGFLAGS = -g
endif

lib_LTLIBRARIES = liblairedis.la

noinst_LIBRARIES = libLaiRedis.a
libLaiRedis_a_SOURCES = \
						 PerformanceIntervalTimer.cpp \
						 Channel.cpp \
						 Context.cpp \
						 ContextConfigContainer.cpp \
						 ContextConfig.cpp \
						 LaiInterface.cpp \
						 LinecardConfig.cpp \
						 LinecardConfigContainer.cpp \
						 RedisChannel.cpp \
						 Lai.cpp \
						 Linecard.cpp \
						 LinecardContainer.cpp \
						 VirtualObjectIdManager.cpp \
						 Notification.cpp \
						 NotificationLinecardStateChange.cpp \
						 NotificationOcmNotify.cpp \
						 NotificationFactory.cpp \
						 RedisVidIndexGenerator.cpp \
						 Recorder.cpp \
						 RedisRemoteLaiInterface.cpp \
						 Utils.cpp \
						 SkipRecordAttrContainer.cpp

liblairedis_la_SOURCES = \
                         lai_redis_interfacequery.cpp \
                         lai_redis_linecard.cpp \
                         lai_redis_port.cpp \
                         lai_redis_transceiver.cpp \
                         lai_redis_logicalchannel.cpp \
                         lai_redis_otn.cpp \
                         lai_redis_ethernet.cpp \
                         lai_redis_physicalchannel.cpp \
                         lai_redis_och.cpp \
                         lai_redis_lldp.cpp \
                         lai_redis_assignment.cpp \
                         lai_redis_interface.cpp \
                         lai_redis_oa.cpp \
                         lai_redis_osc.cpp \
                         lai_redis_aps.cpp \
                         lai_redis_apsport.cpp \
                         lai_redis_attenuator.cpp \
                         lai_redis_wss.cpp \
                         lai_redis_mediachannel.cpp \
                         lai_redis_ocm.cpp \
                         lai_redis_otdr.cpp

libLaiRedis_a_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)

liblairedis_la_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
liblairedis_la_LIBADD = -lhiredis -lswsscommon libLaiRedis.a


