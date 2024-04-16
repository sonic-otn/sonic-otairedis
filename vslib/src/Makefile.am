AM_CPPFLAGS = -I$(top_srcdir)/vslib/inc -I$(top_srcdir)/OTAI/inc -I$(top_srcdir)/OTAI/meta -I/usr/include/libnl3 -I$(top_srcdir)/lib/inc

if DEBUG
DBGFLAGS = -ggdb -D_DEBUG_
else
DBGFLAGS = -g
endif

lib_LTLIBRARIES = libotaivs.la

noinst_LIBRARIES = libotaiVS.a
libotaiVS_a_SOURCES = \
					  ../../lib/src/Notification.cpp \
					  ../../lib/src/PerformanceIntervalTimer.cpp \
					  ../../lib/src/NotificationLinecardStateChange.cpp \
					  ResourceLimiter.cpp \
					  ResourceLimiterContainer.cpp \
					  ResourceLimiterParser.cpp \
					  EventPayloadNotification.cpp \
					  EventPayloadNetLinkMsg.cpp \
					  OtaiEventQueue.cpp \
					  Event.cpp \
					  EventQueue.cpp \
					  Signal.cpp \
					  Buffer.cpp \
					  EventPayloadPacket.cpp \
					  VirtualLinecardOtaiInterface.cpp \
					  LinecardConfigContainer.cpp \
					  LinecardConfig.cpp \
					  OtaiUnittests.cpp \
                                          OtaiCheckLink.cpp \
					  Otai.cpp \
					  LinecardStateBase.cpp \
					  NetMsgRegistrar.cpp \
					  LaneMap.cpp \
					  LaneMapContainer.cpp \
					  LaneMapFileParser.cpp \
					  Linecard.cpp \
					  LinecardContainer.cpp \
					  RealObjectIdManager.cpp \
					  OtaiAttrWrap.cpp \
					  SelectableFd.cpp \
					  LinecardState.cpp \
					  LinecardP230C.cpp \
					  CorePortIndexMap.cpp \
					  CorePortIndexMapContainer.cpp \
					  CorePortIndexMapFileParser.cpp

libotaivs_la_SOURCES = \
					  otai_vs_linecard.cpp \
					  otai_vs_interfacequery.cpp \
					  otai_vs_port.cpp \
					  otai_vs_transceiver.cpp \
					  otai_vs_logicalchannel.cpp \
					  otai_vs_otn.cpp \
					  otai_vs_ethernet.cpp \
					  otai_vs_physicalchannel.cpp \
					  otai_vs_och.cpp \
					  otai_vs_lldp.cpp \
					  otai_vs_assignment.cpp \
					  otai_vs_interface.cpp \
					  otai_vs_oa.cpp \
					  otai_vs_osc.cpp \
					  otai_vs_aps.cpp \
					  otai_vs_apsport.cpp \
					  otai_vs_attenuator.cpp \
					  otai_vs_wss.cpp \
					  otai_vs_mediachannel.cpp \
					  otai_vs_ocm.cpp \
					  otai_vs_otdr.cpp

libotaiVS_a_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)

libotaivs_la_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
libotaivs_la_LIBADD = -lhiredis -lswsscommon libotaiVS.a

