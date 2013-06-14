#include "Shape.h"
#include "AABB.h"
#include "Mesh.h"
#include "Sphere.h"
#include "Cube.h"
#include "Tetrahedron.h"
#include "Model.h"
#include "MathematicalShape.h"
#include "../math/SH.h"
#include "../math/SHSample.h"
#include "../math/SHVector.h"
#include "../math/perlin.h"
#include "../window/Camera.h"
#include "../window/GTPWindow.h"
#include "../Globals.h"
#include "../threading/ParallelizableBatch.h"

function<Vector (real t, const Vector& vert)> Shape::ERR_MOTION = [](real t, const Vector& vert)->Vector
{
  // to indicate error
  error( "Err motion function called -- you have not specified a motion function for one of your objects" ) ;
  //return Vector(randFloat(),randFloat(),randFloat()) ;
  return Vector(0,0,0,-1) ;
} ;


Shape::Shape():aabb(0), hasMath(0), isCubeMap(0), shProjection(0), shProjectionViz(0), meshGroup(0), isActive(1)
{
  shaderMotionFunction=ERR_MOTION ;
}

Shape::Shape( FILE* file )
{
  hasMath=false ;
  isCubeMap = false ;
  cStrRead( name, file, MAX_PATH ) ;
  material = Material( file ) ;// load the material from file
  meshGroup = new MeshGroup( file, this ) ;
  shProjection=0;
  shProjectionViz=0;
  aabb = new AABB( this ) ;
}

Shape* Shape::load( const HeaderShape& head, FILE * f )
{
  Shape *shape ;
  switch( head.ShapeType )
  {
    case ShapeType::CubeShape:
    shape = new Cube( f );
    break;

    case ShapeType::ModelShape:
    shape = new Model( f );
    break;

    case ShapeType::SphereShape:
    shape = new Sphere( f );
    break;

    case ShapeType::TetrahedronShape:
    shape = new Tet( f );
    break;

    default:
    error( "INVALID SHAPE TYPE %d", head.ShapeType ) ;
    break;
  }

  return shape ;
}

int Shape::save( FILE* file )
{
  // base saves material, verts
  int written = 0 ;
  
  cStrWrite( file, name.c_str(), MAX_PATH ) ;
  written += material.save( file ) ;
  written += meshGroup->save( file ) ;
  
  return written ;
}

Shape::~Shape()
{
  info( "Shape `%s` is dying", name.c_str() ) ;
  DESTROY( aabb ) ;
  DESTROY( meshGroup ) ;

  // the shProjection isn't destroyed
  // because it may be used by something else.
}

SHVector* Shape::shProject( const Vector& scale )
{
  info( "SH projecting the shape of %s", name.c_str() ) ;
  // shprojection for a shape is done by
  // integrating the samples against
  // the SHAPE, using radial distances.
  shProjection = new SHVector() ; 
  for( int i = 0 ; i < window->shSamps->nU ; i++ )
  {
    SHSample &samp = window->shSamps->samps[ i ] ;
    
    // correlate.
    real dist = getRadialDistance( samp.tEl(), samp.pAz(), false, true ) ;
    shProjection->integrateSample( samp, dist, scale ) ;

    printf( "proj %d / %d           \r", i+1, window->shSamps->nU ) ;
  }

  shProjection->scale( window->shSamps->dotScaleFactor ) ;

  // It's possible that if there weren't enough samples,
  // the light will be a 0 light
  if( shProjection->isZero() )
  {
    warning( "%s was a 0 sh-projection", name.c_str() ) ;
    shProjection->coeffsScale[0] = Vector(1,1,1) ; // make it a 1, so you don't get a 0 vector
  }

  return shProjection ;
}

void Shape::shGenerateViz( int slices, int stacks, const Vector& pos )
{
  if( shProjection )
    shProjectionViz = shProjection->generateVisualization( slices, stacks, pos ) ;
}

Shape* Shape::transform( const Matrix& m )
{
  // Convenience.
  return transform( m, m.getNormalMatrix() ) ;
}

Shape* Shape::transform( const Matrix& m, const Matrix& nT )
{
  if( !meshGroup ){
    warning( "Trying to transform shape %s that doesn't have a mesh yet, nothin' doin'", name.c_str() );
    return this ;
  }
  
  meshGroup->transform( m, nT ) ;

  // now you need to recalculate the aabb
  if( aabb )  aabb->bound( this ) ;

  return this ;
}

Shape* Shape::randomPosAndRot( const AABB& bounds )
{
  Matrix xform = Matrix::Translate( bounds.random() ) * Matrix::RotationYawPitchRoll( randFloat(0,2*PI),randFloat(0,2*PI),randFloat(0,2*PI) ) ;
  transform( xform ) ;
  return this ;
}

Shape* Shape::tessellate( real tess )
{
  meshGroup->tessellate( tess ) ;
  return this ;
}

bool Shape::isEmissive()
{
  // it's really this simple.  if the
  // base material of the shape is
  // larger than 0, then the shape emits light.
  return material.ke.nonzero() ;
}

bool Shape::isTransmitting()
{
  return material.kt.nonzero() ;
}

bool Shape::intersectExact( const Ray& ray, Intersection* intn )
{
  if( hasMath )
    error( "You claimed to provide a perfect math intersection equation but intersectsExact fell into the base class" ) ;

  return false ;
}

bool Shape::intersectExact( const Ray& ray, Intersection* intn, ColorIndex colorIndex, Vector& colorOut )
{
  if( hasMath )
    error( "You claimed to provide a perfect math intersection equation but intersectsExact fell into the base class" ) ;

  return false ;
}

bool Shape::intersectMesh( const Ray& ray, MeshIntersection* meshIntn )
{
  return meshGroup->intersects( ray, meshIntn ) ;
}

bool Shape::intersectMeshFurthest( const Ray& ray, MeshIntersection* meshIntn )
{
  return meshGroup->intersectsFurthest( ray, meshIntn ) ;
}

// 0 : on plane (completely coincident -- only possible for planar shapes)
// 1 : IN FRONT of plane ( on side of normal )
//-1 : BEHIND plane
int Shape::planeSide( const Plane& plane )
{
  return meshGroup->planeSide( plane ) ;
}

Vector Shape::getCentroid() const
{
  if( meshGroup )
    return meshGroup->getCentroid() ;
  else
    return 0 ;
}

Vector Shape::getRandomPointFacing( const Vector& dir )
{
  // the base implementation of this function uses the mesh directly
  return meshGroup->getRandomPointFacing( dir ) ;
}



Vector Shape::getColor( const Ray& ray, ColorIndex colorIndex )
{
  // shoot a the ray to the mesh, and see what you hit
  MeshIntersection mi ;
  if( intersectMesh( ray, &mi ) )
    return mi.getColor( colorIndex ) ;
  else // you didn't hit the mesh.
    return 0 ;
}

// for when a Shape is used as a light source
Vector Shape::getColor( const Vector& pointOnShape, ColorIndex colorIndex )
{
  return getColor( getCentroid(), pointOnShape, colorIndex ) ;
}

// Mathematical Shape ignores startPt.
Vector Shape::getColor( const Vector& startPt, const Vector& pointOnShape, ColorIndex colorIndex )
{
  // the direction of the ray is implied by pointOnSurface,
  Ray ray( startPt, (pointOnShape - startPt).normalize(), 1000.0 ) ;
  return getColor( ray, colorIndex ) ;
}

// Now here, there's a whole variety of colors
// you might get.
Vector Shape::getColor( real tElevation, real pAzimuth, ColorIndex colorIndex )
{
  Ray ray( getCentroid(), SVector( 1, tElevation, pAzimuth ).toCartesian(), 500000 ) ;
  return getColor( ray, colorIndex ) ;
}

/// There never is an occassion to use this method
/*
// If you want ALL the colors, use this method
vector<Vector> Shape::getColors( const Vector& startPt, const Vector& direction, ColorIndex colorIndex )
{
  vector<Vector> colors ;

  // the direction of the ray is implied by pointOnSurface,
  Ray ray( startPt, direction, 1000 ) ;
  
  // shoot a the ray to the mesh, and see what you hit
  MeshIntersection mi ;
  
  while( intersectMesh( ray, &mi ) )
  {
    colors.push_back( mi.getInterpolatedColor( colorIndex ) ) ;

    // move the start point UP to the intersection point
    // then nudge a bit past the intersected surface,
    // in the direction of the ray
    ray.startPos = mi.point + ray.direction*EPS_MIN ;
  }

  return colors ; // if there were no intersections the vector is empty
}
*/

real Shape::getRadialDistance( real tEl, real pAz, bool fromCentroid, bool outerShell )
{
  if( !meshGroup ){
    error( "No meshgroup" ) ;
    return 0 ;
  }
  
  // shoot a ray from the centroid to the mesh
  // in the direction of tElevation, pAzimuth.
  // If the shape is not convex, you're going to
  // get a really bad approximation.
  
  Vector center(0,0,0);  // SHOOT FROM ORIGIN
  if( fromCentroid )  center = getCentroid() ;   // SHOOT FROM CENTROID

  SVector v( 1, tEl, pAz ) ;
  Ray ray( center, v.toCartesian(), 1000.0 ) ;

  // shoot the ray mesh-based intersection.
  // If the model has holes, you may hit nothing
  MeshIntersection mi ;

  // fcn ptr to intn fcn to use
  //int (CLASSNAME::* __VARIABLE_NAME__ )( int,int,int ) ;

  bool (Shape::* intnFn)( const Ray&, MeshIntersection* ) = &Shape::intersectMesh ;
  if( outerShell )  intnFn = &Shape::intersectMeshFurthest ; // You actually need THE FURTHEST point of intersection, not the closest.

  if( (this->*intnFn)( ray, &mi ) )
  {
    // we really want the distance from
    // center being used to pt of intn
    //window->addDebugLine( center, Vector(1,0,0), mi.point, 1 ) ;
    return (mi.point - center).len() ; // vector that goes center->SURFACE
  }
  else
    return 0 ; // Hmm.  That's kind of confusing, but 0 distance means NO HIT, not "it's right in front of you"
}

Shape* Shape::setColor( ColorIndex colorIndex, const Vector& color )
{
  switch( colorIndex )
  {
    case ColorIndex::DiffuseMaterial:
      material.kd = color ;
      break ;

    case ColorIndex::Emissive:
      material.ke = color ;
      break ;

    default:
      warning( "Didn't change material when setColor( %s ) called", ColorIndexName[ colorIndex ] ) ;
      break ;
  }
  
  if( meshGroup )  meshGroup->setColor( colorIndex, color ) ;
  return this ;
}

Shape* Shape::setMaterial( const Material& iMaterial )
{
  material = iMaterial ;
  if( meshGroup )  meshGroup->setMaterial( material ) ;
  return this ;
}

Shape* Shape::updateMesh()
{
  if( !meshGroup ){ error("%s has no meshgroup to update", name.c_str() ) ; return this ; }
  meshGroup->updateMesh() ;
  return this ;
}

Mesh* Shape::getMesh( int meshNo )
{
  if( meshNo < meshGroup->meshes.size() )
    return meshGroup->meshes[ meshNo ] ;
  else
  {
    error( "Mesh out of bounds" ) ;
    return NULL ;
  }
}

bool Shape::makeReadyForScene()
{
  // if the shape has no mesh, reject it (I can't draw it!)
  if( !meshGroup )
  {
    error( "%s has no mesh, nothin' addin'", name.c_str() ) ;
    return false ;
  }

  // on adding a shape to the scene make sure its bounding box is calc
  if( !aabb )  aabb=new AABB( this ) ;

  // if the shape is going to be drawn, it best better have a vertex buffer ready!
  if( !isVBsReady() ) updateMesh() ; // creates vbs if they don't exist

  return true ;
}

bool Shape::isVBsReady()
{
  for( int i = 0 ; i < meshGroup->meshes.size() ; i++ )
    if( !meshGroup->meshes[i]->vb ) return false ;
  return true;
}

void Shape::setAddTexture( D3D11Surface * tex )
{
  // each mesh in the meshgroup gets this tex pushed back.
  this->meshGroup->each( [tex](Mesh*mesh){ mesh->texes.push_back( tex ) ; } ) ;
}

void Shape::setOnlyTextureTo( D3D11Surface * tex )
{
  this->meshGroup->each( [tex](Mesh*mesh){ mesh->texes.clear(); mesh->texes.push_back( tex ) ; } ) ;
}

// sum the light sources before calling
void Shape::shLight( SHVector* shLightSource )
{
  Vector trans = material.getColor( ColorIndex::Transmissive ) ;
  real avgOpac = 1 - (trans.x+trans.y+trans.z)/3;
  
  for( int mn = 0 ; mn < meshGroup->meshes.size() ; mn++ )
  {
    Mesh* mesh = meshGroup->meshes[ mn ] ;
    
    for( int i = 0 ; i < mesh->verts.size() ; i++ )
    {
      AllVertex &vertex = mesh->verts[i] ;

      // This does the diffuse component of the lighting.
      Vector color = mesh->verts[i].shDiffuseSummed->dot( shLightSource ) ;

      // THE SPECULAR TERM
      if( mesh->verts[i].color[ ColorIndex::SpecularMaterial ].nonzero() )  // skip if surface purely diffuse
      {
        SHVector* shReflLight = mesh->verts[i].shSpecularMatrix->mult( shLightSource ) ;
      
        ////// take the reflected light, convolve it with the brdf kernel
        // Yo uh, brdfSHSpecular must be circularly symmetric (about phi) (which it is)
        SHVector* convd = window->scene->brdfSHSpecular->shProjection->conv( shReflLight ) ;
        DESTROY( shReflLight ) ;

        ////// "evaluate at r" -- dot it in the reflection direction
        Vector eyeToVertex = (vertex.pos - window->camera->eye).normalize() ;
        Vector& reflDir = eyeToVertex.reflect( vertex.norm ) ;
        SHSample shReflDir( window->shSamps->bands, reflDir.toSpherical() ) ;
        for( int k = 0 ; k < window->shSamps->bands ; k++ )
          color += convd->coeffsScale[k] * shReflDir.bandWeights[k] ;
      
        DESTROY( convd ) ;
      }

      mesh->verts[i].color[ ColorIndex::SHComputed ] = color ;

      // Now you can set the alpha to being the average transparency
      // of the 3 bands, as a rough approximation
      mesh->verts[i].color[ ColorIndex::SHComputed ].w = avgOpac ;
    }
  }  
}

void Shape::shDrawVertexVisualizations()
{
  for( int mn=0; mn < meshGroup->meshes.size() ; mn++ )
  {
    Mesh *mesh = meshGroup->meshes[ mn ] ;
    for( int j = 0 ; j < mesh->verts.size() ; j++ )
    {
      // Ask the SH projection to generate the viz, if it exists
      if( mesh->verts[ j ].shDiffuseAmbient )
        mesh->verts[ j ].shDiffuseAmbient->generateVisualization( 10, 10, mesh->verts[j].pos );
    }
  }
}

void Shape::draw()
{
  for( int i=0; i < meshGroup->meshes.size() ; i++ )
    meshGroup->meshes[ i ]->draw() ;
}

void Shape::addMesh( Mesh*mesh )
{
  // If there's no meshgroup in this shape,
  // create one compatible with the mesh being added
  if( !meshGroup )
    meshGroup = new MeshGroup( this, mesh->meshType, mesh->vertexType, 1200 ) ;
  meshGroup->addMesh( mesh ) ;
}

void Shape::takeMeshesFrom( Shape * o )//transferMeshes( Shape * o )
{
  meshGroup = o->meshGroup ; // steal his meshgroup
  
  // REASSIGN MESH OWNING POINTERS AS OWNING SHAPE HAS CHANGED
  meshGroup->setOwner( this ) ;

  o->meshGroup = 0 ; //unlink
}

//void Shape::cloneMeshesFrom( Shape * o )
//{
//  meshGroup = o->meshGroup->clone( this ) ;
//}