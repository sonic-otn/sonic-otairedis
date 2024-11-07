AM_CXXFLAGS = $(OTAIINC)

lib_LTLIBRARIES = libotairedis.la

noinst_LIBRARIES = libOtaiRedis.a

libOtaiRedis_a_SOURCES = \
						 Channel.cpp \
						 Context.cpp \
						 ContextConfigContainer.cpp \
						 ContextConfig.cpp \
						 LinecardConfig.cpp \
						 LinecardConfigContainer.cpp \
						 RedisChannel.cpp \
						 Otai.cpp \
						 Linecard.cpp \
						 LinecardContainer.cpp \
						 VirtualObjectIdManager.cpp \
						 RedisVidIndexGenerator.cpp \
						 RedisRemoteOtaiInterface.cpp \
						 Utils.cpp 

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

libOtaiRedis_a_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON)

libotairedis_la_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON)
libotairedis_la_LIBADD = -lhiredis -lswsscommon libOtaiRedis.a


