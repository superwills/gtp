#ifndef RAY_H
#define RAY_H

#include "../math/Vector.h"

// There are 4 types of rays:
//   1.  Pixel Rays or Eye Rays - Carry light directly to the eye thru a pixel on the screen (surface color - UNREFLECTED)
//   2.  Illumination or Shadow rays - carry light from a light source directly to an object surface
//   3.  Reflection rays  - carry light reflected by an object
//   4.  Transparency rays - carry light passing thru an object

// You need to start rays AHEAD of the surface point they are 
// to be shot from to avoid intersecting immediately with itself again.
//#define EPS_MIN 1e-3
// anything bigger than 1e-15 eliminates it in the RAYTRACER, but
// for sh tracing I need to use a bigger value (1e-8 is insufficient).

// for representing a 2d line
// was going to use this to tell "what side" of
// a line i'm on in tri split, but I didn't need it
struct Line2D
{
  Vector eq ;
  
  Line2D( const Vector& p1, const Vector& p2 )
  {
    eq.x = p1.y - p2.y ; 
    eq.y = p2.x - p1.x ;
    eq.z = p1.x*p2.y - p2.x*p1.y ;
  }

  // tells you what "Side" of the line
  // p is on. (3 possible answers are:
  // + side, 0 (on line), - side)
  int onLine( const Vector& p )
  {
    // can't use dot product because p.z is not *eq.z
    return p.x*eq.x + p.y*eq.y + eq.z ;
  }
} ;

struct Ray
{
  Vector startPos ;
  
  /// EXPECTED: Unit vector
  Vector direction ;

  // The eta the ray is currently in.
  // this changes as the ray is cast through the scene,
  // so copies of the ray are needed.
  Vector eta ;

  // the ray should carry it's rgb power WITH IT, instead of
  // passing this value onto subsequent calls of the ::cast function.
  Vector power ;

  int bounceNum ;

  // Is ray allowed to:
  //  - Bounce Diffuse
  //  - Carry emissive
  //  - Bounce Specular
  //  - Transmit?
  //bool DEST[4];
  // Introduced to NOT ALLOW backshot rays
  // (back to the light source) to perform
  // a specular bounce.
  bool isShadowRay ; // Can't reflect specularly

  /// the length of the Ray.
  real length ;

  Ray() ;

  /// Be sure to start the ray EPS_MIN ahead of
  /// the point you are tracing!
  Ray( const Vector& iStartPos, const Vector& iEndPos ) ;
  
  ////Ray( const Vector& iStartPos, const Vector& iEndPos, bool DONOTNORMALIZE, bool SAFETY2 ) ;

  /// Be sure to start the ray EPS_MIN ahead of
  /// the point you are tracing!
  Ray( const Vector& iStartPos, const Vector& iDirection, real iLength ) ;

  // I tried to avoid embedding optics code here,
  // but the ETA that a ray is travelling in is an important
  // property OF the ray.  also power.
  Ray( const Vector& iStartPos, const Vector& iDirection, real iLength,
       const Vector& startingEta, const Vector& rayPower, int iBounceNum ) ;

  //Ray( real startX, real startY, real startZ,
  //  real directionX, real directionY, real directionZ,
  //  real iLength ) ;

  Vector getEndPoint() const ;

  // Point in space along ray at t-value.
  Vector at( real t ) const ;

  // Reflects a ray, using normal to reflect from.
  // Includes power loss by factor KS,
  // and increases bounce number of the ray.
  Ray reflect( const Vector& normal, const Vector& spacePoint, const Vector& ks ) const ;

  // Guarantee avoid copying rays, in case the compiler does not elide correctly
  Ray* reflectPtr( const Vector& normal, const Vector& spacePoint, const Vector& ks ) const ;
  
  // Each refraction call produces 3 rays. r,g,b.
  void refract3( const Vector& normal, const Vector& toEta, const Vector& spacePoint, const Vector& kt, Ray rays[3] ) const ;

  // For refracting a ray in a single band
  Ray refract( const Vector& normal, int etaIndex, real toEta, const Vector& spacePoint, const Vector& kt ) const ;

  // For the special case when you only need to carry 1 refracted ray (same eta for RGB)
  Ray refract( const Vector& normal, real toEta, const Vector& spacePoint, const Vector& kt ) const ;

  Ray* refractPtr( const Vector& normal, int etaIndex, real toEta, const Vector& spacePoint, const Vector& kt ) const ;

  // one
  Ray* refractPtr( const Vector& normal, real toEta, const Vector& spacePoint, const Vector& kt ) const ;

} ;






#endif