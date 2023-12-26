#ifndef SHAPE_H
#define SHAPE_H

// shapes are defined as 
// mathematical objects,
// not polysoup.

// you can sample a shape/obtain
// a mesh by calling .getMesh() on
// any shape.

// you do not have to know what kind
// of shape you are working with to get mesh.

#include "../math/Vector.h"
#include "Intersection.h"
#include "Ray.h"
#include "../scene/Material.h"
#include <functional>
using namespace std ;

struct Mesh ;
struct MeshGroup ;
struct AABB ;

class SHVector ;
class SH ;
class Scene ;
struct Callback ;
class ParallelBatch ;
struct D3D11Surface ;
struct PerlinGenerator ;
struct Plane ;

// you have to label what each Shape is
// in order to be able to restore it from file
enum ShapeType
{
  SphereShape,
  CubeShape,
  TetrahedronShape,
  
  ModelShape,
  
  // a shape defined by an arbitrary mathematical function
  // Naming conflict with "MathematicalShape".
  MathematicalShapeType,

  // this will have been a Cubemap, actually,
  // which is one level down from a Model
  CubemapShapeLight
} ;

struct HeaderShape
{
  int ShapeType ; // concrete type the shape is
  ////int dataLen ;   // length of data block until next shape
};

// Intersectable shape interface
struct Shape abstract 
{
  // CONSIDER ELIMINATING VIRTUAL FUNCTIONS
  //           shapetype
  //           0000
  // Sphere,Cube,Model..
  // let the msb denote whether to use a MATH intersection
  // or a whether a mesh-based intersection must be used

  // every shape has a name
  string name ;

  // brdf would go here
  Material material ;

  // mostly not used.
  AABB* aabb ;
  
  // A motion function -- generates a displacement for 't'
  // for probably the CENTROID of a shape (moves it rigidly
  // and without rotation)
  function<Vector (real t, const Vector& vert)> shaderMotionFunction ;
  static function<Vector (real t, const Vector& vert)> ERR_MOTION ;

  // the polygonal mesh repn
  MeshGroup* meshGroup ; // A Shape is allowed multiple meshes.

  bool hasMath ; // this Shape has an exact boolean intersection math equation
  bool isCubeMap ; // I need an easy way to tell if this shape
  // is actually a CubeMap down the hierarchy.  A simple boolean flag
  // is the easiest and most efficient way to do this, rather than
  // do RTTI (attempt to dynamic_cast<CubeMap*>, if that fails (you get a NULL)
  // then it WASN'T a CubeMap)
  //!! YOU SHOULD HAVE USED SHAPETYPE (BELOW)!!
  // Used for file saving. This member ISN'T required, we could just use
  // dynamic_cast<> attempts to determine the shape type.
  // but as # shape types grows, this will make file save possibly faster.
  // plus if CubeMap : Sphere : Shape, then we'd have to make sure
  // to try to cast to CubeMap BEFORE Sphere, because the cast to
  // Sphere will succeed as well.
  ShapeType shapeType ;

  // Flag to indicate whether the object is
  // ON as a light or not
  bool isActive ;

  // The shProjection of this shape. not always initialized or used.
  SHVector *shProjection ;
  Shape *shProjectionViz ;

  // a constant which represents the
  // directionality of the emissive component.
  // tends to direct light in direction of __NORMAL__
  // to emitting surface. A LASER is purely directional (diffusion=0),
  // and a fogged incandescent bulb is purely diffusional (diffusion=1).
  // simple multiplication by cos( diffusion ) would
  // scale magnitude nicely (perfect diffuse has no
  // attenuation from any angle.)
  ////real emissiveDiffusion ; // this really has to do with SPECULARITY..
  // perhaps it should be related to KS

  Shape() ;
  Shape( FILE* file ); // load from file
  
  // loads a shape described in HEAD from FILE.
  static Shape* load( const HeaderShape& head, FILE * f ) ;

  // write me to the file handle.
  virtual int save( FILE* file ) ;

  virtual ~Shape() ;

  // SHprojects THE SHAPE ITSELF.  This is for use
  // when a SHAPE IS A LIGHT.
  // This DOES __NOT__ project the
  // light function at each vertex of the shape.
  // that is Mesh::shProject.
  // This function is overridden by 
  // MathematicalShape::shProject, CubeMap::shProject
  virtual SHVector* shProject( const Vector& scale ) ;

  // Generate a vis of the shprojection
  void shGenerateViz( int slices, int stacks, const Vector& pos ) ;

  virtual Shape* transform( const Matrix& m ) ;

  // Override to move your local points, like
  // the center point for a sphere
  virtual Shape* transform( const Matrix& m, const Matrix& nT ) ;

  Shape* randomPosAndRot( const AABB& bounds ) ;

  Shape* tessellate( real tess ) ;

  // A CONVENIENCE FUNCTION that tells you
  // if any of the vertices in the shape's mesh
  // have a ke component.
  // overridden by MathematicalShape (emissive function set or not)
  // and CubeMapShape (ALWAYS return true)
  virtual bool isEmissive() ;

  // does this shape transmit light?
  virtual bool isTransmitting() ;

  /// See if this object intersects
  /// with a ray.
  virtual bool intersectExact( const Ray& ray, Intersection* intn ) ;

  /// See if the object intersects with a ray
  /// and get me the color at the point of intersection
  virtual bool intersectExact( const Ray& ray, Intersection* intn, ColorIndex colorIndex, Vector& colorOut ) ;

  // Intersects the mesh:  AABB: if hit, intersect against all tris.
  // Got a better way to do it? Override this function!
  // (There is a better way: use the tri collection returned by the octree)
  virtual bool intersectMesh( const Ray& ray, MeshIntersection* meshIntn ) ;

  // Used when getting OUTER radial distance (hardly ever used)
  virtual bool intersectMeshFurthest( const Ray& ray, MeshIntersection* meshIntn ) ;

  // 
  virtual int planeSide( const Plane& plane ) ;

  // give the centroid, assumed to be the
  // average of all the points in the mesh
  virtual Vector getCentroid() const ;

  virtual Vector getRandomPointFacing( const Vector& dir ) ;



  /// BE AWARE GETTING A COLOR THIS WAY
  /// INVOLVES A RAY INTERSECTION WITH THE SHAPE.
  /// IF YOU'VE ALREADY INTERSECTED WITH THE SHAPE,
  /// YOU SHOULD RETRIEVE THE COLOR FROM THE INTERSECTION OBJECT
  
  // Mathematical Shape
  virtual Vector getColor( const Ray& ray, ColorIndex colorIndex ) ;

  // Mathematical Shape overrides
  virtual Vector getColor( const Vector& pointOnShape, ColorIndex colorIndex ) ;
  
  // Mathematical Shape ignores startPt.
  virtual Vector getColor( const Vector& startPt, const Vector& pointOnShape, ColorIndex colorIndex ) ;

  // Mathematical Shape overrides this too
  virtual Vector getColor( real tElevation, real pAzimuth, ColorIndex colorIndex ) ;
  
  // Evaluating a SHAPE at tElevation, pAzimuth
  // means shooting a ray with that angle and seeing
  // where it intersects the shape.
  // for a sphere, this is trivial (you get back r),
  // but for an arbitrary mesh, it is a ray-mesh intersection.
  // fromCentroid:  Do you want the distance from the CENTROID of the shape
  // to the edge of the mesh, or FROM THE ORIGIN to the edge of the mesh?
  // outerShell:  Do you want the distance to the FIRST INTERSECTION (formerly "inner radial distance")
  // or the FURTHEST intersection (outerShell)?
  virtual real getRadialDistance( real tEl, real pAz, bool fromCentroid, bool outerShell ) ;

  // This really should be SETMATERIAL
  Shape* setColor( ColorIndex colorIndex, const Vector& color ) ;
  Shape* setMaterial( const Material& iMaterial ) ;

  // makes ready for scene insertion
  bool makeReadyForScene() ;
  bool isVBsReady() ;

  // flush changes to vertices down to GPU
  Shape* updateMesh() ;
  Mesh* getMesh( int meshNo ) ;
  void setAddTexture( D3D11Surface * tex ) ;
  void setOnlyTextureTo( D3D11Surface * tex ) ;

  // Diffuse + Specular SH CPU computation
  void shLight( SHVector* shLightSource ) ;
  
  // draws the SH functions at each vertex. "Messy".
  void shDrawVertexVisualizations() ;

  virtual void draw() ;

  void addMesh( Mesh*mesh ) ;

  // unlink meshes from other group
  // and give them to me
  void takeMeshesFrom( Shape * o ) ;  //transferMeshes( Shape * o ) ;

  //void cloneMeshesFrom( Shape * o ) ;

  /// Given a normal POINTING TOWARDS ME,
  /// I want a POINT __on__ SHAPE
  /// that is ON A FACE WHICH IS FACING TOWARDS
  /// 'normal'.
  /// What would be EVEN BETTER is if I weighed
  /// probability of the point you get on the shape
  /// by the SURFACE AREA OF EACH FACE RELATIVE TO THE SENDING POINT,
  ////virtual Vector getRandomPointOnShapeFacing( const Vector& normal ) ;

  // other ideas are: get a hull (2d poly) of the outermost points (OUTLINE)
  // AFTER PROJECTION OF THE LIGHT SOURCE FROM MY
  // VANTAGE POINT (ie facing "me") (the sender) and choose a point in that 2d polygon.
  // 
} ;

#endif

