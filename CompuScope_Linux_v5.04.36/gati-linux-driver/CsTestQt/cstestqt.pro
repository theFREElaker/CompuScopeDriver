#-------------------------------------------------
#
# Project created by QtCreator 2016-01-14T11:51:38
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 warn_on
TARGET = cstestqt
TEMPLATE = app
INCLUDEPATH += $$PWD/include

DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT

win32 {
   GAGEPATH_ = $$(CombineWDF)
   BUILDARCH_ = x86
   BINARCH_ = bin
   BUILDTYPE_ = release
   contains (QT_ARCH, x86_64) {
      DEFINES += BUILD64
      BUILDARCH_ = x64
      BINARCH_ = bin64
   }
   CONFIG (debug, debug|release) {
      #  CONFIG += force_debug_info
      #  TARGET = GsExpressd
      BUILDTYPE_ = debug
   }
#  resource.rc2 is not part of the project, thus will not see INCLUDEPATH;
#  we need to define an explicit path for the include.
   DEFINES += CSRCINFOH=\\\"\"$$GAGEPATH_\include\Private\csrcinfo.h\"\\\"
   INCLUDEPATH += "$$GAGEPATH_/include/public" "$$GAGEPATH_/include/private" "$$PWD/lib/win"
   LIBS += -L"$$GAGEPATH_/$$BINARCH_/lib/public" -lcsssm
   LIBS += -L"$$GAGEPATH_/$$BINARCH_/lib/private" -lCsDisp
   LIBS += -L"$$GAGEPATH_/$$BINARCH_/lib/private" -lCsRmDll
#   VERSION_ = $$system(getver.bat)
   RC_FILE = CsTestQt.rc2
}

unix {
   GAGEPATH_ = $$(CombineWdf)
   BUILDARCH_ = x64
#   INCLUDEPATH += "$$GAGEPATH_/Include/Public" "$$GAGEPATH_/Include/Private" "$$GAGEPATH_/Include/Linux"
#   INCLUDEPATH += -I$$GAGEPATH_/Include/Public -I$$GAGEPATH_/Include/Private -I$$GAGEPATH_/Include/Linux
   INCLUDEPATH += ../Include/Public ../Include/Private ../Include/Linux
   LIBS += -L/usr/local/lib -lCsSsm -lCsDisp -lCsRmDll
   RC_FILE = CsTestQt.rc2
}

SOURCES += main.cpp\
        cstestqt.cpp \
    CsTestErrMng.cpp \
    acquisitionworker.cpp \
    AsyncCsOps.cpp \
    CsTestFunc.cpp \
    DrawableChannel.cpp \
    GageDispSystem.cpp \
    GageSsmSystem.cpp \
    GageSystem.cpp \
    qconfigdialog.cpp \
    qcsgraph.cpp \
    qCursorInfodialog.cpp \
    qdebugdialog.cpp \
    qMulrecdialog.cpp \
    qNiosSpiCmddialog.cpp \
    qperformancetestdialog.cpp \
    qReadDataFiledialog.cpp \
    qReadWriteRegsdialog.cpp \
    qscope.cpp \
    qsystemselectiondialog.cpp \
    SystemCaps.cpp \
    qaboutdialog.cpp \
    csSaveFile.cpp \
    qReadSigFiledialog.cpp \
    qProgressBar.cpp \
    qGoToSample.cpp

HEADERS  += cstestqt.h \
    CsTestErrMng.h \
    acquisitionworker.h \
    AsyncCsOps.h \
    CsTestFunc.h \
    DrawableChannel.h \
    GageDispSystem.h \
    GageSsmSystem.h \
    GageSystem.h \
    qaboutdialog.h \
    qconfigdialog.h \
    qcsgraph.h \
    qCursorInfodialog.h \
    qdebugdialog.h \
    qMulrecdialog.h \
    qNiosSpiCmddialog.h \
    qperformancetestdialog.h \
    qReadDataFiledialog.h \
    qReadWriteRegsdialog.h \
    qscope.h \
    qsystemselectiondialog.h \
    SystemCaps.h \
    csSaveFile.h \
    cstestqttypes.h \
    qReadSigFiledialog.h \
    qProgressBar.h \
    qGoToSample.h

FORMS    += cstestqt.ui \
    qaboutdialog.ui \
    qaboutdialog2.ui \
    qconfigdialog.ui \
    qcontrolsdialog.ui \
    qdebugdialog.ui \
    qMulrecdialog.ui \
    qNiosSpiCmddialog.ui \
    qperformancetestdialog.ui \
    qReadDataFiledialog.ui \
    qReadWriteRegsdialog.ui \
    qscope.ui \
    qsystemselectiondialog.ui \
    qCursorInfodialog.ui \
    qReadSigFiledialog.ui \
    qprogressbar.ui \
    qGotoSample.ui
