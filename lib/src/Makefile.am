AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib/inc -I$(top_srcdir)/OTAI/inc -I$(top_srcdir)/OTAI/meta

if DEBUG
DBGFLAGS = -ggdb -D_DEBUG_
else
DBGFLAGS = -g
endif

lib_LTLIBRARIES = libotairedis.la

noinst_LIBRARIES = libotaiRedis.a
libotaiRedis_a_SOURCES = \
						 PerformanceIntervalTimer.cpp \
						 Channel.cpp \
						 Context.cpp \
						 ContextConfigContainer.cpp \
						 ContextConfig.cpp \
						 OtaiInterface.cpp \
						 LinecardConfig.cpp \
						 LinecardConfigContainer.cpp \
						 RedisChannel.cpp \
						 Otai.cpp \
						 Linecard.cpp \
						 LinecardContainer.cpp \
						 VirtualObjectIdManager.cpp \
						 Notification.cpp \
						 NotificationLinecardStateChange.cpp \
						 NotificationOcmNotify.cpp \
						 NotificationFactory.cpp \
						 RedisVidIndexGenerator.cpp \
						 Recorder.cpp \
						 RedisRemoteOtaiInterface.cpp \
						 Utils.cpp \
						 SkipRecordAttrContainer.cpp

libotairedis_la_SOURCES = \
                         otai_redis_interfacequery.cpp \
                         otai_redis_linecard.cpp \
                         otai_redis_port.cpp \
                         otai_redis_transceiver.cpp \
                         otai_redis_logicalchannel.cpp \
                         otai_redis_otn.cpp \
                         otai_redis_ethernet.cpp \
                         otai_redis_physicalchannel.cpp \
                         otai_redis_och.cpp \
                         otai_redis_lldp.cpp \
                         otai_redis_assignment.cpp \
                         otai_redis_interface.cpp \
                         otai_redis_oa.cpp \
                         otai_redis_osc.cpp \
                         otai_redis_aps.cpp \
                         otai_redis_apsport.cpp \
                         otai_redis_attenuator.cpp \
                         otai_redis_wss.cpp \
                         otai_redis_mediachannel.cpp \
                         otai_redis_ocm.cpp \
                         otai_redis_otdr.cpp

libotaiRedis_a_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)

libotairedis_la_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
libotairedis_la_LIBADD = -lhiredis -lswsscommon libotaiRedis.a


