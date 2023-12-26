#ifndef THREAD_H
#define THREAD_H

#include <Windows.h>
#include "Job.h"
#include "../util/StdWilUtil.h"
#include "ThreadPool.h"

#define THREAD_VERBOSE 0

struct Thread
{
  enum Status{ Running, Suspended } ;

  static bool allThreadsRunning ;
  
// these should be private but rsi shortcut
  HANDLE handle ;  // the thread handle, for waking up and sleeping the thread.
  DWORD threadId ; // the thread identifier (returned by the CreateThread call)

private:
  Status status ;

public:
  Thread() ;
  ~Thread() ;

  // What we'll do is START (n-1) threads at program start.
  // We'll spin off each thread to be contained in the function below
  // This WOULD BE a member function of ThreadExecData,
  // but __thiscall cannot be used in CreateThread.
  static DWORD threadIdle( LPVOID execData ) ;

  Status getStatus() ;

  // If this doesn't work well,
  // you can try sleep/awaken using
  // mutex locks and WaitForSingleObject()
  void sleep() ;

  void wakeUp() ;
} ;

#endif