#ifndef MESH_H
#define MESH_H

#include "Triangle.h"
#include "Ray.h"
#include "Intersection.h"
#include "Adjacency.h"
#include "../scene/Material.h"

#include "../window/D3D11Window.h"
#include "../window/D3DDrawingVertex.h"
#include "../math/Vector.h"
#include "MeshGroup.h"

#include "../window/D3D11VB.h"

#include <string>
using namespace std ;

class VertexBuffer ;

enum MeshType
{
  Indexed,
  Nonindexed
} ;

extern char* MeshTypeName[] ;

typedef map< Triangle*,vector<Adjacency> >/* as simply */ AdjMap ;
typedef map< Triangle*,vector<Adjacency> >::iterator /* as simply */ AdjMapIter ;

struct HeaderMesh
{
  int meshType ;
  int vertexType ;
  int dataLen ;
  int numTris ;
  int numVerts ;
  int numIndices ;
  int shNumCoeffsPerVertex ;
} ;

extern VertexType defaultVertexType ;
extern MeshType defaultMeshType ;

struct Mesh
{
  Matrix world ;        // apply it's own matrix to vertices
  friend class Parser ; // the parser needs to know the meshType really.
  friend struct MeshGroup ; // meshgroups contain meshes, and should be able to access mesh's privates.
  Vector centroid ;     // cached centroid. Updated by mesh when transform is called.

  MeshType meshType ;
  VertexType vertexType ; // intended vertex type for this mesh, so

  VertexBuffer *vb ;
  
  vector<Triangle> tris ;               // Geometric representation of the mesh.
  vector<AllVertex> verts ;             // THE MASTER VERTEX ARRAY
  vector<unsigned int> faces ;          // the indices (visitation order) for drawing, active only IF you're using an index buffer
  // whether you're using a vertex buffer or not is an unchangeable property
  // of the Mesh SET AT CREATION TIME.  The behaviour of the public addTri()
  // function depends on the mode in which the Mesh was created.

  Shape *shape ;                  // a back reference to the Shape that owns this mesh.
                                  // A mesh can't exist on its own.. it must be inside a Shape
  AABB *aabb ; // each mesh can have its own AABB as well.

  // textures know their own slot,
  // a mesh may use any number of textures
  vector<D3D11Surface *> texes ;
  //!! should also accept vertex format

  Mesh( Shape *iShape, MeshType iMeshType, VertexType iVertexType ) ;

private: // prevent external copying of Meshes
  // This is to prevent accidental copy on pass-by-value
  // (which you should never do)
  // A MeshGroup can clone you, however
  Mesh( const Mesh& mesh ) ;
  Mesh( const Mesh* meshToClone, Shape* newOwner ) ;

public:
  
  Mesh( FILE* file, Shape* iShape ) ;
  int save( FILE *file ) ;
  ////Mesh& operator=( const Mesh& mesh ) ;
  
  ~Mesh() ;

public:
  // read-only
  MeshType getMeshType() { return meshType ; }

  int planeSide( const Plane& plane ) ;

  // Say you CHANGED the verts and normals but you
  // didn't update the tris collection.
  void updateTrisFromVertsAndFaces() ;

  // permanently deforms the mesh
  void transform( const Matrix& m, const Matrix& nT ) ;
  void addVertex( const AllVertex& v ) ;

  // this is the public interface function you use to add tris
  // when you don't want to do outside switching on the vertexformat
  void addTri( const Vector& A, const Vector& B, const Vector& C ) ;
  void addTri( const AllVertex& A, const AllVertex& B, const AllVertex& C ) ;  

  // Order: A,B,C,D must travel AROUND quad in a C-shape, CCW
  void addQuad( const Vector& A, const Vector& B, const Vector& C, const Vector& D ) ;
  void addTessQuad( const Vector& A, const Vector& B, const Vector& C, const Vector& D, int uPatches, int vPatches ) ;

  void addQuad( const AllVertex& A, const AllVertex& B, const AllVertex& C, const AllVertex& D ) ;
  void addTessQuad( const AllVertex& A, const AllVertex& B, const AllVertex& C, const AllVertex& D, int uPatches, int vPatches ) ;

  void setVertexBuffer( const vector<AllVertex>& sourceVerts ) ;
  
  // avoid the long vertex add times by manually indicating the
  // INDICES of the vertices you want to add.  Used for when
  // constructing triangles for an object from a vertex buffer YOU ALREADY HAVE SET
  void addTriByIndex( int ia, int ib, int ic ) ;


private:
  void addNonindexedTri( const Vector& A, const Vector& B, const Vector& C ) ;
  void addNonindexedTri( const AllVertex& A, const AllVertex& B, const AllVertex& C ) ;

  void addIndexedTri( const Vector& A, const Vector& B, const Vector& C ) ;
  void addIndexedTri( const AllVertex& A, const AllVertex& B, const AllVertex& C ) ;
 
public:
  // For meshes that you are LOADING from file, and you already know
  // the vertex/index buffer sharing strategy.
  void addIndexedTri( const AllVertex& A, const AllVertex& B, const AllVertex& C,
                      int iA, int iB, int iC ) ;

  // call this after you've added all your tris
  bool createVertexBuffer() ;

private:
  bool createVertexBufferVC() ;
  bool createVertexBufferVNC() ;
  bool createVertexBufferVTC() ;
  bool createVertexBufferVTTTTNC() ;
  bool createVertexBufferVTTTTNCCCC() ;
  bool createVertexBufferVT10NC10() ;

public:
  void updateVertexBuffer() ;

private:
  void updateVertexBufferVC() ;
  void updateVertexBufferVNC() ;
  void updateVertexBufferVTC() ;
  void updateVertexBufferVTTTTNC() ;
  void updateVertexBufferVTTTTNCCCC() ;
  void updateVertexBufferVT10NC10() ;
  
  // I call this when the mesh is changed.
  void computeCentroid() ;

public:
  void setColor( ColorIndex colorIndex, const Vector& newColor ) ;
  void setMaterial( const Material& iMaterial ) ;

  bool intersects( const Ray& ray ) ;
  bool intersects( const Ray& ray, MeshIntersection* intn ) ;
  bool intersectsFurthest( const Ray& ray, MeshIntersection* intn ) ;

  // averages the normals aka averagenormals
  void calculateNormals() ;

  // recursively tessellates tris in the mesh
  void tessellate( real maxArea ) ;

  void draw() ;

  void shLight( SHVector* shLight ) ;


  static void visualizeCreateMesh(
    // It has to have a shape to belong to
    Shape* shapeToAddMeshTo,

    int slices, int stacks,
    MeshType iMeshType, VertexType iVertexType,
    const Vector& center,
    function<Vector (real tElevation, real pAzimuth)> colorFunc,
    function<real (real tElevation, real pAzimuth)> magFunc
  ) ;

} ;

#endif