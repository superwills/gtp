#include "ByteColor.h"
#include "../math/Vector.h"

ByteColor::ByteColor() : a(0), r(0), g(0), b(0)
{
}

ByteColor::ByteColor( BYTE ir, BYTE ig, BYTE ib ) : r(ir), g(ig), b(ib), a(255)
{
}

ByteColor::ByteColor( BYTE ir, BYTE ig, BYTE ib, BYTE ia ) :  r(ir), g(ig), b(ib), a( ia )
{
}

ByteColor::ByteColor( unsigned int byteColor ) : intValue( byteColor ) {}

ByteColor& ByteColor::operator+=( const ByteColor & other )
{
  r = min<int>( (int)r+other.r, 255 ) ;
  g = min<int>( (int)g+other.g, 255 ) ;
  b = min<int>( (int)b+other.b, 255 ) ;

  return *this ;
}

ByteColor& ByteColor::operator+=( int other )
{
  // interpret the int as ByteColor object
  ByteColor* c = (ByteColor*)(&other);
  r = min<int>( (int)r+c->r, 255 ) ;
  g = min<int>( (int)g+c->g, 255 ) ;
  b = min<int>( (int)b+c->b, 255 ) ;

  return *this ;
}

void ByteColor::zero()
{
  a=r=g=b=0 ;
}

void ByteColor::set( BYTE val )
{
  a=r=g=b=val ;
}

// Give self random value
ByteColor ByteColor::random()
{
  ByteColor bc ;
  bc.a = 255 ;
  bc.r = randInt( 0, 255 ) ;
  bc.g = randInt( 0, 255 ) ;
  bc.b = randInt( 0, 255 ) ;
  return bc;
}

ByteColor ByteColor::random( int low, int high )
{
  ByteColor bc;
  bc.a = 255 ;
  bc.r = randInt( low, high ) ;
  bc.g = randInt( low, high ) ;
  bc.b = randInt( low, high ) ;
  return bc;
}

Vector ByteColor::toRGBAVector() const
{
  return Vector( r/255.0, g/255.0, b/255.0, a/255.0 ) ;
}

Vector ByteColor::toRGBAVector( unsigned int color )
{
  return ByteColor( color ).toRGBAVector() ;
}

ByteColor operator+( const ByteColor & c1, const ByteColor & c2 )
{
  // clamp values above by 255
  return ByteColor(
    min<int>((int)c1.r + c2.r,255), 
    min<int>((int)c1.g + c2.g,255),
    min<int>((int)c1.b + c2.b,255) ) ;
}

ByteColor operator-( const ByteColor & c1, const ByteColor & c2 )
{
  // clamp values below by 0 (don't let negative underflow occur)
  return ByteColor( 
    max<int>((int)c1.r - c2.r,0), 
    max<int>((int)c1.g - c2.g,0), 
    max<int>((int)c1.b - c2.b,0) ) ;
}

ByteColor operator*( const ByteColor & c1, const ByteColor & c2 )
{
  real red   = c1.r*c2.r ;
  real green = c1.g*c2.g ;
  real blue  = c1.b*c2.b ;

  clamp<real>( red, 0, 255 ) ;
  clamp<real>( green, 0, 255 ) ;
  clamp<real>( blue, 0, 255 ) ;
  return ByteColor( red, green, blue ) ;
}

ByteColor operator*( const ByteColor & c, real mult )
{
  real red   = c.r*mult ;
  real green = c.g*mult ;
  real blue  = c.b*mult ;

  clamp<real>( red, 0, 255 ) ;
  clamp<real>( green, 0, 255 ) ;
  clamp<real>( blue, 0, 255 ) ;
  return ByteColor( red, green, blue ) ;
}

ByteColor operator*( real mult, const ByteColor & c )
{
  return c*mult;
}

ByteColor operator*( const ByteColor & c, const Vector & v )
{
  real red   = c.r*v.x ;
  real green = c.g*v.y ;
  real blue  = c.b*v.z ;

  clamp<real>( red, 0, 255 ) ;
  clamp<real>( green, 0, 255 ) ;
  clamp<real>( blue, 0, 255 ) ;
  return ByteColor( red, green, blue ) ;
}

ByteColor operator*( const Vector & v, const ByteColor & c )
{
  return c*v; 
}
