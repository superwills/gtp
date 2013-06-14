#include "Ray.h"

/// Default ray is ray at origin
/// with direction -z
Ray::Ray():direction(0.0,0.0,-1.0),length(1.0)
{
  isShadowRay=false;
}

Ray::Ray( const Vector& iStartPos, const Vector& iEndPos ):
  startPos( iStartPos ) 
{
  // get direction vector, store original length, normalize
  direction = iEndPos - iStartPos ;
  length = direction.len() ; // i need to save the length
  if( length )
    direction /= length ; //.normalize() ; // save comp
  isShadowRay=false;
}

////Ray::Ray( const Vector& iStartPos, const Vector& iEndPos, bool DONOTNORMALIZE, bool SAFETY2 ):
////  startPos( iStartPos ) 
////{
////  // here, the ray is allowed to travel
////  // from 0.0 to 1.0, and the ray is not
////  // normalized.  so the "length" is
////  // embedded in the direction vector.
////  // this is more efficient for some things,
////  // to avoid the normalization and so on.
////  direction = iEndPos - iStartPos ;
////  length = 1.0 ;
////}

Ray::Ray( const Vector& iStartPos, const Vector& iDirection, real iLength ):
  startPos( iStartPos ), direction( iDirection ), length( iLength )
{
  direction.normalize();//make sure its normalized
  isShadowRay=false;
}

Ray::Ray( const Vector& iStartPos, const Vector& iDirection, real iLength, const Vector& startingEta, const Vector& rayPower, int iBounceNum ):
  startPos( iStartPos ), direction( iDirection ), length( iLength ), eta( startingEta ), power( rayPower ), bounceNum( iBounceNum )
{
  direction.normalize() ;
  isShadowRay=false;
}

Vector Ray::getEndPoint() const
{
  return startPos + length * direction ;
}

Vector Ray::at( real t ) const
{
  return startPos + t*direction ;
}

Ray Ray::reflect( const Vector& normal, const Vector& spacePoint, const Vector& ks ) const
{
  Vector reflDir = direction.reflectedCopy( normal ) ;
  return Ray( spacePoint + EPS_MIN*normal, reflDir, 1000, eta, power*ks, bounceNum+1 ) ;
}

Ray* Ray::reflectPtr( const Vector& normal, const Vector& spacePoint, const Vector& ks ) const
{
  Vector reflDir = direction.reflectedCopy( normal ) ;
  return new Ray( spacePoint + EPS_MIN*normal, reflDir, 1000, eta, power*ks, bounceNum+1 ) ;
}

void Ray::refract3( const Vector& normal, const Vector& toEta, const Vector& spacePoint, const Vector& kt, Ray rays[3] ) const
{
  for( int i = 0 ; i < 3 ; i++ )
  {
    Vector refractDir = direction.refractedCopy( normal, eta.e[i], toEta.e[i] ) ;
    
    // The new power filter cuts off 2 bands, and
    // scales the last (eg (r,0,0)).
    Vector newPower ;
    newPower.e[i] = power.e[i] * kt.e[i] ;
    
    // push the ray off the surface ever so slightly, in the direction of the REFRACT DIR
    // (because if the ray is ENTERING the surface, then you need to go in the OPPOSITE direction
    // of the normal).
    rays[i] = Ray( spacePoint + refractDir * EPS_MIN, refractDir, length, toEta, newPower, bounceNum+1 ) ;
  }
}

// For refracting a ray in a single band
Ray Ray::refract( const Vector& normal, int etaIndex, real toEta, const Vector& spacePoint, const Vector& kt ) const
{
  // the TOETA gets turned around if the DIRECTION _faces_ the NORMAL.
  Vector refractDir = direction.refractedCopy( normal, eta.e[etaIndex], toEta ) ;
  
  Vector newPower ;
  newPower.e[ etaIndex ] = power.e[ etaIndex ] * kt.e[ etaIndex ];
  
  return Ray( spacePoint + refractDir * EPS_MIN, refractDir, length, toEta, newPower, bounceNum+1 ) ;
}

Ray Ray::refract( const Vector& normal, real toEta, const Vector& spacePoint, const Vector& kt ) const
{
  // the TOETA gets turned around if the DIRECTION _faces_ the NORMAL.
  Vector refractDir = direction.refractedCopy( normal, eta.x, toEta ) ;
  
  Vector newPower ;
  newPower = power * kt ;
  
  return Ray( spacePoint + refractDir * EPS_MIN, refractDir, length, toEta, newPower, bounceNum+1 ) ;
}

Ray* Ray::refractPtr( const Vector& normal, int etaIndex, real toEta, const Vector& spacePoint, const Vector& kt ) const
{
  // the TOETA gets turned around if the DIRECTION _faces_ the NORMAL.
  Vector refractDir = direction.refractedCopy( normal, eta.e[etaIndex], toEta ) ;
  
  Vector newPower ;
  newPower.e[ etaIndex ] = power.e[ etaIndex ] * kt.e[ etaIndex ];
  
  return new Ray( spacePoint + refractDir * EPS_MIN, refractDir, length, toEta, newPower, bounceNum+1 ) ;
}

Ray* Ray::refractPtr( const Vector& normal, real toEta, const Vector& spacePoint, const Vector& kt ) const
{
  // the TOETA gets turned around if the DIRECTION _faces_ the NORMAL.
  // equal eta, use x
  Vector refractDir = direction.refractedCopy( normal, eta.x, toEta ) ;
  
  Vector newPower ;
  newPower = power * kt ;
  
  return new Ray( spacePoint + refractDir * EPS_MIN, refractDir, length, toEta, newPower, bounceNum+1 ) ;
}