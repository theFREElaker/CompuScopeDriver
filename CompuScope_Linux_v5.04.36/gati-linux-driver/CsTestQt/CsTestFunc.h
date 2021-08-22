#ifndef CSTESTFUNC_H
#define CSTESTFUNC_H

#include <QtGlobal>
#include <QString>
#include <QTime>
#include "qapplication.h"
#include "qelapsedtimer.h"
#include "math.h"
#ifndef Q_OS_WIN
    #include "unistd.h"
#endif

#ifdef Q_OS_WIN
    #define NOMINMAX	// to prevent compilation errors from QDateTime because of including windows.h
    #include <Windows.h> // Used for QTest::qSleep
#endif

#include "CsPrototypes.h"
#include "CsTypes.h"

namespace CsTestFunc
{
    inline void qSleep(uInt32 u32ms)
    {
        if(u32ms == 0)
            return;

        #ifdef Q_OS_WIN
            Sleep(u32ms);
        #else
            struct timespec ts = { (long long)(u32ms / 1000), (long long)((u32ms % 1000) * 1000 * 1000) };
            nanosleep(&ts, NULL);
        #endif
    }

    inline void quSleep(uInt32 u32usec)
    {
        if(u32usec == 0)
            return;

        #ifdef Q_OS_WIN
            HANDLE timer;
            LARGE_INTEGER ft;

            ft.QuadPart = -(10 * (int32)u32usec); // Convert to 100 nanosecond interval, negative value indicates relative time

            timer = CreateWaitableTimer(NULL, TRUE, NULL);
            SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
            WaitForSingleObject(timer, INFINITE);
            CloseHandle(timer);
        #else
            //struct timespec ts = { (long long)(u32nsec / 1000), (long long)((u32nsec % 1000) * 1000 * 1000) };
            //nanosleep(&ts, NULL);
            usleep(u32usec);
        #endif
    }

    inline void qDelay( int usecstoWait )
    {
        double NsecElapsed;
        unsigned long long ElapsedTimeUsec = 0;
        QElapsedTimer TimeMng;
        TimeMng.restart();

        while( ElapsedTimeUsec < (unsigned long long)usecstoWait )
        {
            NsecElapsed = TimeMng.nsecsElapsed();
            ElapsedTimeUsec = (unsigned long long)(floor((NsecElapsed / 1000) + 0.5));
        }
    }
}

#endif
