#ifndef AABB_H
#define AABB_H

#include "../window/D3DDrawingVertex.h"
#include "../math/Vector.h"
#include "Triangle.h"
#include "Ray.h"


struct Triangle ;
struct Shape ;
struct Intersection ;
class Scene ;

struct AABB
{
  #define HUGE 1e30f
  Vector min, max ;
  // NOT caching 'extents' because it's not used a lot
  // and it's like more memory for no reason.

  AABB() ;
  AABB( const Vector& iMin, const Vector& iMax ) ;
  AABB( const Scene * scene ) ;
  AABB( const Shape * shape ) ;
  
  void resetInsideOut() ;
  
  // Extents and volume measure
  inline real xExtents() { return max.x - min.x ; }
  inline real yExtents() { return max.y - min.y ; }
  inline real zExtents() { return max.z - min.z ; }
  inline real extents( int axisIndex ) { return max.e[axisIndex] - min.e[axisIndex] ; }
  inline Vector extents() { return max - min ; }
  // Gives you the volume of an AABB.  This is 0
  // if the AABB is a point, or if the aabb is
  // infinitely thin in any direction
  real volume() { return xExtents()*yExtents()*zExtents(); }

  // ie the aabb is a point, not a box anymore.
  inline bool isZeroVolume() { return max==min ; }

  // Bounding
  void bound( const Scene * scene ) ;
  void bound( const Shape * shape ) ;
  void bound( const Triangle* tri ) ;
  void bound( const PhantomTriangle* tri ) ;

  // Intersection methods:
  bool intersects( const AABB& o ) const ;
  
  // no one needs teh intersection point for the aabb,
  // it's just a boolean check
  //bool intersects( const Ray& ray, Intersection* intn ) ;
  bool intersects( const Ray& ray ) ;

  bool intersects( const Ray& ray, Vector& pt ) ;

  // Gives you a new AABB that describes where
  // one AABB intersects another.  You get
  // an EMPTY (point) AABB if they don't intersect
  // at all (max=min=0,0,0)
  AABB getIntersectionVolume( const AABB& o ) const ;
  
  #pragma region contains inclusive
  // point/tri/PhantomTri/AABB is
  // inside or on edges of aabb.
  
  // named contains because that's what it really is.
  // you can "intersect" a point, but its better to say
  // the AABB CONTAINS it.
  inline bool containsIn( const Vector& point ) const {
    // the point is in the box of space between min and max
    return point.betweenIn( min, max ) ;
  }

  // Unfortunately exists due to octree's genericness
  // (calls on POINTERS to both triangle and Shape*.)
  // Shape*'s cannot be dereferenced, since Shape is abstract,
  // so you cannot pass (*object) all the time.
  inline bool containsIn( const Triangle* tri ) const {
    return containsIn( *tri ) ;
  }
  
  // Here we return true IFF the tri's 3 pts
  // are completely contained within the AABB
  inline bool containsIn( const Triangle& tri ) const {
    return tri.a.betweenIn( min, max ) &&
           tri.b.betweenIn( min, max ) &&
           tri.c.betweenIn( min, max ) ;
  }

  inline bool containsIn( PhantomTriangle* tri ) const {
    return tri->a.betweenIn( min, max ) &&
           tri->b.betweenIn( min, max ) &&
           tri->c.betweenIn( min, max ) ;
  }

  bool containsIn( const AABB& oaabb ) const
  {
    return oaabb.min.betweenIn( min, max ) && 
           oaabb.max.betweenIn( min, max ) ;
  }

  bool containsIn( const Shape* shape ) const ;
  #pragma endregion

  // Phantom triangle is STRICTLY INSIDE
  // the aabb
  inline bool containsEx( PhantomTriangle* tri ) const {
    return tri->a.betweenEx( min, max ) &&
           tri->b.betweenEx( min, max ) &&
           tri->c.betweenEx( min, max ) ;
  }

  // BAD, DO NOT USE
  // "Almost" containment doesn't cut it for sending a triangle to
  // a subdivision node -- it should be exactly
  // contained by floating point error, or leave it in the root node then.
  // The only problem is what will happen if a cut polygons won't go
  // into the nodes they were cut for?? TEST THIS.
  inline bool containsAlmost( PhantomTriangle* tri ) const {
    // instead of being exact, use an approximation
    return tri->a.betweenNearly( min, max ) &&
           tri->b.betweenNearly( min, max ) &&
           tri->c.betweenNearly( min, max ) ;
  }
  
  inline bool containsAlmost( const Vector& pt ) const {
    return pt.betweenNearly( min, max ) ;
  }

  // splits on a given axis into 2 aabb's,
  // used in kd-tree construction
  // Push 0 for x, 1 for y, 2 for z.
  vector<AABB> split2( int axisIndex, real val ) ;
  
  // splits into 8 AABBs,
  // used mainly for octree construction
  vector<AABB> split8() ;

  vector<Vector> getCorners() const ;

  bool sameSide( const vector<Vector>& v ) const ;

  // don't use an offset
  void generateDebugLines( const Vector& color ) const ;

  // A convenience method to get the lines
  void generateDebugLines( const Vector& offset, const Vector& color ) const ;

  Vector random() const ;

  // point in the middle of box
  inline Vector mid() { return ( min + max ) / 2 ; }
} ;

#endif