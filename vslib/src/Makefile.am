AM_CPPFLAGS = -I$(top_srcdir)/vslib/inc -I$(top_srcdir)/LAI/inc -I$(top_srcdir)/LAI/meta -I/usr/include/libnl3 -I$(top_srcdir)/lib/inc

if DEBUG
DBGFLAGS = -ggdb -D_DEBUG_
else
DBGFLAGS = -g
endif

lib_LTLIBRARIES = liblaivs.la

noinst_LIBRARIES = libLaiVS.a
libLaiVS_a_SOURCES = \
					  ../../lib/src/Notification.cpp \
					  ../../lib/src/PerformanceIntervalTimer.cpp \
					  ../../lib/src/NotificationLinecardStateChange.cpp \
					  ResourceLimiter.cpp \
					  ResourceLimiterContainer.cpp \
					  ResourceLimiterParser.cpp \
					  EventPayloadNotification.cpp \
					  EventPayloadNetLinkMsg.cpp \
					  LaiEventQueue.cpp \
					  Event.cpp \
					  EventQueue.cpp \
					  Signal.cpp \
					  Buffer.cpp \
					  EventPayloadPacket.cpp \
					  VirtualLinecardLaiInterface.cpp \
					  LinecardConfigContainer.cpp \
					  LinecardConfig.cpp \
					  LaiUnittests.cpp \
                                          LaiCheckLink.cpp \
					  Lai.cpp \
					  LinecardStateBase.cpp \
					  NetMsgRegistrar.cpp \
					  LaneMap.cpp \
					  LaneMapContainer.cpp \
					  LaneMapFileParser.cpp \
					  Linecard.cpp \
					  LinecardContainer.cpp \
					  RealObjectIdManager.cpp \
					  LaiAttrWrap.cpp \
					  SelectableFd.cpp \
					  LinecardState.cpp \
					  LinecardP230C.cpp \
					  CorePortIndexMap.cpp \
					  CorePortIndexMapContainer.cpp \
					  CorePortIndexMapFileParser.cpp

liblaivs_la_SOURCES = \
					  lai_vs_linecard.cpp \
					  lai_vs_interfacequery.cpp \
					  lai_vs_port.cpp \
					  lai_vs_transceiver.cpp \
					  lai_vs_logicalchannel.cpp \
					  lai_vs_otn.cpp \
					  lai_vs_ethernet.cpp \
					  lai_vs_physicalchannel.cpp \
					  lai_vs_och.cpp \
					  lai_vs_lldp.cpp \
					  lai_vs_assignment.cpp \
					  lai_vs_interface.cpp \
					  lai_vs_oa.cpp \
					  lai_vs_osc.cpp \
					  lai_vs_aps.cpp \
					  lai_vs_apsport.cpp \
					  lai_vs_attenuator.cpp \
					  lai_vs_wss.cpp \
					  lai_vs_mediachannel.cpp \
					  lai_vs_ocm.cpp \
					  lai_vs_otdr.cpp

libLaiVS_a_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)

liblaivs_la_CPPFLAGS = $(DBGFLAGS) $(AM_CPPFLAGS) $(CFLAGS_COMMON)
liblaivs_la_LIBADD = -lhiredis -lswsscommon libLaiVS.a

