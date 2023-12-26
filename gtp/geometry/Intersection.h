#ifndef INTERSECTION_H
#define INTERSECTION_H

#include <vector>
#include <algorithm>
using namespace std ;
#include "../math/Vector.h"
#include "../window/D3DDrawingVertex.h"

struct Shape ;
struct Triangle ;
struct Mesh ;

// Used for general Ray-Shape intersection.
struct Intersection
{
  Vector point ;
  Vector normal ;  // The normals at the points of intersection
  // an Intersection may not intersect a Shape (it may be an AABB)
  Shape *shape ;
  
  Intersection() ;
  Intersection( const Vector& pt, const Vector& norm, Shape* iShape ) ;
  virtual ~Intersection() {}

  // This is a ptr because it'll need to work for the subclass
  bool isCloserThan( Intersection * intn, const Vector& toPoint ) ;

  real getDistanceTo( const Vector& toPoint ) ;

  // Gets you a color at the point of intersection.
  // DOESN'T work in N dot L product.
  virtual Vector getColor( ColorIndex colorIndex ) ;

  // the general way of telling if there was a hit or not
  // is if the shape is not NULL.  If the shape is NULL,
  // then clearly you did not hit anything.
  bool didHit() ;

  static Intersection HugeIntn ;
} ;

// Ray-Mesh intersection is Ray-Shape+extended info
// about exactly what triangle in the mesh was hit,
// and where on the triangle it was hit.
struct MeshIntersection : public Intersection
{
  Vector bary ;    // barycentric coordinates at point of intersection.
  Triangle *tri ;  // triangle object that was intersected in the mesh.
  
  MeshIntersection() ;
  MeshIntersection( const Vector& pt, const Vector& norm, const Vector& iBary, Triangle* iTri ) ;

  Mesh *getMesh() ;     // the mesh that was intersected (that owned the tri)

  // obviously these have to be INTERPOLATED.
  Vector getColor( ColorIndex colorIndex ) override ;
  Vector getTexcoord( TexcoordIndex texIndex ) ;

  real getAO() ; //interpolated

  // the normal is already interpolated.  see
  // interpNormal in Triangle::intersects
  ///Vector getInterpolatedNormal() ; // the normal

  static MeshIntersection HugeMeshIntn, SmallMeshIntn ;
} ;




#endif