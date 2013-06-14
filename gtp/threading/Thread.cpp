#include "Thread.h"

bool Thread::allThreadsRunning = true ;

Thread::Thread()
{
  // it starts suspended
  status = Suspended ;

  handle = CreateThread( 0, 0, 
    (LPTHREAD_START_ROUTINE)threadIdle,
    0, // not using a data parameter
    CREATE_SUSPENDED,
    &threadId
  ) ;

  if( !handle )   // Then thread creation failed
    printWindowsLastError( "Thread creation failed" );
  else
  {
    #if THREAD_VERBOSE
    info( "Thread %d has started thread id=%d", GetCurrentThreadId(), threadId ) ;
    #endif
  }
}

Thread::~Thread()
{
  // put it to sleep
  sleep() ;

  // use its handle to kill it while it is sleeping
  TerminateThread( handle, 0 ) ;
}

// What we'll do is START (n-1) threads at program start.
// We'll spin off each thread to be contained in the function below
// This WOULD BE a member function of ThreadExecData,
// but __thiscall cannot be used in CreateThread.
DWORD Thread::threadIdle( LPVOID execData )
{
  while( allThreadsRunning )
  {
    // Check if there are any available jobs.
    Job *myJob = threadPool.getNextJob() ; // it'll be null if
    // there are no jobs to do. if there are,
    // then that job will be dequeued from threadPool.queuedJobs
    // and added to threadPool.activeJobs (where it will stay
    // until this thread COMPLETES executing "myJob->start()",
    // which could take anywhere from 0.0001 ms to hours/days.

    if( !myJob )
    {
      // I don't have a job to do, and I just checked the queue.
      // I will sleep
      DWORD myId = GetCurrentThreadId() ;

      // I cannot get the thread object that represents me
      // without using my current running thread id.
      Thread * me = threadPool.getThreadById( myId ) ;

      // so it can happen (when thread first created)
      // that we don't find "me" in the queue, AS the object is created,
      // the thread runs off and executes this function
      if( me )
      {
        #if THREAD_VERBOSE
        info( Magenta, "Thread %d hasn't anything to do, going to sleep", me->threadId ) ;
        #endif
        
        me->sleep() ;

        // WHEN I'm awakened, I will start here,
        #if THREAD_VERBOSE
        info( Magenta, "Thread %d awakened from sleep", myId ) ;
        #endif
      }
      else
        warning( "Thread could not find itself in list, suspend failed" ) ;
    }

    // I have a job to do.
    else //if( myJob != NULL )
    {
      myJob->start() ; // do it!!

      // When the job finishes, control
      // will return here.
      myJob->done() ;

      if( TALKATIVE )
        info( Blue, "Job '%s' DONE by thread %d", myJob->name, GetCurrentThreadId() ) ;

      // Remove the job from the ACTIVE/assigned queue,
      // and into the DONE queue, as it is DONE.

      // to touch the queues you must acquire the lock though.
      

      threadPool.jobDone( myJob ) ;

      
    }
  }

  return 0 ; // thread terminates
}

Thread::Status Thread::getStatus()
{
  return status ;
}

// If this doesn't work well,
// you can try sleep/awaken using
// mutex locks and WaitForSingleObject()
void Thread::sleep()
{
  #if THREAD_VERBOSE
  info( "Thread %d has gone to sleep", threadId ) ;
  #endif
  status = Suspended ;
  SuspendThread( handle ) ; // Suspend myself
}

void Thread::wakeUp()
{
  #if THREAD_VERBOSE
  info( Yellow, "Thread %d is being woken up by %d", threadId, GetCurrentThreadId() ) ;
  #endif
  status = Running ;
  ResumeThread( handle ) ;
}