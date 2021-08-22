
include $(top_srcdir)/common_driver.mk

AM_CPPFLAGS += -I$(srcdir)/Common -std=gnu++0x

lib_LTLIBRARIES = libCsxyG8.la libCsEcdG8.la

# headers included in SOURCES are required for distcheck-target.

libCsxyG8_la_SOURCES = \
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
	./Common/shcommon/PeevAdaptor.cpp \
	./Common/CsRabbit.cpp \
	./Common/CsRabbitDev.cpp \
	./Common/CsRabbitCalib.cpp \
	./Common/CsRabbitConfig.cpp \
	./Common/CsRabbitDataXfer.cpp \
	./Common/CsRabbitEvtThread.cpp \
	./Common/CsRabbitFlash.cpp \
	./Common/CsRabbitHw.cpp \
	./Common/CsRabbitMisc.cpp \
	../../Bd_Include/CsCardState.h \
	../../Bd_Include/CsDriverTypes.h \
	../../Bd_Include/CsDrvApi.h \
	../../Bd_Include/CsDrvDefines.h \
	../../Bd_Include/CsIoctl.h \
	../../Bd_Include/CsIoctlTypes.h \
	../../Bd_Include/CsPlxDefines.h \
	../../Bd_Include/CsLinuxSignals.h \
	../../Bd_Include/CsRabbitState.h \
	../../Bd_Include/CsSymLinks.h \
	../../UserSpace/CsRabbit/Common/CsRabbit.h \
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
	../US_Include/PlxDrvConsts.h \
	../US_Include/GageCfgParser.h

   
libCsxyG8_la_LIBADD = \
	-lpthread \
	-lrt

libCsxyG8_la_CXXFLAGS = -D_RABBIT_DRV_

libCsEcdG8_la_SOURCES = $(libCsxyG8_la_SOURCES)

libCsEcdG8_la_LIBADD = $(libCsxyG8_la_LIBADD)

libCsEcdG8_la_CXXFLAGS = -D_COBRAMAXPCIE_DRV_

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
		mkdir -p $(top_builddir)/Boards/UserSpace/CsRabbit/Common/shcommon; \
		cd $(top_builddir)/Boards/UserSpace/CsRabbit/Common/shcommon; \
		ln -s $(abs_top_srcdir)/Boards/UserSpace/US_Common/* .; \
		ln -s $(abs_top_srcdir)/Common/Linux/* .; \
		ln -s $(abs_top_srcdir)/Common/* .; \
	}

clean-local:
	-@rm -f ./Common/shcommon/*.lo ./Common/shcommon/*.o

distclean-local:
	-@rm -rf Common/shcommon ./Common/shcommon

