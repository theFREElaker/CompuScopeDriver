
include $(top_srcdir)/common_driver.mk

AM_CPPFLAGS += \
	-I$(srcdir)/Common \
	-D_SPLENDA_DRV_

lib_LTLIBRARIES = libCs16xyy.la

# headers included in SOURCES are required for distcheck-target.

libCs16xyy_la_SOURCES =	\
	./Common/shcommon/CWinEventHandle.cpp \
	./Common/shcommon/WinEvent.cpp \
	./Common/shcommon/xml.cpp \
	./Common/shcommon/misc.cpp \
	./Common/shcommon/CsEventLog.cpp \
	./Common/shcommon/CsBaseHwDLL.cpp \
	./Common/shcommon/CsBoilerPlateDll.cpp \
	./Common/shcommon/CsDevIoCtl.cpp \
	./Common/shcommon/CsDeviceApi.cpp \
	./Common/shcommon/GageWrapper.cpp \
	./Common/shcommon/GageCfgParser.cpp \
	./Common/shcommon/CsNiosApi.cpp \
	./Common/shcommon/CsPlxBase.cpp \
	./Common/shcommon/CsPlxFlash.cpp \
	./Common/shcommon/CsEvtThread.cpp \
	./Common/CsSplenda.cpp \
	./Common/CsSplendaDev.cpp \
	./Common/CsSplendaCalib.cpp \
	./Common/CsSplendaConfig.cpp \
	./Common/CsSplendaDataXfer.cpp \
	./Common/CsSplendaEvtThread.cpp \
	./Common/CsSplendaFlash.cpp \
	./Common/CsSplendaHw.cpp \
	./Common/CsSplendaMisc.cpp \
	../../Bd_Include/CsSplendaState.h \
	./Common/CsSplenda.h \
	./Common/CsSplendaHwCaps.h

libCs16xyy_la_LIBADD = \
	-lpthread \
	-lrt

# subdir-objects creates dependency issue with the way libraries are created here,
# and BUILT_SOURCES provides a partial fix (will not work if the libraries are
# not dlopen-ed).

BUILT_SOURCES = \
	./Common/shcommon/CWinEventHandle.cpp \
	./Common/shcommon/WinEvent.cpp \
	./Common/shcommon/xml.cpp \
	./Common/shcommon/misc.cpp \
	./Common/shcommon/CsEventLog.cpp \
	./Common/shcommon/CsBaseHwDLL.cpp \
	./Common/shcommon/CsBoilerPlateDll.cpp \
	./Common/shcommon/CsDevIoCtl.cpp \
	./Common/shcommon/CsDeviceApi.cpp \
	./Common/shcommon/GageWrapper.cpp \
	./Common/shcommon/GageCfgParser.cpp \
	./Common/shcommon/CsNiosApi.cpp \
	./Common/shcommon/CsPlxBase.cpp \
	./Common/shcommon/CsPlxFlash.cpp \
	./Common/shcommon/CsEvtThread.cpp

$(BUILT_SOURCES):
	@[ ! -e ./Common/shcommon/CWinEventHandle.cpp ] && { \
		mkdir -p $(top_builddir)/Boards/UserSpace/CsSplenda/Common/shcommon; \
		cd $(top_builddir)/Boards/UserSpace/CsSplenda/Common/shcommon; \
		ln -s $(abs_top_srcdir)/Boards/UserSpace/US_Common/* .; \
		ln -s $(abs_top_srcdir)/Common/Linux/* .; \
		ln -s $(abs_top_srcdir)/Common/* .; \
	}

clean-local:
	-@rm -f ./Common/shcommon/*.lo ./Common/shcommon/*.o

distclean-local:
	-@rm -rf Common/shcommon ./Common/shcommon

