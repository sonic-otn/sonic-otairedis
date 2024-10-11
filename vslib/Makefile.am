AM_CPPFLAGS = $(OTAIINC) -I$(top_srcdir)/lib

lib_LTLIBRARIES = libotaivs.la

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
					  otai_vs_otdr.cpp \
					  Otai.cpp \
					  RealObjectIdManager.cpp \
					  OtaiObjectSimulator.cpp \
					  OtaiObjectNotificationSim.cpp


libotaivs_la_CXXFLAGS = $(DBGFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS_COMMON)
libotaivs_la_LIBADD = -lhiredis -lswsscommon

