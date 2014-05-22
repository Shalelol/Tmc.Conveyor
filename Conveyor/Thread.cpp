/*
 *
 * Copyright © Mohammad Kaisar Ul Haque
 * http://kaisar-haque.blogspot.com/2007/07/c-nice-thread-class_23.html
 *
*/

/*
 * Thread Creation library
 */

#ifndef _THREADING_CLASS_
#define _THREADING_CLASS_

#include <Windows.h>
#include <process.h>

class Thread
{
public:
 Thread()
 {
    hThread = 0;
 }
 void start()
 {
    _threadObj = this;
    DWORD threadID;
    hThread = CreateThread(0, 0, threadProc, _threadObj, 0, &threadID);
 }

 void stop()
 {
    if (hThread)
     TerminateThread (hThread, 0);
 }

 virtual void run() = 0;

protected:
 static unsigned long __stdcall threadProc(void* ptr)
 {
    ((Thread*)ptr)->run();
    return 0;
 }

 Thread *_threadObj;
 HANDLE    hThread;
};

#endif