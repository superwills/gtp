#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "../math/Vector.h"
#include "Plane.h"
#include "Shape.h"
#include "Ray.h"
#include "Adjacency.h"
#include "../window/D3DDrawingVertex.h"
#include <string>
using namespace std ;

struct Edge
{
  Vector a, b ;

  Edge( const Vector& ia, const Vector& ib ):a(ia),b(ib)
  {
  }

  bool Near( const Edge& o )
  {
    return ( a.Near( o.a ) && b.Near( o.b ) ) ||
           ( a.Near( o.b ) && b.Near( o.a ) ) ;
  }

  int sharing( const Edge& o )
  {
    int s = 0 ;
    if( a.Near( o.a ) || a.Near( o.b ) ) s++ ;
    if( b.Near( o.b ) || b.Near( o.a ) ) s++ ;
    return s ;
  }
} ;

/// Triangle as in the geometric object, purely.
struct Triangle : public Plane //, public Shape //I stopped Triangle from being a Shape to break the
// inheritance loop. Now you can make a single Triangle by using
// class Model, and just adding ONE tri.
{
  Vector a, b, c ;

  // Cached intersection data.
  Vector v0, v1 ;
  real d00,d01,d11,invDenomBary ;

  /// The vertex array indices of this Triangle
  /// in the MESH::verts[] ARRAY.
  int iA, iB, iC;    // If its not part of a vertex array yet.. then those indices are 0's.

  Mesh* meshOwner ;  // the tri should know the parent mesh (since indices above index an array in THERE)
  // The Mesh in turn knows the Shape.

  // 16M max anyway, so int goes to 2B
  int id ; // the id, assigned and indexed by RadCore
  /// Construct an unnamed triangle with no
  /// material or name.  Used in patch
  /// creation..
  Triangle( const Vector& pa, const Vector& pb, const Vector& pc, Mesh*iMeshOwner );

  Triangle( const Vector& pa, const Vector& pb, const Vector& pc, int ia, int ib, int ic, Mesh*iMeshOwner );

  Vector getRandomWithin() {
    Vector bary = Vector::randBary() ;
    return a*bary.x + b*bary.y + c*bary.z ;
  }

  void precomputeBary() ;

  ///Triangle( FILE *file, Mesh* iMeshOwner ) ; // this can't exist because the Plane class needs the verts already
  void load( FILE *file ) ;

  void setColor( ColorIndex colorIndex, const Vector& color ) ;

  // sets id of mesh and underlying vertices as well
  void setId( int iid ) ;

  // Turns off the alpha bits.
  // Really this could be reversed (id could have stored with alpha bits OFF)
  inline int getId() const { return id & ~ALPHA_FULL ; }

  // FOR RADIOSITY
  Vector getAverageColor( ColorIndex what ) ;

  Vector getAverageTexcoord( TexcoordIndex what ) ;

  // a bit of an optimization avoid adding
  // components you don't need
  real getAverageColor( ColorIndex what, int channelIndex ) ;

  real getAverageTexcoord( TexcoordIndex what, int channelIndex ) ;

  real getAverageAO() ;
  
  Vector getBarycentricCoordinatesAt( const Vector & P ) const;

  Vector getRandomPointOn() const ;

  bool isDegenerate() const ;

  static bool isDegenerate( const Vector& A, const Vector& B, const Vector& C ) ;

  /// Computes and returns the
  /// area of the triangle.
  real area() const;

  int planeSide( const Plane& plane ) ;

  void refresh() ;

  void transform( const Matrix& m );

  // If you don't want the MeshIntersection information, pass NULL.
  bool intersects( const Ray& ray, MeshIntersection* intn ) ;

  // tells you if this triangle intersects an AABB
  // extremely complicated
  //bool intersects( const AABB& aabb ) ;

  // More of a reminder that you can easily tell if a triangle belongs to
  // a specific Shape or not.
  bool belongsTo( const Shape* shape ) ;

  // mandatory to be able to give the centroid
  Vector getCentroid() const ;



  // recursively subdivide a triangle, until
  // maxDepth is reached, or the triangles
  // being divided have areas LESS THAN maxArea allowed.
  // New triangles are appended to triList, which
  // you need to pass on the first call to this function.
  void subdivide( int depth, int maxDepth, real maxArea, list<Triangle>& triList ) ;

  // you have to have somewhere to put them.
  // this version of the subdivide method
  // puts triangles into the mesh as they are created.
  // this avoids creation/insertion into some sort of intermediate list.
  void recursivelySubdivideAndAddToMeshWhenSmallEnough( int depth, int maxDepth, real maxArea, Mesh* mesh ) ;

  // recursively subdivides series of points,
  // and pushes every point out to fit
  // on a sphere of radius r.
  void recursivelySubdivideAndPushToSphereThenAddToMeshWhenDepthReachedOrSmallEnough( int depth, int maxDepth, real maxArea, real r, Mesh* mesh ) ;

  bool isAdjacent( Triangle& other, Adjacency *adj ) ;

  bool usesVertex( const Vector& v ) ;

  Edge ab() { return Edge( a, b ) ; }
  Edge bc() { return Edge( b, c ) ; }
  Edge ac() { return Edge( a, c ) ; }

  inline real lenAB() const { return (b-a).len() ; }
  inline real lenBC() const { return (c-b).len() ; }
  inline real lenAC() const { return (c-a).len() ; }

  AllVertex* vA() ;
  AllVertex* vB() ;
  AllVertex* vC() ;
} ;

// clipboard inheritance Triangle class,
// supports intersection, and contains a
// reference to the original (full size) triangle
struct PhantomTriangle : public Plane
{
  Vector a,b,c ;
  Triangle* tri ;

  PhantomTriangle( Triangle& iTri ) ;

  // copy ctor based on ptr
  PhantomTriangle( PhantomTriangle* iPhantom ) ;
  
  PhantomTriangle( const Vector& pa, const Vector& pb, const Vector& pc, Triangle* origTri ) ;

  inline real lenAB() const { return (b-a).len() ; }
  inline real lenBC() const { return (c-b).len() ; }
  inline real lenAC() const { return (c-a).len() ; }

  //bool hasLongerEdgesThanOwner() const ;

  bool isDegenerate() const ;

  real area() const ;

  // It's out of the shape hierarchy, but for
  // templated duck typing it provides planeSide.
  // (kdTree wants this)
  int planeSide( const Plane& plane ) ;
} ;

#endif