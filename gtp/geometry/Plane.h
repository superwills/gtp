#ifndef PLANE_H
#define PLANE_H

#include "../math/Vector.h"

struct Intersection ;
struct Ray ;

enum PlaneSide
{
  // You are definitely behind the plane
  Behind     = -1,

  // You are IN the plane (single point), or
  // STRADDLING the plane (multiple points)
  Straddling =  0,

  // You are definitely in front of the plane.
  InFront    =  1
} ;

struct Plane
{
  // the plane normal
  Vector normal ;

  // distance from the origin
  real d ;

  Plane( const Vector& iNormal, real iD )
  {
    normal = iNormal ;
    d = -iD ;
  }

  // 0 for 'x', 1 for 'y', 2 for 'z'
  Plane( int axisIndex, real perpendicularDistance )
  {
    d = -perpendicularDistance ;
    normal.e[ axisIndex ] = 1 ;
  }

  // WIND THE TRIANGLE CCW!
  Plane( const Vector& A, const Vector& B, const Vector& C )
  {
    recomputePlane(A,B,C);
  }

  void recomputePlane( const Vector& A, const Vector& B, const Vector& C )
  {
    // the normal is just the cross of 
    // 2 vectors in the plane, normalized
    Vector BA = A - B ;
    Vector BC = C - B ;

    normal = BC << BA ;

    if( normal.len2() == 0 )
    {
      //error( "Triangle has 3 vertices that are collinear, (ie two vertices may be the same) the normal is 0" ) ;
      d=0;// zero normal means d=0 as well
      return ;
      //__debugbreak() ;
    }
    
    // now normalize this.
    normal.normalize() ;

    // http://en.wikipedia.org/wiki/Line-plane_intersection
    // use the normal to compute the d
    d = - normal.Dot( A ) ;
  }

  // Returns true if the 3 points are on the same LINE
  static bool areCollinear( const Vector& A, const Vector& B, const Vector& C )
  {
    Plane p( A,B,C ) ;
    if( p.normal.len2() == 0 )
      return true ;
    else
      return false ;
  }

  // classifies what side of the plane / triangle you are on
  // (+side, 0 on plane, -side)
  // ax + by + cz + d = { 0  point on plane
  //                    { +  point "in front" of plane, (ie on side of normal)
  //                    { -  point "behind" plane (ie on side that normal is NOT on).
  //inline real side( const Vector& p ) const {
  //  return (normal % p) + d ;
  //}

  // Could experiment with thickness and see if it
  // makes this better, for whatever reason.
  inline PlaneSide iSide( const Vector& p ) const
  {
    real val = (normal % p) + d ;
    
    if( IsNear( val, 0.0, EPS_MIN ) )  return PlaneSide::Straddling ;
    else if( val > 0 ) return PlaneSide::InFront ;
    else return PlaneSide::Behind ;
  }

  // TRUE if all points on same side,
  // FALSE if at least one point on a different side
  bool sameSide( const vector<Vector>& pts )
  {
    int sign = iSide( pts[0] ) ; 
    for( int i = 1 ; i < pts.size() ; i++ )
      if( sign != iSide( pts[i] ) )
        return false ;
    return true ;
  }

  // tells you if the ray intersects the plane, and where.
  // this is named differently than the one in class Triangle,
  // because you basically only use this in poly splitting,
  // and rarely actually want to call this function.
  // using inheritance here would make Triangle intersection a virtual function,
  // which is unnecessary.
  bool intersectsPlane( const Ray& ray, Intersection* intn ) ;
} ;

#endif