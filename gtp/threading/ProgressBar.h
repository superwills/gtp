#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "../math/ByteColor.h"

struct ProgressBar
{
  float x,y,w,h,percDone;
  static float cx,cy,defSpace,marginX,marginY,defX,defY,defW,defH ;
  int fontId ;
  ByteColor color ;
  char txt[160] ;  // ALLOW EXTRA SPACE FOR SPRINTFING

  ProgressBar( const char* iTxt, int iFontId=0, int ix=cx, int iy=cy, int width=defW, int height=defH ) ;
  int nextY() ;
  ~ProgressBar() ;

  inline float r() const { return x + w ; }
  inline float b() const { return y + h ; }
} ;

#endif