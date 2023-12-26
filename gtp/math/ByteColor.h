#ifndef COLOR_H
#define COLOR_H

#include <vector>
using namespace std ;

// allows colors to be added, subtracted
#include <windows.h>
#include "../util/StdWilUtil.h"

union Vector ;

union ByteColor
{

  struct
  {

    BYTE r,g,b,a ;    // d3d11 is rgba
    // byteArray[0] is a
    // byteArray[1] is b
    // byteArray[2] is g
    // byteArray[3] is r

    ////BYTE b, g, r, a ; // little end first, so a
    // gets stored first.  its all to do
    // with the intValue here, because
    // at EOD that's all a color is,
    // a DWORD, and so, yeah.  because
    // d3d is argb by default, that's how it goes.
    // little endian, argb=>bgra
    // byteArray[0] is b
    // byteArray[1] is g
    // byteArray[2] is r
    // byteArray[3] is a
  } ;
  
  // when this is used as an ID,
  // then we assign this ByteColor::intValue
  // the id number..
  // DWORD is (d3d9) D3DFMT_A8R8G8B8 for bgra,
  // and (d3d11) DXGI_FORMAT_R8G8B8A8_UNORM for rgba.
  UINT intValue ;

  BYTE byteArray[4] ;

  ByteColor() ;

  ByteColor( BYTE ir, BYTE ig, BYTE ib ) ;
  ByteColor( BYTE ir, BYTE ig, BYTE ib, BYTE ia ) ;
  ByteColor( unsigned int byteColor ) ;

  ByteColor& operator+=( const ByteColor & other ) ;
  ByteColor& operator+=( int other ) ;

  void zero() ;

  void set( BYTE val ) ;

  // Give self random value
  static ByteColor random() ;

  static ByteColor random( int low, int high ) ;

  /// ranges 0..1
  Vector toRGBAVector() const ;

  static Vector toRGBAVector( unsigned int color ) ;
  
} ;

ByteColor operator+( const ByteColor & c1, const ByteColor & c2 ) ;
ByteColor operator-( const ByteColor & c1, const ByteColor & c2 ) ;
ByteColor operator*( const ByteColor & c1, const ByteColor & c2 ) ;

ByteColor operator*( const ByteColor & c, real mult ) ;
ByteColor operator*( real mult, const ByteColor & c ) ;

ByteColor operator*( const ByteColor & c, const Vector & v ) ;
ByteColor operator*( const Vector & v, const ByteColor & c ) ;

#endif