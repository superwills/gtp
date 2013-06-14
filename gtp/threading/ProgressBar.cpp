#include "ProgressBar.h"
#include "../window/GTPWindow.h"

ProgressBar::ProgressBar( const char* iTxt, int iFontId, int ix, int iy, int width, int height ):
    fontId(iFontId),x(ix),y(iy),w(width),h(height),percDone(0.0)
{
  strcpy( txt, iTxt ) ;
  strcat( txt, " (not started)" ) ;
  color = ByteColor::random() ;
  color.a = 128 ;

  // get the y for this box
  {
    MutexLock LOCK( window->mutexPbs, INFINITE ) ;
    y = nextY() ;
    window->pbs.push_back( this ) ;
  }
}

int ProgressBar::nextY()
{
  if( !window->pbs.size() )
  {
    cy = marginY ; // there are no pbs, so you get the marginY as the start point.
  }
  else
  {
    // Must find max value to det lower limit..
    int biggestY = 0 ;
    foreach( list<ProgressBar*>::iterator, iter, window->pbs )
      if( (*iter)->y > biggestY ) biggestY = (*iter)->y ;

    int j;
    for( j = marginY ; j < biggestY ; j += defSpace )
    {
      bool jUsed = false ;

      // if NO element uses this j value, then it is a hole
      foreach( list<ProgressBar*>::iterator, iter, window->pbs )
      {
        if( (*iter)->x != cx )
          continue ; // this x is NOT in the current column, so we are not interested in this elt's y
        if( (*iter)->y == j )
        {
          jUsed = true ;
          break ; // proceed to next y-value to test.
        }
      }
      
      if( !jUsed )
      {
        cy = j ;
        return cy ; // no more checking if found a hole
      }
    }

    // If no hole was found, return 1 past the end
    cy = biggestY + defSpace ; // cy is 1 past the end

    // if the y passes window width, wrap, incr cx
    if( y >= window->getHeight() )
    {
      cx += defW + defSpace ;
      cy = defY ; // push cy back to the top
    }
  }
  return cy ;
}

ProgressBar::~ProgressBar()
{
  {
    MutexLock LOCK( window->mutexPbs, INFINITE ) ;
    window->pbs.remove( this ) ;
  }
}

// cx,cy are CURRENT x and y
float ProgressBar::cx = 15, ProgressBar::cy = 125,
ProgressBar::defSpace = 25,
// margins are the amount of space in pixels to leave on top of the things
ProgressBar::marginX = 15, ProgressBar::marginY = 210,
ProgressBar::defX = 15, ProgressBar::defY = 125,
ProgressBar::defW = 200, ProgressBar::defH = 20 ;
