
include $(top_srcdir)/common_driver.mk

AM_CPPFLAGS += \
	-I$(srcdir)/Common \
	-D_HEXAGON_DRV_

lib_LTLIBRARIES = libCsE16bcd.la

# headers included in SOURCES are required for distcheck-target.

libCsE16bcd_la_SOURCES = \
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
	./Common/CsHexagon.cpp \
	./Common/CsHexagonDev.cpp \
	./Common/CsHexagonCalib.cpp \
	./Common/CsHexagonConfig.cpp \
	./Common/CsHexagonDataXfer.cpp \
	./Common/CsHexagonEvtThread.cpp \
	./Common/CsHexagonFlash.cpp \
	./Common/CsHexagonHw.cpp \
	./Common/CsHexagonMisc.cpp \
	./Common/CsHexagonDbg.cpp \
	../../Bd_Include/CsCardState.h \
	../../Bd_Include/CsDriverTypes.h \
	../../Bd_Include/CsDrvApi.h \
	../../Bd_Include/CsDrvDefines.h \
	../../Bd_Include/CsIoctl.h \
	../../Bd_Include/CsIoctlTypes.h \
	../../Bd_Include/CsPlxDefines.h \
	../../Bd_Include/CsLinuxSignals.h \
	../../Bd_Include/CsHexagonState.h \
	../../Bd_Include/CsSymLinks.h \
	../../Bd_Include/CsHexagonRegs.h \
	./Common/CsHexagon.h \
	../US_Include/CsAdvHwDLL.h \
	../US_Include/CsBaseHwDLL.h \
	../US_Include/CsDeviceApi.h \
	../US_Include/CsDevIoCtl.h \
	../US_Include/CsNiosApi.h \
	../US_Include/CsPlxBase.h \
	../US_Include/CurrentDriver.h \
	../US_Include/DllStruct.h \
	../US_Include/GageWrapper.h \
	../US_Include/PeevAdaptor.h \
	../US_Include/PlxDrvConsts.h

libCsE16bcd_la_LIBADD = \
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
		mkdir -p $(top_builddir)/Boards/UserSpace/CsHexagon/Common/shcommon; \
		cd $(top_builddir)/Boards/UserSpace/CsHexagon/Common/shcommon; \
		ln -s $(abs_top_srcdir)/Boards/UserSpace/US_Common/* .; \
		ln -s $(abs_top_srcdir)/Common/Linux/* .; \
		ln -s $(abs_top_srcdir)/Common/* .; \
	}

clean-local:
	-@rm -f ./Common/shcommon/*.lo ./Common/shcommon/*.o

distclean-local:
	-@rm -rf Common/shcommon ./Common/shcommon

