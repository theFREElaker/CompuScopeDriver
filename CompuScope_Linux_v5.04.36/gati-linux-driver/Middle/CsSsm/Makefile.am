
include $(top_srcdir)/common_middle.mk

AM_CPPFLAGS += \
	-I$(srcdir)/Common \
	-I$(srcdir)/Common/Queue \
	-I$(srcdir)/Common/SigFiles \
	-I$(srcdir)/Linux \
	-I$(abs_top_srcdir)/Middle/CsSsm/Common \
	-I$(abs_top_srcdir)/Middle/CsSsm/Linux \
	-I$(abs_top_srcdir)/Middle/CsSsm/Common/Queue \
	-I$(abs_top_srcdir)/Sdk/CsAppSupport \
	-I$(abs_top_builddir)/Middle/CsSsm/Common

lib_LTLIBRARIES = libCsSsm.la

# headers included in SOURCES are required for distcheck-target.

libCsSsm_la_SOURCES =  \
	./Common/SigFiles/CsSigFiles.cpp \
	./Common/Queue/Worker.cpp \
	./Common/CsSSMStateMachine_sm.cpp \
	./Common/CsSsm.cpp \
	./Common/CsSsmConfig.cpp \
	./Common/CsSsmInitialization.cpp \
	./Common/CsSsmOps.cpp \
	./Common/CsSsmStateMachine.cpp \
	./Common/CsSsmStatus.cpp \
	./Common/CsSsmXfer.cpp \
	./Common/EventThread.cpp \
	./Common/SingletonData.cpp \
	./Common/SingletonData.h \
	../../Common/Linux/CWinEventHandle.cpp \
	../../Common/Linux/WinEvent.cpp \
	../../Common/misc.cpp \
	./Linux/CsErrorStruct.h \
	./Common/Queue/Functor.h \
	./Common/Queue/Reference.h \
	./Common/Queue/SimpleReferencer.h \
	./Common/Queue/DeferCall.h \
	./Common/Queue/Worker.h \
	./Common/Queue/FreeFunctor.h \
	./Common/Queue/MemberFunctor.h \
	./Common/CsSsmStateMachine.h \
	./Common/CsSsm.h \
	../../Sdk/CsAppSupport/CsAppSupport.h

libCsSsm_la_LIBADD = \
	../CsDisp/libCsDisp.la \
	../CsRmDLL/libCsRmDll.la \
	../CsFs/libCsFs.la \
	-ldl \
	-lpthread \
	-lrt

# mkdir, EXTRA_DIST and DISTCLEANFILES are needed by distcheck-target.

BUILT_SOURCES = \
	./Common/CsSSMStateMachine_sm.cpp \
	./Common/CsSSMStateMachine_sm.h \
	./Common/CsSSMStateMachine.h

$(BUILT_SOURCES): ./Common/Smc.jar ./Common/CsSSMStateMachine.sm
	@mkdir -p $(top_builddir)/Middle/CsSsm/Common;
	@cd $(top_builddir)/Middle/CsSsm/Common;java -jar $(abs_top_srcdir)/Middle/CsSsm/Common/Smc.jar -c++ -g $(abs_top_srcdir)/Middle/CsSsm/Common/CsSSMStateMachine.sm

EXTRA_DIST = \
	./Common/Smc.jar \
	./Common/CsSSMStateMachine.sm

# CLEANFILES =

DISTCLEANFILES = \
	./Common/CsSSMStateMachine_sm.cpp \
	./Common/CsSSMStateMachine_sm.h
