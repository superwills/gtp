#ifndef MESHGROUP_H
#define MESHGROUP_H

#include <vector>
#include <functional>
using namespace std ;

union Vector ;
union Matrix ;

struct Mesh ;
struct Ray ;
struct MeshIntersection ;

enum MeshType ;
enum VertexType ;

struct HeaderMeshGroup
{ 
  int defMeshType ;
  int defVertexType ;
  int numMeshes ;
} ;

struct MeshGroup
{
  // a group of meshes.
  vector<Mesh*> meshes ;

  //bool dirty ; // adding a dirty bit would be nice,
  // so you don't have to recalculate the centroid
  // when not necessary.

  // This class is actually extraneous -- that's why I needed
  // a pointer to the containing class.
  // But still, collecting together MeshGroup functions
  // like this is handy.  Multiple inheritance would
  // actually help.
  
  //      
  //             ^   MeshGroup
  //              \     ^
  //                \   | // MUST in order to be rasterizable
  //                  Shape
  //                 /     \
  //              Sphere   Cube


  Shape *shape ; // owning shape. (It should know who it is!)

  // The DEFAULT meshtypes/vertex types for the addTri method.
  // this is NOT exclusive/these can be overridden if you access
  // the meshes member directly.
  int maxTrisPerMesh ;
  MeshType   defaultMeshType ;
  VertexType defaultVertexType ;

  MeshGroup( Shape* ownerShape, MeshType meshType, VertexType vertexType, int iMaxTrisPerMesh ) ;
  
  MeshGroup( FILE * file, Shape *iShape ) ;
  MeshGroup* clone( Shape* ownerToBe ) const ;
  int save( FILE * file ) ;

  ~MeshGroup() ;

  void each( function<void (Mesh*)> func ) ;

  // transforms each mesh then updates the vertex
  // buffer
  // deforms mesh
  void transform( const Matrix& m, const Matrix& nT ) ;

  // doesn't deform mesh
  void transformSetWorld( const Matrix& m ) ;

  void updateMesh() ;

  // execute some function on each mesh
  // that has a return type you care about
  // (returned in a vector of <T>)
  template <typename T> vector<T> each( function<T (Mesh*)> func )
  {
    vector<T> res ;
    for( int i = 0 ; i < meshes.size() ; i++ )
      res.push_back( func( meshes[i] ) ) ;
    return res ;
  }

  // Request progressively more information
  // (getting more expensive to compute each)
  bool intersects( const Ray& ray ) ;

  bool intersects( const Ray& ray, MeshIntersection* mintn ) ;

  bool intersectsFurthest( const Ray& ray, MeshIntersection* mintn );

  int planeSide( const Plane& plane ) ;

  bool isEmpty() const ;

  Vector getCentroid();

  // parameterize variable type, class type it belongs to.
  //<template typename TM,TC>
  //void smoothOperator( TM TC::* memberName ) ;

  // Accepts pointer to real valued member. I'm not sorry.
  void smoothOperator( real AllVertex::* memberName ) ;

  // Accepts pointer to vector member.
  void smoothOperator( Vector (AllVertex::* memberName)[10], int index ) ;

  // Smooths only ONE 
  void smoothOperator( Vector (AllVertex::* memberName)[10], int index, int componentIndex ) ;

  void smoothVectorOccludersData() ;

  // Updates the normals, but YOU MUST CALL UPDATEMESH() to make this show
  // DIFFERENT FROM SMOOTHOPERATOR!  Uses the
  // per-face normals and smooths them, NOT the
  // per-vertex normals that may already be there (those are ignored)
  void calculateNormalsSmoothedAcrossMeshes() ;

  int getTotalVertices() ;

  void addMesh( Mesh* iMesh ) ; 

  void setColor( ColorIndex colorIndex, const Vector& color ) ;
  void setMaterial( const Material& iMaterial ) ;
  void setOwner( Shape *newOwner ) ;

  // these push down into a Mesh.
  // when you call these 
  void addTri( const Vector& A, const Vector& B, const Vector& C ) ;
  void addTri( const AllVertex& A, const AllVertex& B, const AllVertex& C ) ;  

  void tessellate( real maxArea ) ;

  Vector getRandomPointFacing( const Vector& normal ) ;
} ;

#endif