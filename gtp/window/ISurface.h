#ifndef ISURFACE_H
#define ISURFACE_H

#include <Windows.h>
#include "../math/ByteColor.h"

struct Vec3 ;

enum SurfaceType
{
  ColorBuffer,  // CPU WRITE, GPU READ.  Can use as
                // raytraceTex, matrix of 4d vectors, with 1 byte components
                // you can put whatever you want in here
  RenderTarget  // GPU WRITE, CPU NEEDS TO COPY DATA TO SEE.
                // meant to be used by rendering system as thing to render to
} ;

// The generic interface a Surface is required to implement

// I need to break this into 2 classes:
//   ColorBuffer
//   RenderTarget
struct ISurface
{
  // to read pixel values we must "lock" the surface.
  // what we'll do is require an "open" and "close"
  // in the interface

  // PURE says purely abstract, so linker, don't
  // look for an ISurface::open() function imp
  // (you won't find it)
  virtual BYTE* open() PURE ;
  virtual void close() PURE ;
  virtual bool isOpened() PURE ;

  virtual void draw() PURE ;

  virtual int getWidth() const PURE ;
  virtual int getHeight() const PURE ;
  virtual int getNumPixels() const PURE ;

  // function to get a pointer to the color buffer
  // of this surface.
  virtual ByteColor* getColorAt( int pxNum ) PURE ;
  virtual ByteColor* getColorAt( int row, int col ) PURE ;

  virtual void setColorAt( int row, int col, ByteColor val ) PURE ;
  
  // set a pixel's color using 2 shorts instead of 4 bytes
  virtual void setColorAt( int row, int col, USHORT h1, USHORT h2 ) PURE ;

  virtual void setColorAt( int row, int col, const Vec3& color ) PURE ;
  
  virtual void copyFromByteArray( BYTE * arrSrc, int bytesPerPixel, int numPixels ) PURE ;
  
  // make this the active texture
  virtual void activate() PURE ;

  // unset this as the active texture
  virtual void deactivate() PURE ;

  // set as active surface:
  // bind this surface to output-merger stage
  virtual void rtBind() PURE ;

  // binds the last active surface there was
  virtual void rtUnbind() PURE ;

  // binds the "screen" as the active surface (really the backbuffer)
  virtual void rtBindMainscreen() PURE ;

  // saves the surface to a .png file
  virtual bool save( char* filename ) PURE ;

  // saves to a DDS
  virtual void saveDDS( char* filename ) PURE ;

  // saves jpg
  virtual void saveJPG( char* filename ) PURE ;
} ;



#endif