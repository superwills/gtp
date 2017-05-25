#ifndef VECTOR4_H
#define VECTOR4_H

#include <math.h>
#include "../util/StdWilUtil.h"
#include "Matrix.h"
#include "ByteColor.h"

// What it means to be "near"
//const real EPS = 1e-3f ;
// You need to start rays AHEAD of the surface point they are 
// to be shot from to avoid intersecting immediately with itself again.
#define EPS_MIN 1e-6
// anything bigger than 1e-15 eliminates it in the RAYTRACER, but
// for sh tracing I need to use a bigger value (1e-8 is insufficient).

// BIGGER EPSILON==should have FEWER errors for rayshooting
// should make the octree bin much more coursely.

// Large epsilon also has the side effect of ruining meshes
// (4 verts too close together skip/don't form quad)

union Matrix ;
union ByteColor;

/// *** ON Vector.
/// Here I haven't included 'w' in
/// any of the cross product or anything
/// computations.  'w' is ONLY used
/// in perspective projection.  For
/// all other intents and purposes this
/// is a Vector3.. class.. hence why it is
/// named simply "Vector" ..

union Vector ;

struct WaveParam ;

struct SVector
{
  real r,
    tElevation, // angle from +y
    pAzimuth ;  // angle from +x axis

  SVector() { r=pAzimuth=tElevation=0 ; }
  SVector( real ir, real iTElevation, real iPAzimuth ):
  r(ir),tElevation(iTElevation),pAzimuth(iPAzimuth) { }
  Vector toCartesian() const ;

  inline ByteColor toByteColor() const {
    return ByteColor( r, tElevation/PI * 255.0, pAzimuth/(2*PI) * 255.0 ) ; // w can be used
    // r is represented poorly.  you could take r*10 for RED, or r*100,
    // and then do 
  }

  // Gives a random point on the sphere
  static SVector random( real radius ) ;

  // Gets you a random vector on a hemisphere about
  // the normal.
  static Vector randomHemi( const Vector& normal ) ;

  // This gives you a random distribution
  // actually in a lobe about the normal.
  static SVector randomHemiCosine( real radius ) ;

  static Vector randomHemiCosine( const Vector& normal ) ;
} ;

union Vector
{
  // This breaks due to C2719
  //__m256 mreg ;
  
  #pragma region members
  real e[ 4 ] ; // allows programmatic selection of members. (e[i] instead of having to do .x)

  struct {
    real x,y,z,w ;
  } ;

  
  //struct { real r,g,b,a ; } ;                      // an alternative to make the Vector multifunc.
  //struct { real radius,tElevation,pAzimuth ; } ;   // an alternative to make the Vector multifunc.
  // see also http://stackoverflow.com/questions/2253878/why-does-c-disallow-unnamed-structs-and-unions
  // to get around on linux, try
  // #define x e[0]
  // #define y e[1]
  // OR 
  // inline real x() const { return e[0]; }
  // That may be the reason Shirley used only an array
  // in his "Realistic Ray tracing" book.
  #pragma endregion

  #pragma region <CTOR>
  Vector():x(0.0),y(0.0),z(0.0),w(1.0) { }  //!! concern: w is ALWAYS initialized @1

  // init the vector to same value. required for the
  // wavelet functions in swu::numeric to work for generic T.
  // biter: if you init a color or a vector with this, the w value isn't 1.0.
  // THE ONLY TIME w IS NOT __1__ on construction is here, and in the ctor that passes a 'w'
  Vector( real v ):x(v),y(v),z(v),w(v) { } 

  Vector( real ix, real iy ):x(ix),y(iy),z(0.0),w(1.0) { }

  Vector( real ix, real iy, real iz ):x(ix),y(iy),z(iz),w(1.0) { }

  Vector( real ix, real iy, real iz, real iw ):x(ix),y(iy),z(iz),w(iw) { }

  Vector( const Vector & b ):x(b.x),y(b.y),z(b.z),w(b.w) { }

  // dangerous choice. replaced with single number ctor above
  //Vector( FILE* file ){ fread( &x, sizeof(Vector), 1, file ) ; }
  void load( FILE* file ){ fread( &x, sizeof(Vector), 1, file ) ; }

  int save( FILE* file ) { return sizeof(Vector)*fwrite( &x, sizeof(Vector), 1, file ) ; }

  ////Vector( const D3DXVECTOR3 & v ):x(v.x),y(v.y),z(v.z) {}
  ////
  ////static Vector fromD3DVector( const D3DXVECTOR3 & v ) {
  ////  return Vector( v.x, v.y, v.z ) ;
  ////}

  static Vector fromByteColor( int ir, int ig, int ib ){
    return Vector( ir/255.0, ig/255.0, ib/255.0 ) ;
  }

  static Vector fromByteColor( int ir, int ig, int ib, int ia ){
    Vector color = Vector( ir/255.0, ig/255.0, ib/255.0 ) ;
    color.w = ia/255.0 ;
    return color ;
  }

  static Vector fromByteColor( unsigned int byteColor )
  {
    ByteColor bc ;
    bc.intValue = byteColor ;
    return bc.toRGBAVector() ;
  }

  /// Gives you a vector that is spatially between
  /// the AABB formed by min, max.
  static Vector randomWithin( const Vector & min, const Vector & max ) {
    return Vector( randFloat( min.x, max.x ), 
      randFloat( min.y, max.y ), 
      randFloat( min.z, max.z )
    ) ;
  }

  // graphics gems page 
  static Vector randBary() {
    real s=sqrt( randFloat() ), t=randFloat() ;
    Vector bary ;
    bary.x = 1 - t ;
    bary.y = ( 1 - s ) * t ;
    bary.z = s*t;
    return bary ;
  }
  
  static Vector randomPointInsideTri( const Vector& a, const Vector& b, const Vector& c ) ;
  //static inline Vector randomPointInsideTri( const Vector& a, const Vector& b, const Vector& c ) {
  //  Vector bary = randBary() ;
  //  return a*bary.x + b*bary.y + c*bary.z ; // won't compile b/c operator* for Vector not encountered yet
  //}

  static Vector random() {
    return Vector( randFloat(), randFloat(), randFloat() ) ;
  }

  static Vector random( real low, real high ) {
    return Vector( randFloat( low, high ), randFloat( low, high ), randFloat( low, high ) ) ;
  }

  static Vector randomSign() {
    return Vector( randSign(), randSign(), randSign() ) ;
  }

  //static Vector Zero, One ; // static's not allowed in union
  
  // r,g,b values are from 0 to 1
  // h = [0,360], s = [0,1], v = [0,1]
  //		if s == 0, then h = -1 (undefined)
  static Vector toHSV( real r, real g, real b )
  {
    Vector hsv ;
	  
    float min, max, delta;

    // if you have a problem here because you're using Gdiplus,
    // just remove std:: from in front and let the compiler use
    // the min, max fcns defined in Windef.h.
	  min = std::min( std::min( r, g ), b ) ; // MIN( r, g, b );
	  max = std::max( std::max( r, g ), b ) ; // MAX( r, g, b );
	  hsv.z = max;				// v
	  
    delta = max - min;
	  if( max != 0 )
		  hsv.y = delta / max;		// s
	  else {
		  // r = g = b = 0		// s = 0, v is undefined
		  hsv.y = 0;  //s
		  hsv.x = -1; //h
		  return hsv ;
	  }
	  if( r == max )
		  hsv.x = ( g - b ) / delta;		// h: between yellow & magenta
	  else if( g == max )
		  hsv.x = 2 + ( b - r ) / delta;	// h: between cyan & yellow
	  else
		  hsv.x = 4 + ( r - g ) / delta;	// h: between magenta & cyan

    hsv.x *= 60;				// h: degrees (it would have been 0 to 6).
	  if( hsv.x < 0 )
		  hsv.x += 360; //h:

      return hsv;
  }

  static Vector toRGB( real h, real s, real v )
  {
    Vector rgb ;

	  int i;
	  float f, p, q, t;
	  if( s == 0 ) {
		  // achromatic (grey)
		  rgb.x = rgb.y = rgb.z = v;
		  return rgb ;
	  }

    h /= 60;			// sector 0 to 5
	  i = floor( h );
	  f = h - i;			// factorial part of h
	  p = v * ( 1 - s );
	  q = v * ( 1 - s * f );
	  t = v * ( 1 - s * ( 1 - f ) );
	  switch( i ) {
		  case 0:
			  rgb.x = v;
			  rgb.y = t;
			  rgb.z = p;
			  break;
		  case 1:
			  rgb.x = q;
			  rgb.y = v;
			  rgb.z = p;
			  break;
		  case 2:
			  rgb.x = p;
			  rgb.y = v;
			  rgb.z = t;
			  break;
		  case 3:
			  rgb.x = p;
			  rgb.y = q;
			  rgb.z = v;
			  break;
		  case 4:
			  rgb.x = t;
			  rgb.y = p;
			  rgb.z = v;
			  break;
		  default:		// case 5:
			  rgb.x = v;
			  rgb.y = p;
			  rgb.z = q;
			  break;
	  }

    return rgb ;
  }
  #pragma endregion
  
  #pragma region <CONST>
    // Well, I'm NOT including w in this comparison.
  inline bool operator==( const Vector & b ) const {
    return x==b.x && y==b.y && z==b.z ; // w not considered
  }

  inline bool operator!=( const Vector & b ) const {
    return x!=b.x || y!=b.y || z!=b.z ; // w not considered
  }

  inline int maxIndex() const { 
    if( x >= y && x >= z )  return 0 ;
    else if( y >= x && y >= z ) return 1 ;
    else return 2 ;
  }
  inline real max() const { return max3( x,y,z ) ; }
  inline real min() const { return min3( x,y,z ) ; }
  inline real getAvg() const { return (x+y+z)/3 ; }
 
  inline bool all( const real& val ) const { return x==val && y==val && z==val; }
  inline bool allEqual() const { return x==y && x==z; }
  inline bool nonzero() const { return x || y || z ; }

  // IsNear macro not available
  //inline bool allNearly( const real& val ) const { return IsNear(x,val) && IsNear(y,val) && IsNear(z,val) ; }
  inline bool allNearly( const real& val ) const { return fabs(x-val) < EPS_MIN && fabs(y-val) < EPS_MIN && fabs(z-val) < EPS_MIN ; }
  
  inline bool notAll( const real& val ) const { return x!=val || y!=val || z!=val ; }
  inline bool anyGreaterThan( const real& val ) const { return x>val || y>val || z>val; }
  inline bool anyLessThan( const real& val ) const { return x<val || y<val || z<val; }

  inline bool Near( const Vector & b ) const {
    return fabs( x - b.x ) < EPS_MIN &&
           fabs( y - b.y ) < EPS_MIN &&
           fabs( z - b.z ) < EPS_MIN ;
  }

  inline bool Near( const Vector & b, const real& epsVal ) const {
    return fabs( x - b.x ) < epsVal &&
           fabs( y - b.y ) < epsVal &&
           fabs( z - b.z ) < epsVal ;
  }

  inline real Dot( const Vector & b ) const {
    return x*b.x + y*b.y + z*b.z ;
  }

  inline bool isOrthogonalWith( const Vector& b ) const {
    return IsNear( Dot( b ), 0.0, EPS_MIN ) ;
  }

  inline Vector componentWiseMultiply( const Vector& b ) const{
    return Vector( x*b.x, y*b.y, z*b.z ) ;
  }

  inline Vector componentWiseDivide( const Vector& b ) const{
    return Vector( x/b.x, y/b.y, z/b.z ) ;
  }

  inline Vector componentSquared() const {
    return Vector(x*x, y*y, z*z) ;
  }

  inline real len() const {
    // 
    //_mm256_addsub_pd
    return sqrt( x*x+y*y+z*z );  // w not considered
  }

  inline real len2() const {
    return x*x + y*y + z*z ;  // w not considered
  }

  inline bool isPositive() const {
    return x > 0 && y > 0 && z > 0 ;
  }

  // for when using a vector as barycentric..
  inline bool isNonnegative() const {
    return x >= 0 && y >= 0 && z >= 0 ;
  }

  // any of the values in the vector has a bad
  // value
  inline bool isBad() const {
    return IsNaN( x ) || IsNaN( y ) || IsNaN( z ) ||
           IsInfinite( x ) || IsInfinite( y ) || IsInfinite( z ) ;
  }

  inline real angleWith( const Vector& b ) const {
    return acos( (this->Dot( b ))/(len()*b.len()) ) ;
  }

  // Call this if you know the vectors are already normalized
  inline real angleWithNormalized( const Vector& bNormalized ) const {
    return acos( this->Dot( bNormalized ) ) ;
  }

  inline bool betweenIn( const Vector& min, const Vector& max ) const {
    return BetweenIn( x, min.x, max.x ) &&
           BetweenIn( y, min.y, max.y ) &&
           BetweenIn( z, min.z, max.z ) ;
  }

  inline bool betweenEx( const Vector& min, const Vector& max ) const {
    return BetweenEx( x, min.x, max.x ) &&
           BetweenEx( y, min.y, max.y ) &&
           BetweenEx( z, min.z, max.z ) ;
  }

  inline bool betweenNearly( const Vector& min, const Vector& max ) const {
    return BetweenApprox( x, min.x, max.x ) &&
           BetweenApprox( y, min.y, max.y ) &&
           BetweenApprox( z, min.z, max.z ) ;
  }

  // These won't inline because operator- hasn't been defined yet.
  //real distanceTo( const Vector& b ) const {
  //  return ((*this) - b).len() ;
  //}
  
  //real distanceTo2( const Vector& b ) const {
  //  return ((*this) - b).len2() ;
  //}

  // Returns true if a is closer to THIS than b is
  bool isCloser( const Vector& amICloser, const Vector& thanMe ) const ;

  int printInto( char *buf ) const ;
  void printPlain() const ;
  void print() const ;

  inline ByteColor toByteColor() const {
    return ByteColor( x*255,y*255,z*255,w*255 ) ;
  }

  // moves (-1,-1,-1),(1,1,1) range to
  // (0,0,0),(1,1,1)
  ByteColor toSignedByteColor() const ;

  SVector toSpherical() const ;

  inline int getDominantAxis() const {
    if( fabs(x) > fabs(y) && fabs(x) > fabs(z) )  return 0 ;
    else if( fabs(y) > fabs(x) && fabs(y) > fabs(z) )  return 1 ;
    else if( fabs(z) > fabs(x) && fabs(z) > fabs(y) )  return 2 ;
    else return -1 ; // (none -- at least two axes equal.
    // this happens for (0,0,0), (1,1,0) etc.
  }

  // Returns a dominant axis even when two of the axes
  // are equal. The "lowest" axis is returned
  // (e.g. (1,1,0) has x as the dominant axis)
  inline int getDominantAxisEq() const {
    if( all(0) ) return -1 ; // first check that they're all not 0
    else if( fabs(x) >= fabs(y) && fabs(x) >= fabs(z) )  return 0 ; // now this bins (1,1,0) with just axis 0
    else if( fabs(y) >= fabs(x) && fabs(y) >= fabs(z) )  return 1 ;
    else if( fabs(z) >= fabs(x) && fabs(z) >= fabs(y) )  return 2 ;
    else return -1 ; // (none -- at least two axes equal.
    // this happens for (0,0,0), (1,1,0) etc.
  }

  /// Gets you a perpendicular vector.  Tries to
  /// use the dominant axis so ( 0, 0, 1 ) will
  /// return ( -1, 0, 0 ) (a correct perpendicular)
  /// not ( -0, 0, 1 ) (which is useless, same vector)
  inline Vector getPerpendicular() const {
    // in the below switch, we
    // detect the dominant axis, and make sure
    // to use that component in the swap.
    // also we AVOID using a zero component for
    // the swap:  (-0.7, 0, 0.6) => (-0, -0.7, 0.6) is NOT a perpendicular
    // (but it would have been if the y component was non-zero)

    // (0,0,0) returns a vector (0, -0, 0)

    // unintuitively, we must zero a component
    // to get the perpendicular.
    switch( getDominantAxis() )
    {
    case 0: // x should be involved.  (1, 0, 0) --> ( -0, 1, 0 )
      if( y == 0.0 ) // avoid y
        return Vector( z, 0, -x ) ; // swap x, z. apply negation to x
      else
        return Vector( y, -x, 0 ) ; // swap x, y
      break ;
    case 1: // y should be involved:  (-1, 1, -1) --> (-1,-1, -1)
      // avoid swapping a zero component
      if( x == 0.0 ) // avoid x
        return Vector( 0, z, -y ) ; // y,z
      else
        return Vector( -y, x, 0 ) ; // x,y
    case 2: // z should be involved
    case -1: // No matter which we switch.
    default:
      if( x == 0.0 ) // avoid x
        return Vector( 0, -z, y ) ; // y,z [(0,0,0) case]
      else
        return Vector( -z, 0, x ) ; // swap -z,x
    }
  }

  inline Vector normalizedCopy() const {
    real length = len2() ;
    if( length == 0 ) return *this ; // avoid the sqrt op if 0 len
    else if( IsNear( length, 1, EPS_MIN ) ) return *this ;  // don't compute of already near 1
    length = sqrt( length ) ;
    return Vector( x/length,y/length,z/length ) ;
  }

  // Gives you a vector JUST offset from this one
  // in the direction of "direction".  Mostly used
  // when raytracing and setting the eye JUST OFF the surface.
  ////inline Vector offset( const Vector& direction ) const {
  ////  return (*this) + direction*EPS_MIN ;
  ////}

  inline void writeFloat3( float* arr ) const{
    arr[ 0 ] = (float)e[ 0 ] ;
    arr[ 1 ] = (float)e[ 1 ] ;
    arr[ 2 ] = (float)e[ 2 ] ;
  }

  inline void writeFloat4( float* arr ) const{
    arr[ 0 ] = (float)e[ 0 ] ;
    arr[ 1 ] = (float)e[ 1 ] ;
    arr[ 2 ] = (float)e[ 2 ] ;
    arr[ 3 ] = (float)e[ 3 ] ;
  }

  Vector& jitter( real amnt ) ;
  Vector jitteredCopy( real amnt ) const ;

  Vector reflectedCopy( const Vector& N ) const ;
  Vector refractedCopy( const Vector& N, real n1, real n2 ) const ;

  /// gives you a vector halfway between "me" and "other"
  Vector halfWay( const Vector& other ) const ;

  Vector getRandomPerpendicular() const ;

  inline bool acute( const Vector& other ) const
  {
    // dot( this, other ) > 0 means angle is acute,
    // ==0 means they are orthogonal
    // < 0 means they are obtuse
    return (x*other.x + y*other.y + z*other.z) > 0 ;
  }
  inline bool obtuse( const Vector& other ) const
  {
    // dot( this, other ) > 0 means angle is acute,
    // ==0 means they are orthogonal
    // < 0 means they are obtuse
    return (x*other.x + y*other.y + z*other.z) < 0 ;
  }

  inline Vector clampedCopy( const Vector& low, const Vector& high )
  {
    Vector v = *this ;
    return v.clamp( low.x, high.x, low.y, high.y, low.z, high.z ) ;
  }

  // I NEED FUNCTIONS THAT TREAT VECTORS AS VEC4.
  // These are mostly used for when COLORS are being
  // represented by vectors.
  // AddMe functions are +=
  // Add function is 4-component +.
  Vector Add4( const Vector& v ) const {
    return Vector( x+v.x, y+v.y, z+v.z, w+v.w ) ;
  }
  Vector Add4( real val ) const {
    return Vector( x+val, y+val, z+val, w+val ) ;
  }
  Vector Sub4( const Vector& v ) const {
    return Vector( x-v.x, y-v.y, z-v.z, w-v.w ) ;
  }
  Vector Sub4( real val ) const {
    return Vector( x-val, y-val, z-val, w-val ) ;
  }
  Vector Mult4( const Vector& v ) const {
    return Vector( x*v.x, y*v.y, z*v.z, w*v.w ) ;
  }
  Vector Mult4( real val ) const {
    return Vector( x*val, y*val, z*val, w*val ) ;
  }
  Vector Div4( const Vector& v ) const {
    return Vector( x/v.x, y/v.y, z/v.z, w/v.w ) ;
  }
  Vector Div4( real val ) const {
    return Vector( x/val, y/val, z/val, w/val ) ;
  }
  #pragma endregion

  #pragma region <NON-CONST>
  // CONSIDERS ALPHA VALUES
  // USED FOR COLOR MANIPULATION.
  // AddMe functions are +=
  // Add function is 4-component +.
  Vector& AddMe4( const Vector& v ) {
    x+=v.x, y+=v.y, z+=v.z, w+=v.w ; return (*this) ;
  }
  Vector& SubMe4( const Vector& v ) {
    x-=v.x, y-=v.y, z-=v.z, w-=v.w ; return (*this) ;
  }
  Vector& MultMe4( const Vector& v ) {
    x*=v.x, y*=v.y, z*=v.z, w*=v.w ; return (*this) ;
  }
  Vector& DivMe4( const Vector& v ) {
    x/=v.x, y/=v.y, z/=v.z, w/=v.w ; return (*this) ;
  }

  Vector& AddMe4( real a ) {
    x+=a, y+=a, z+=a, w+=a ; return (*this) ;
  }
  Vector& SubMe4( real a ) {
    x-=a, y-=a, z-=a, w-=a ; return (*this) ;
  }
  Vector& MultMe4( real a ) {
    x*=a, y*=a, z*=a, w*=a ; return (*this) ;
  }
  Vector& DivMe4( real a ) {
    x/=a, y/=a, z/=a, w/=a ; return (*this) ; 
  }

  // this method doesn't exist because it isn't any
  // more efficient than v = mat*v; and you HAVE TO
  // say "columnMajor" to avoid ambiguity.
  // (the reason it can't be more efficient is you have to
  //  complete the operation before assigning new members
  //  values - so a full copy must be made anyway).
  ////Vector& transformColumnMajor( const Matrix& mat ) ;

  // FLIP about the normal,
  Vector& flip( const Vector& N ) ; 

  Vector& reflect( const Vector& N ) ;
  Vector& refract( const Vector& N, real n1, real n2 ) ;

  // Allows assignment from RGBA 255 bytecolors
  //Vector& operator=( const ByteColor& byteColor ) ;

  // allows assignment from ints
  Vector& operator=( int v ) ;
  Vector& operator=( real v ) ;

  inline real& operator[]( int index ) {
    return e[ index ] ;
  }

  inline Vector& negate(){
    x = -x ;   y = -y ;   z = -z ;
    return *this ;
  }
  
  inline Vector& normalize(){
    real length = len2() ;
    if( length == 0 ) return *this ; // avoid the sqrt and divisions if 0 len
    length = sqrt( length ) ;
    x /= length ;   y /= length ;   z /= length ;
    return (*this) ;
  }

  inline Vector& setMag( real newMag ){
    this->normalize() ;
    (*this) *= newMag ;

    return *this ;
  }

  inline Vector& setxyz( real val ){
    x=y=z=val;
    return *this;
  }

  inline Vector& rotateX( real radians ) {
    real oy = y*cos(radians) - z*sin(radians) ;
    real oz = y*sin(radians) + z*cos(radians) ;
    y=oy;z=oz;
    return *this ;
  }

  inline Vector& rotateY( real radians ) {
    real oz = z*cos(radians) - x*sin(radians) ;
    real ox = z*sin(radians) + x*cos(radians) ;
    z=oz;
    x=ox;
    return *this ;
  }

  inline Vector& rotateZ( real radians ) {
    real ox = x*cos(radians) - y*sin(radians) ;
    real oy = x*sin(radians) + y*cos(radians) ;
    x=ox;
    y=oy;
    return *this ;
  }

  /// Adds another vector to this one,
  /// mutating _this_.
  inline Vector& operator+=( const Vector & b ) {
    x += b.x ;   y += b.y ;   z += b.z ;
    return *this ;
  }

  /// Subtracts another vector from
  /// this one.
  inline Vector& operator-=( const Vector & b ) {
    x -= b.x ;   y -= b.y ;   z -= b.z ;
    return *this ;
  }

  inline Vector& operator*=( const real b ) {
    x *= b ;     y *= b ;     z *= b ;
    return *this ;
  }

  // Component-wise
  inline Vector& operator*=( const Vector& b ) {
    x *= b.x ;     y *= b.y ;     z *= b.z ;
    return *this ;
  }

  inline Vector& operator/=( const real b ) {
    x /= b ;     y /= b ;     z /= b ;
    return *this ;
  }

  // Component-wise
  inline Vector& operator/=( const Vector& b ) {
    x /= b.x ;     y /= b.y ;     z /= b.z ;
    return *this ;
  }

  inline Vector& clamp(
    real lx, real hx,
    real ly, real hy,
    real lz, real hz )
  {
    swu::clamp<real>( x, lx, hx ) ;
    swu::clamp<real>( y, ly, hy ) ;
    swu::clamp<real>( z, lz, hz ) ;

    return *this ;
  }

  inline Vector& clamp( real low, real high )
  {
    swu::clamp<real>( x, low, high ) ;
    swu::clamp<real>( y, low, high ) ;
    swu::clamp<real>( z, low, high ) ;

    return *this ;
  }

  inline Vector& clamp( const Vector& low, const Vector& high )
  {
    clamp( low.x, high.x, low.y, high.y, low.z, high.z ) ;
    return *this ;
  }
  #pragma endregion

  // Static
  static void wave( real time, real phase, real freq, real amp,
    const Vector& longAxis, const Vector& transverseAxis, Vector& pos, Vector& norm ) ;

  static void wave( real time, const WaveParam& wp, Vector& pos, Vector& norm ) ;

} ;


struct WaveParam
{
  real freq ;
  real amp ;
  Vector longAxis ;
  Vector transverseAxis ;

  WaveParam( real iFreq, real iAmp, const Vector& iLongAxis, const Vector& iTransverseAxis ) ;

  // Gets you the phase of a position in space by
  // projecting onto the longitudinal axis
  real getPhase( const Vector& pos ) const ;
} ;


#pragma region outside operators
/// CONST
/// Operators outside.  We don't want to define
/// a.plus( b ) ; rather its more like add( a, b ) here
Vector operator+( const Vector & a, const Vector & b ) ;
Vector operator+( const real & a, const Vector & b ) ;
Vector operator+( const Vector & a, const real & b ) ;

Vector operator-( const Vector & a, const Vector & b ) ;
Vector operator-( const real & a, const Vector & b ) ;
Vector operator-( const Vector & a, const real & b ) ;
// unary minus
Vector operator-( const Vector & a ) ;

real operator%( const Vector & a, const Vector & b ) ;

bool operator,( const Vector & a, const Vector & b ) ;

// multiplication by a vector is going to be the 
// same thing it is in shaders: component wise
// multiply
Vector operator*( const Vector & a, const Vector & b ) ;

Vector operator<<( const Vector & a, const Vector & b ) ;

Vector operator*( const Vector & a, const real b ) ;

Vector operator*( const real b, const Vector & a ) ;





/// Pre multiply matrix by vector
Vector operator*( const Vector& v, const Matrix & a ) ;

// Transforms a 3-vector while leaving off the translation component
Vector transformNormal( const Vector& n, const Matrix & a ) ;

/// Premultiply matrix by vector.
// We chose to forbid this operation to force
// correct and consistent matrix multiplies.
//Vector operator*( const Vector& v, const Matrix & matrix ) ;

Vector operator/( const Vector & a, const real b ) ;
Vector operator/( const Vector & a, const Vector& b ) ;



inline real distanceBetween( const Vector& a, const Vector& b )
{
  return (a-b).len() ;
}

inline real distanceBetween2( const Vector& a, const Vector& b )
{
  return (a-b).len2() ;
}

// bad macro: hard to read.
// and determining if (a vector A is
// closer to a vector refVector than a vector C is.)
//#define isCloserThan( A,B,C ) ( dist2(A,B) > dist2(C,B) )

#pragma endregion


#endif