
include $(top_srcdir)/common_driver.mk

AM_CPPFLAGS += \
	-I$(srcdir)/Common \
	-D_SPIDER_DRV_

lib_LTLIBRARIES = libCs8xxx.la

# headers included in SOURCES are required for distcheck-target.

libCs8xxx_la_SOURCES = \
	./Common/shcommon/CWinEventHandle.cpp \
	./Common/shcommon/WinEvent.cpp \
	./Common/shcommon/misc.cpp \
	./Common/shcommon/CsEventLog.cpp \
	./Common/shcommon/CsBaseHwDLL.cpp \
	./Common/shcommon/CsBoilerPlateDll.cpp \
	./Common/shcommon/CsDeviceApi.cpp \
	./Common/shcommon/CsDevIoCtl.cpp \
	./Common/shcommon/CsNiosApi.cpp \
	./Common/shcommon/GageWrapper.cpp \
	./Common/shcommon/GageCfgParser.cpp \
	./Common/shcommon/CsPlxBase.cpp \
	./Common/shcommon/CsPlxFlash.cpp \
	./Common/shcommon/CsEvtThread.cpp \
	./Common/CsSpider.cpp \
	./Common/PeevAdaptor.cpp \
	./Common/CsSpiderDev.cpp \
	./Common/CsSpiderCalib.cpp \
	./Common/CsSpiderConfig.cpp \
	./Common/CsSpiderDataXfer.cpp \
	./Common/CsSpiderEvtThread.cpp \
	./Common/CsSpiderFlash.cpp \
	./Common/CsSpiderHw.cpp \
	./Common/CsSpiderMisc.cpp \
	./Common/CsSpiderMs.cpp \
	../../Bd_Include/CsLinuxSignals.h \
	../../Bd_Include/CsSpiderState.h \
	./Common/CsSpider.h \
	./Common/Cs8xxxHwCaps.h \
	./Common/PeevAdaptor.h

libCs8xxx_la_LIBADD = \
	-lpthread \
	-lrt

# subdir-objects creates dependency issue with the way libraries are created here,
# and BUILT_SOURCES provides a partial fix (will not work if the libraries are
# not dlopen-ed).

BUILT_SOURCES = \
	./Common/shcommon/CWinEventHandle.cpp \
	./Common/shcommon/WinEvent.cpp \
	./Common/shcommon/misc.cpp \
	./Common/shcommon/CsEventLog.cpp \
	./Common/shcommon/CsBaseHwDLL.cpp \
	./Common/shcommon/CsBoilerPlateDll.cpp \
	./Common/shcommon/CsDeviceApi.cpp \
	./Common/shcommon/CsDevIoCtl.cpp \
	./Common/shcommon/CsNiosApi.cpp \
	./Common/shcommon/GageWrapper.cpp \
	./Common/shcommon/GageCfgParser.cpp \
	./Common/shcommon/CsPlxBase.cpp \
	./Common/shcommon/CsPlxFlash.cpp \
	./Common/shcommon/CsEvtThread.cpp

$(BUILT_SOURCES):
	@[ ! -e ./Common/shcommon/CWinEventHandle.cpp ] && { \
		mkdir -p $(top_builddir)/Boards/UserSpace/CsSpider/Common/shcommon; \
		cd $(top_builddir)/Boards/UserSpace/CsSpider/Common/shcommon; \
		ln -s $(abs_top_srcdir)/Boards/UserSpace/US_Common/* .; \
		ln -s $(abs_top_srcdir)/Common/Linux/* .; \
		ln -s $(abs_top_srcdir)/Common/* .; \
	}

clean-local:
	-@rm -f ./Common/shcommon/*.lo ./Common/shcommon/*.o

distclean-local:
	-@rm -rf Common/shcommon ./Common/shcommon
