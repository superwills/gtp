#include "Plane.h"
#include "Ray.h"
#include "Intersection.h"

bool Plane::intersectsPlane( const Ray& ray, Intersection* intn )
{
  // 1)  Intersect against plane in which triangle lies.
  //     Call this intersection point P.
  
  // 4x misses:
  //RayShootingAwayFromPlane
  // t < 0

  //RayFallsShortOfPlane
  // t > rayLength
  
  //RayParallelToPlane
  // Either inside plane or
  // skewed
  // t = INF
  
  //RayHitsPlaneMissesTriangle
  // Valid t, barycentric eliminates intersection
  // 0 < t < rayLength

  //RayHitsTriangle
  // 0 < t < rayLength, and
  // barycentric confirms intn
  real den = normal.Dot( ray.direction ) ;
  
  if( den == 0 )
  {
    // RayParallelToPlane:
    
    // ray is orthogonal to plane normal ie
    // parallel to PLANE
    return false ;
  }
  
  real num = normal.Dot(ray.startPos) + d ;
  real t = - (num / den) ;

  if( t < 0 || t > ray.length )
  {
    // t < 0 : RayShootingAwayFromPlane:  complete miss, ray is in front of plane
    // t > ray.len : RayFallsShortOfPlane:  "out of reach" of ray.
    // there isn't an intersection here either
    return false ;
  }

  // Ray hit the plane.
  if( intn )  *intn = Intersection( ray.at( t ), normal, NULL ) ; // no shape
  
  return true ;
}