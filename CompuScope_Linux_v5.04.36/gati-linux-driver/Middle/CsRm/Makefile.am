
include $(top_srcdir)/common_middle.mk

AM_CPPFLAGS += \
	-I$(srcdir)/Common \
	-DMAKE_DAEMON

bin_PROGRAMS = csrmd

# headers included in SOURCES are required for distcheck-target.

csrmd_SOURCES = \
	../../Common/CriticalSection.cpp \
	../../Common/RawSocketServerWorker.cpp \
	../../Common/ThreadDispatcher.cpp \
	../../Common/RawSocket.cpp \
	../../Common/misc.cpp \
	../../Common/xml.cpp \
	../../Common/Linux/CWinEventHandle.cpp \
	../../Common/Linux/WinEvent.cpp \
	./Common/CsResourceManager.cpp \
	./Common/ProcessList.cpp \
	./Common/RM.cpp \
	./Common/Worker.cpp \
	./Common/CsResourceManager.h \
	./Common/ProcessList.h \
	./Common/Worker.h

csrmd_LDADD = \
	../CsRmDLL/libCsRmDll.la \
	-lpthread \
	-ldl \
	-lrt


