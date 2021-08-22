
include $(top_srcdir)/common_driver.mk

AM_CPPFLAGS += \
	-I$(srcdir)/Common \
	-D_JDEERE_DRV_

lib_LTLIBRARIES = libCscdG12.la

# headers included in SOURCES are required for distcheck-target.

libCscdG12_la_SOURCES = \
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
	./Common/shcommon/CsPlxBase.cpp \
	./Common/shcommon/CsPlxFlash.cpp \
	./Common/shcommon/CsEvtThread.cpp \
	./Common/CsDeere.cpp \
	./Common/CsDeereDevice.cpp \
	./Common/CsDeereCalib.cpp \
	./Common/CsDeereConfig.cpp \
	./Common/CsDeereDataXfer.cpp \
	./Common/CsDeereEvtThread.cpp \
	./Common/CsDeereHw.cpp \
	./Common/CsDeereMisc.cpp \
	./Common/CsDeereSineFit.cpp \
	./Common/CsDeereCommit.cpp \
	./Common/CsDeereFlashObj.cpp \
	./Common/CsDeereMs.cpp \
	../../Bd_Include/DeereCardState.h \
	./Common/CsDeere.h \
	./Common/CsDeereSineFit.h \
	./Common/CsDeereFlashObj.h

libCscdG12_la_LIBADD = \
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
	./Common/shcommon/CsPlxBase.cpp \
	./Common/shcommon/CsPlxFlash.cpp \
	./Common/shcommon/CsEvtThread.cpp

$(BUILT_SOURCES):
	@[ ! -e ./Common/shcommon/CWinEventHandle.cpp ] && { \
		mkdir -p $(top_builddir)/Boards/UserSpace/CsJohnDeere/Common/shcommon; \
		cd $(top_builddir)/Boards/UserSpace/CsJohnDeere/Common/shcommon; \
		ln -s $(abs_top_srcdir)/Boards/UserSpace/US_Common/* .; \
		ln -s $(abs_top_srcdir)/Common/Linux/* .; \
		ln -s $(abs_top_srcdir)/Common/* .; \
	}

clean-local:
	-@rm -f ./Common/shcommon/*.lo ./Common/shcommon/*.o

distclean-local:
	-@rm -rf Common/shcommon ./Common/shcommon

