#ifndef HIGHPERFTIMER_H
#define HIGHPERFTIMER_H

#include <windows.h>
#include <stdio.h>

// FPS counter, also acts as the wall-clock.
class FPSCounter
{
  LARGE_INTEGER freq, thisTime, lastTime ;
  double fFreq ;
  int f ;
  const static int n=50 ;
  double last[ n ] ; 

public:
  double total_time ;
  double fps ;

  FPSCounter()
  {
    QueryPerformanceFrequency( &freq ) ;
    fFreq = (double)freq.QuadPart ;
    start();
    f=0;
    total_time = 0.0 ;
    //printf( "     --- The ffreq is %lf\n", fFreq ) ;
  }

  void start()
  {
    QueryPerformanceCounter( &thisTime ) ;
    lastTime = thisTime ;
  }

  void update()
  {
    lastTime = thisTime ;
    QueryPerformanceCounter( &thisTime ) ;

    // The amount of time that passed since last call.
    double secs = (thisTime.QuadPart - lastTime.QuadPart)/fFreq ;
    f= ++f % n ;
    last[ f ] = 1.0/secs ;
    total_time += secs ;

    fps = 0;
    for( int i = 0 ; i < n ; i++ )
      fps += last[ i ] ;
    fps /= n ;
  }

} ;


class Timer
{
  LARGE_INTEGER startTime ;
  double fFreq ;

public:
  Timer() {
    LARGE_INTEGER freq ;
    QueryPerformanceFrequency( &freq ) ;
    fFreq = (double)freq.QuadPart ;
    reset();
  }

  void reset() {   QueryPerformanceCounter( &startTime ) ;  }

  // Gets the most up to date time.
  double getTime() const {
    LARGE_INTEGER endTime ;
    QueryPerformanceCounter( &endTime ) ;
    return ( endTime.QuadPart - startTime.QuadPart ) / fFreq ; // as double
  }
} ;

#endif //HIGHPERFTIMER_H