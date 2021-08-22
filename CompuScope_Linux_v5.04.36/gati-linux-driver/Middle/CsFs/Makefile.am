
include $(top_srcdir)/common_middle.mk

AM_CPPFLAGS += -I$(srcdir)/Common

lib_LTLIBRARIES = libCsFs.la

# headers included in SOURCES are required for distcheck-target.

libCsFs_la_SOURCES = \
	../../Common/Structs.c \
	../../Common/misc.cpp \
	./Common/CsFs.cpp \
	./Common/SigUtils.cpp \
	./Common/SigUtils.h

