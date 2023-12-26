#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <map>
#include <set>
#include <string>
using namespace std ;

#include "../geometry/Ray.h"
#include "../geometry/CubeMap.h"
#include "../math/ByteColor.h"
#include "../window/D3DDrawingVertex.h"
#include "Octree.h"

struct Shape ;
struct Intersection ;
struct Hemicube ;
struct MathematicalShape ;

struct LightSet
{
  // all the shapes that represent the lights
  vector<Shape*> lights ;

  // a single combine sh visualization
  Shape* shViz ;
} ;

class Scene
{
public:
  vector<Shape*> shapes ;  // all shapes
  vector<Shape*> lights ;  // Anything emissive.  Lights is all shapes that emit AND all far lights that are not shapes.
                           // REDUNDANT, contains objects that are lights
                           // Whitted ray tracer walks thru these
  vector<Shape*> farLights ; // the lights that are NOT shapes ONLY (collection: shapes-(lights in shapes))
                             // Used by VO because LIGHTS THAT MOVE SHOULDN'T BLOCK ANYTHING IN THE VO COMPUTE PASS
  vector<CubeMap*> cubeMaps ; // the collection of cubemaps
  int activeCubemap ; 

  // For rasterizing
  vector<Shape*> translucentShapes ;  // contains elements REDUNDANT to shapes
  vector<Shape*> opaqueShapes ;       // contains elements REDUNDANT to shapes

  PerlinGenerator perlin ;  // perlin sky generator
  bool perlinSkyOn ;

  MathematicalShape * brdfSHSpecular ; // the math shape that represents the brdf of the base specular material
  vector<Shape*> viz ;     // visualizations that get cleared with 'c'
  
  vector<Shape*> scenery ; // just visual aids, they are not
  // part of the scene really.  stuff that remains even after a clear, it just visual scenery
  // and doesn't REALLY participate directly in renders
  bool spacePartitioningOn ;

  // if newly added shapes are added as SCENE GEOMETRY
  // (reqd for raytracing, radiosity) or as just
  // point or infinitely distant lights (required for VO and SH)
  // (lights not intersectable part of scene -- ie they have no geometry)
  bool useFarLights ;
  
  HANDLE mutexVizList ;
  HANDLE mutexScenery ;
  HANDLE mutexShapeList ;
  HANDLE mutexDebugLines ;
  HANDLE mutexFCR ;
  
  Vector bgColor ;
  Vector mediaEta ; // base media eta of the scene

  CubicSpacePartition<Shape*> *spExact ;
  CubicSpacePartition<PhantomTriangle*> *spMesh ;
  CubicSpacePartition<PhantomTriangle*> *spAll ; 

  int lastVertexCount, lastFaceCount, lastMeshCount ;

  // Last used rotated shprojection
  // this is global because it is used on the raytracer
  SHVector* spRotated ;
  Model* shCombinedViz ;

  /// Creates the Scene object.
  Scene() ;

  /// Calls clearEntireScene()
  ~Scene() ;

  void addShape( Shape * s ) ;

  void addCubemap( CubeMap* cubemap ) ;
  
  void addVisualization( Shape *s ) ;
  
  int countFaces() ;

  // update the vertex count
  int countVertices() ;

  // how many mesh objects are there, for vo count worst case
  // vector storage
  int countMeshes() ;

  void computeSpacePartition() ;

  /// destroy all things in
  /// scene.
  void clearEntireScene() ;

  void createVertexBuffers() ;

  // post processing step on shlighting
  void shAddInterreflections() ;

  void waveletAddInterreflections() ;

  // smoothing operations
  void smoothNormals() ;

  SHVector* sumAllLights() ;

  // does ALL lights.
  // flag included in each light to EXCLUDE the light
  void shRotateAllLights( const Matrix& matrixRotation ) ;

  // lights by the isActive only
  //void shRotateLight( const Matrix& matrixRotation ) ;

  // bakes in the lighting into each shape as
  // diffuse color, using active light in SCENE object
  // MEANT TO BE CALLED EVERY FRAME
  // use the active light
  // (can also SUM active lights)
  void shLightCPU( const Matrix& matrixRotation ) ;

  void shLightGPU( const Matrix& matrixRotation ) ;

  void waveletLight() ;

  #pragma region geometry generation
  /// This is the only way in fact
  /// to gen/add vertices
  
  /// Generates 'num' tetrahedra
  /// with sizes between sizeMin and sizeMax,
  /// spatially within the AABB formed
  /// by min, max
  void generateRandomTetrahedra( int num, real sizeMin, real sizeMax, Vector min, Vector max ) ;

  /// Randomly creates 'num' tetrahedra that
  /// has an initial exitance for
  /// radiosity
  void generateRandomLightedTetrahedra( int num, real sizeMin, real sizeMax, Vector min, Vector max ) ;

  void generateRandomCubes( int num, real sizeMin, real sizeMax, Vector min, Vector max ) ;

  void generateRandomLightedCubes( int num, real sizeMin, real sizeMax, Vector min, Vector max ) ;
  #pragma endregion

public:

  // Check for closest intersection on the LIGHTS array,
  // does not use an octree (the array is expected to be small)
  static bool getClosestIntn( const Ray& ray, vector<Shape*>& collection, Intersection **closestIntersection ) ;

  // gets you the closest intersection
  // it will be exact if that's available,
  // or a meshintersection object if not.
  // REPLACES getClosestIntnExact()
  bool getClosestIntn( const Ray& ray, Intersection **closestIntersection ) const ;
  
  /// Raytracing: gets you the closest Shape object
  /// hit by ray, using an exact intersection
  /// where possible (purely triangular meshes
  /// obviously use ray-mesh intn)
  bool getClosestIntnExact( const Ray& ray, Intersection *closestIntersection ) const ;

  /// This function forces ray mesh intersections
  /// for EVERY object in the scene.  This is used
  /// for SH ray tracing, because sh needs to
  /// query vertices and doesn't handle implicit shapes.
  bool getClosestIntnMesh( const Ray& ray, MeshIntersection *closestIntersection ) const ;

  void getAllTrisUsingVertex( const Vector& v, set<Triangle*> tris ) ;

  int getNumTris() const ;

  // Runs a func on each SHAPE in the scene
  void eachShape( const function<void (Shape*shape)>& func ) ;

  // Runs a function on each TRI in the scene.
  void eachTri( function<void (Triangle*tri)> func ) ;

  // Runs a function on each VERTEX in the scene.
  void eachVert( function<void (AllVertex*vert)> func ) ;

  // TOO MODAL
  ///Shape* getActiveLight() ;
  CubeMap* getActiveCubemap() ;
} ;



#endif