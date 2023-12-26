///
/// GTP - graphics test project
/// by William Sherif
///
//#include <vld.h>
///#define _CRTDBG_MAP_ALLOC
///#include <stdlib.h>
///#include <crtdbg.h>
/// disable these annoynings
//#pragma warning( disable: 4018 4244 4482 4996 4800 )
#include "main.h"
#include "window/Camera.h"
#include "window/GTPWindow.h"
//#include "geometry/Sphere.h"
#include "geometry/Cube.h"
#include "geometry/Tetrahedron.h"
#include "geometry/Octahedron.h"
#include "geometry/Icosahedron.h"
#include "geometry/MathematicalShape.h"
#include "geometry/Torus.h"
#include "geometry/Quad.h"
#include "geometry/CubeMap.h"
#include "geometry/Bezier.h"
#include "geometry/Model.h"
#include "Globals.h"
#include "rendering/RaytracingCore.h"
#include "rendering/RadiosityCore.h"
#include "rendering/FullCubeRenderer.h"
#include "math/SHVector.h"

GTPWindow *window ;

enum Scenes
{
  Bunny,
  BunnyCrappy,
  KickingGirl,
  Tree,
  Dragon,
  Shapes,
  Shapes1,
  DoubleRefln,
  SpheresX4,
  SpheresX25,
  SingleCube,
  SingleSphere,
  GlassPlate,
  CheckeredFloor,
  CheckeredPlaneRandom,
  CheckeredPlaneBW,
  SolidFloor,
  CornerCase,
  CubesX6,
  Walls,
  WallsCeilingLight,
  Teapot,
  Lucy,
  Tearoom,
  BezierObject,
  SingleTriangle,
  Tori,
  SphereFromOctahedron,
  Icosahedra,
  Sea,
  SeaFloor,
  PlaneFloor,
  PlaneFloorx2,
  MarbleFloor,
  SmallSpheres
} ;

enum LightScenes
{
  CubeColor,
  CubeGrace,
  BlueLobe,
  RedGreenLobes,
  SphereWhite,
  HemisphereWhiteX20,
  FlatWhiteX20,
  Sun,
  CamLight, // camera controlled light
  Underlamp,
  SkyDome,
  SphereLightX4,
  PXWhiteLobe,
  WallLight,
  CubeColor64,
  BlackWhite,
  Teal,
  PurpleGalaxy
} ;

void generateScene( int sceneNumber )
{
  real tess = 0.05 ; //smaller than 0.005 takes a really long time
  
  int sphereDivisions = 12 ;
  real minShapeSize=.78, maxShapeSize=1.2;
  real boundSize = 7;
  int numShapes = 2 ;
  int uPatches=1,vPatches=1;

  AABB bounds(
    Vector( -boundSize/2, 0, -boundSize/2 ),
    Vector(  boundSize/2,  boundSize/2,  boundSize/2 ) ) ;
  
  real matLow=.65, matHigh=1;
  Vector spec( 1,1,1,25 ) ;

  Material modelMaterial( Vector(1,0,0), 0, 0, 0, 1 ) ;

  if( sceneNumber == Scenes::SpheresX4 )
  {
    // diffuse, e, s, t, fresnel
    real p = 1.25, rad=1 ;
    //window->scene->addShape( new Sphere( "sphere1", Vector( 0, p, 0 ), 2, m1, sphereDivisions, sphereDivisions ) ) ;
    //window->scene->addShape( new Sphere( "sphere2", Vector( p, p, p ), 2, m2, sphereDivisions, sphereDivisions ) ) ;
    //window->scene->addShape( new Sphere( "sphere3", Vector( p, p, -p ), 2, m3, sphereDivisions, sphereDivisions ) ) ;
    
    real S = 1 ;

    int subd = 3 ;
    // make it from an icosahedron
    Icosahedron *icosahedron = new Icosahedron( "spheresx4 icosahedron", rad, Material( Vector(1,0,0), 0, Vector(S,S,S,25), 0, 1.33 ) ) ;//SPECULAR
    Sphere *sp = Sphere::fromShape( "red sphere", icosahedron, icosahedron->r, subd ) ;
    sp->transform( Matrix::Translate( -p,p,-p ) ) ;
    window->scene->addShape( sp ) ;

    icosahedron->material = Material( Vector(1,1,1), 0, Vector(S,S,S,25), 0, 1.33 ) ; //SPECULAR
    //icosahedron->material = Material( Vector(0,0,0), 0, spec, Vector(1,1,1), Vector( 2.410, 2.430, 2.450 ) ) ;
    sp = Sphere::fromShape( "white sphere", icosahedron, icosahedron->r, subd ) ;
    sp->transform( Matrix::Translate( -p,p,p ) ) ;
    window->scene->addShape( sp ) ;
    
    icosahedron->material = Material( Vector(0,1,0), 0, Vector(S,S,S,25), 0, 1.33 ) ; // NON-SHINY
    sp = Sphere::fromShape( "green sphere", icosahedron, icosahedron->r, subd ) ;
    sp->transform( Matrix::Translate( p,p,p ) ) ;
    window->scene->addShape( sp ) ;

    icosahedron->material = Material( Vector(0,0,1), 0, Vector(S,S,S,25), 0, 1.33 ) ; 
    sp = Sphere::fromShape( "blue sphere", icosahedron, icosahedron->r, subd ) ;
    sp->transform( Matrix::Translate( p,p,-p ) ) ;
    window->scene->addShape( sp ) ;

    DESTROY( icosahedron ) ;
  }
  
  if( sceneNumber == Scenes::SpheresX25 )
  {
    // diffuse, e, s, t, fresnel
    real p = 1.25, rad=1 ;
    //window->scene->addShape( new Sphere( "sphere1", Vector( 0, p, 0 ), 2, m1, sphereDivisions, sphereDivisions ) ) ;
    //window->scene->addShape( new Sphere( "sphere2", Vector( p, p, p ), 2, m2, sphereDivisions, sphereDivisions ) ) ;
    //window->scene->addShape( new Sphere( "sphere3", Vector( p, p, -p ), 2, m3, sphereDivisions, sphereDivisions ) ) ;
    
    bool specular=true ;

    int subd = 2 ; //2
    // make it from an icosahedron
    Icosahedron *icosahedron = new Icosahedron( "spheresx25 icosahedron", rad,
      Material( Vector(1,1,1), 0, specular?Vector(1,1,1,25):Vector(0,0,0,25), 0, 1.33 ) ) ;

    real xs = 2, zs = 2 ;
    int nx = 5, nz=5;
    char bName[255];
    int sNo = 1 ;
    for( int i = 0 ; i < nx ; i++ )
    {
      for( int j = 0 ; j < nz ; j++ )
      {
        icosahedron->material.kd=icosahedron->material.ks=Vector::random() ;
        sprintf( bName, "sphere %d/%d", sNo++, nx*nz ) ;
        Sphere *sp = Sphere::fromShape( bName, icosahedron, icosahedron->r, subd ) ;
        sp->transform( Matrix::Translate( (i-(nx-1.)/2.)*xs, p, (j-(nz-1.)/2.)*zs ) ) ;
        window->scene->addShape( sp ) ;
      }
    }
    
    DESTROY( icosahedron ) ;
  }
  
  
  if( sceneNumber == Scenes::CubesX6 )
  {
    window->scene->addShape( (new Cube(
      "c green", Vector( 0, 10, 0 ), 4,
      uPatches,vPatches, 0,0,0,
      Material::Green ) )->tessellate( tess ) ) ;
    window->scene->addShape( (new Cube(
      "c magenta", Vector( 0, -10, 0 ), 4,
      uPatches,vPatches, 0,0,0, Material::Magenta ))->tessellate( tess )  ) ;

    window->scene->addShape( (new Cube(
      "c red", Vector( 10, 0, 0 ), 4,
      uPatches,vPatches, 0,0,0, Material::Red ))->tessellate( tess )  ) ;
    window->scene->addShape( (new Cube(
      "c dark red", Vector( -10, 0, 0 ), 4,
      uPatches,vPatches, 0,0,0, Material::DkRed ))->tessellate( tess )  ) ;

    window->scene->addShape( (new Cube( 
      "c orange", Vector( 0, 0, 10 ), 4,
      uPatches,vPatches, 0,0,0, Material::Orange ))->tessellate( tess )  ) ;
    window->scene->addShape( (new Cube( 
      "c dark orange", Vector( 0, 0, -10 ), 4,
      uPatches,vPatches, 0,0,0, Material::DkOrange ))->tessellate( tess )  ) ;
  }

  if( sceneNumber == Scenes::Shapes )
  {
    for( int i = 0 ; i < numShapes ; i++ )
    {
      char buf[260];
      sprintf( buf, "sphere-%02d", i ) ;
      info( "Generating %s", buf ) ;
      window->scene->addShape( new Sphere(
        buf, bounds.random(),
        randFloat( minShapeSize, maxShapeSize ),
        Material::randomSolid(matLow,matHigh),
        sphereDivisions, sphereDivisions ) ) ; 
  
      sprintf( buf, "cube-%02d", i ) ;
      info( "Generating %s", buf ) ;
      Cube* cube = new Cube(
        buf, bounds.random(),
        randFloat( minShapeSize, maxShapeSize ),
        uPatches,vPatches,
        randFloat( 0, 90 ), randFloat( 0, 90 ), randFloat( 0, 90 ),
        Material::randomSolid(matLow,matHigh) ) ;
      window->scene->addShape( cube->tessellate( tess ) ) ; 
  
      sprintf( buf, "tet-%02d", i ) ;
      info( "Generating %s", buf ) ;
      Tet* tet = new Tet(
        buf,
        bounds.random(),
        randFloat( minShapeSize, maxShapeSize ),
        Material::randomSolid(matLow,matHigh) ) ;
      window->scene->addShape( tet->tessellate( tess ) ) ;

      sprintf( buf, "torus-%02d", i ) ;
      Torus *torus = new Torus( buf, 0,
        randFloat( minShapeSize, maxShapeSize ),
        randFloat( minShapeSize, maxShapeSize ),
        Material::randomSolid( matLow, matHigh ),
        sphereDivisions, sphereDivisions );
      
      Matrix xform = Matrix::Translate( bounds.random() )
        * Matrix::RotationYawPitchRoll( randFloat(0,2*PI), randFloat(0,2*PI), randFloat(0,2*PI) ) ;
      torus->transform( xform );
      window->scene->addShape( torus ) ;

      sprintf( buf, "octa-%02d", i ) ;
      Octahedron *oct = new Octahedron( buf, randFloat( minShapeSize, maxShapeSize ), Material::randomSolid( matLow, matHigh ) ) ;
      oct->randomPosAndRot( bounds ) ;
      window->scene->addShape( oct ) ;

      sprintf( buf, "icos-%02d", i ) ;
      Icosahedron *icos = new Icosahedron( buf, randFloat( minShapeSize, maxShapeSize ),
        Material::randomSolid( matLow, matHigh ) ) ;
      icos->randomPosAndRot( bounds ) ;
      window->scene->addShape( icos ) ;
    }

    // give each a perlin noise generator
    for( int i = 0 ; i < window->scene->shapes.size() ; i++ )
    {
      Shape *s = window->scene->shapes[i];
      s->material.ks = 0 ;
      
      // The diffuse material has a perlin texture
      s->material.perlinGen[ColorIndex::DiffuseMaterial] = new PerlinGenerator(
        randInt(0, 20000), 2.0, 2.0, 8,
        randFloat( 1,2 )*s->aabb->extents(),
        Vector::random(), Vector::random() ) ;
    }
  }

  if( sceneNumber == Scenes::SolidFloor )
  {
    // ground plane
    real h = 12 ;
    Cube *cube = new Cube( Vector(-h,-1,-h), Vector(h,0,h) ) ;
    cube->name = "Ground plane" ;
    info( "Generating %s", cube->name.c_str() ) ;
    cube->material = Material( 1, 0, 0, 0, 1 ) ;
    cube->createMesh( MeshType::Nonindexed, defaultVertexType, uPatches,vPatches ) ;
    cube->tessellate( tess ) ;

    window->scene->addShape( cube ) ;
  }

  if( sceneNumber == Scenes::CornerCase )
  {
    // 2 red cubes on opposite sides of a plane
    real s = 6 ;
    Quad *q = new Quad( "plane floor", Vector(s,0,-s), Vector(-s,0,-s), Vector(-s,0,s), Vector(s,0,s),
      uPatches*2,vPatches*2,
      Material( 1,0,1,0,1 ) );
    q->tessellate( tess ) ;
    window->scene->addShape( q ) ;

    q = new Quad( "wall 1", Vector(s,0,-s), Vector(-s,0,-s), Vector(-s,0,s), Vector(s,0,s),
      uPatches,vPatches,
      Material( 1,0,1,0,1 ) );
    Matrix xform = Matrix::Translate( -s-.1, s/2, 0 )*Matrix::RotationZ( RADIANS(-90.0) ) ;
    q->transform( xform ) ;
    q->tessellate( tess ) ;
    window->scene->addShape( q ) ;

    //q = new Quad( "wall 2", Vector(s,0,-s), Vector(-s,0,-s), Vector(-s,0,s), Vector(s,0,s),
    //  uPatches,vPatches,
    //  Material( Vector(1,0,0),0,1,0,1 ) );
    //q->transform( Matrix::Translate( s+.1, s/2, 0 )*Matrix::RotationZ( RADIANS(90.0) ) ) ;
    //window->scene->addShape( q ) ;
  }

  if( sceneNumber == Scenes::CheckeredFloor )
  {
    // generate a tile floor
    Vector start( -15,-1,-15 ) ;
    Vector step( 3, 0, 3 ) ;
    for( int i = 0 ; i < 10 ; i++ )
    {
      for( int j = 0 ; j < 10 ; j++ )
      {
        Vector min = start + Vector(step.x*i,0,step.z*j ) ;
        Vector max = start + Vector(0,1,0) + Vector(step.x*(i+1),0,step.z*(j+1) ) ;
        
        Material cubeMat = Material::randomSolid() ;
        //cubeMat.kd.clamp( 0, .5 ) ;
        cubeMat.ks = 0 ; // this should be consistent or it looks odd (some reflective tiles)
        ////cubeMat.kt = .5 ;
        
        Cube *cube = new Cube( "cube", min, max, uPatches,vPatches, cubeMat ) ;
        cube->tessellate( tess ) ;
        
        window->scene->addShape( cube ) ;
      }
    }
  }

  if( sceneNumber == Scenes::CheckeredPlaneRandom )
  {
    // generate a tile floor
    Vector start( -15,0,-15 ) ;
    Vector step( 3, 0, 3 ) ;
    for( int i = 0 ; i < 10 ; i++ )
    {
      for( int j = 0 ; j < 10 ; j++ )
      {
        Vector a = start + Vector(step.x*(i+1),0,step.z*j ) ;
        Vector b = start + Vector(step.x*i,0,step.z*j ) ;
        Vector c = start + Vector(step.x*i,0,step.z*(j+1) ) ;
        Vector d = start + Vector(step.x*(i+1),0,step.z*(j+1) ) ;
        
        Material mat = Material::randomSolid() ;
        //cubeMat.kd.clamp( 0, .5 ) ;
        mat.ks = 1 - mat.kd ; // this should be consistent or it looks odd (some reflective tiles)
        ////cubeMat.kt = .5 ;
        
        Quad *q = new Quad( "quad", a,b,c,d, uPatches,vPatches, mat ) ;
        q->tessellate( tess ) ;
        
        window->scene->addShape( q ) ;
      }
    }
  }

  if( sceneNumber == Scenes::CheckeredPlaneBW )
  {
    // generate a tile floor
    real s = 10 ; // s=60 for statue, 15 for teapot
    real t=s/5 ;
    Vector start( -s,0,-s ) ;
    Vector step( t, 0, t ) ;
    int rows=10,cols=10;

    for( int i = 0 ; i < rows ; i++ )
    {
      for( int j = 0 ; j < cols ; j++ )
      {
        Vector a = start + Vector(step.x*(i+1),0,step.z*j ) ;
        Vector b = start + Vector(step.x*i,0,step.z*j ) ;
        Vector c = start + Vector(step.x*i,0,step.z*(j+1) ) ;
        Vector d = start + Vector(step.x*(i+1),0,step.z*(j+1) ) ;
        
        Material mat ;
        mat.ks = 0 ;
        //mat.specularJitter = 0.12 ; // COOK
        Vector onColor = 1, //Vector(1,0,0)
              offColor = 0; //Vector(1,1,0) ; 
        if( (i%2) == (j%2) ) mat.kd = offColor ;
        else mat.kd = onColor ;
        ////cubeMat.kt = .5 ;
        
        Quad *q = new Quad( "quad", a,b,c,d, uPatches,vPatches, mat ) ;
        q->tessellate( tess ) ;
        
        window->scene->addShape( q ) ;
      }
    }
  }

  if( sceneNumber == Scenes::Bunny )
  {
    window->cd( DIR_BASE( "/res/models/bunny" ) ) ;
    Material bunnyMat( Vector(.5,.5,.5,1), 0, .5, 0, 1.5 ) ;
    Model *model = new Model( "rabbit_full.obj", FileType::OBJ, MeshType::Indexed, bunnyMat ) ;
    model->meshGroup->calculateNormalsSmoothedAcrossMeshes() ;
    model->updateMesh() ;
    window->scene->addShape( model ) ;
    window->cdPop() ;
  }

  if( sceneNumber == Scenes::Shapes1 )
  {
    window->cd( DIR_BASE( "/res/models/shapes" ) ) ;
    Model *model = new Model( "shapes1.obj", FileType::OBJ, MeshType::Indexed, modelMaterial ) ;
    //model->meshGroup->calculateNormalsSmoothedAcrossMeshes() ;
    //model->updateMesh() ;
    //for( int i = 0 ; i < model->meshGroup->meshes.size() ; i++ )
    //  model->meshGroup->meshes[i]->setMaterial
    model->material.specularJitter = 0.5 ;
    window->scene->addShape( model ) ;
    window->cdPop() ;
  }

  if( sceneNumber == Scenes::DoubleRefln )
  {
    window->cd( DIR_BASE( "/res/models/gtpTestScenes" ) ) ;
    Model *model = new Model( "doubleRefln.obj", FileType::OBJ, MeshType::Indexed, modelMaterial ) ;
    //model->meshGroup->calculateNormalsSmoothedAcrossMeshes() ;
    //model->updateMesh() ;
    window->scene->addShape( model ) ;
    window->cdPop() ;
  }

  if( sceneNumber == Scenes::BunnyCrappy )
  {
    window->cd( DIR_BASE( "/res/models/bunny" ) ) ;
    
    Material bunnyMat( Vector(.5,.5,.5,1), 0, .5, 0, 1.5 ) ;
    Model *model = new Model( "rabbit.obj", FileType::OBJ, MeshType::Indexed, bunnyMat ) ; //crabbit
    model->meshGroup->calculateNormalsSmoothedAcrossMeshes() ;
    model->updateMesh() ;
    
    window->scene->addShape( model ) ;
    
    // try wood
    //model->perlinGen = new PerlinGenerator( PerlinNoiseType::Wood, 8, model->aabb->extents().max() ) ;
    //
    // weird blue
    //model->material.perlinGen[ ColorIndex::DiffuseMaterial ] = new PerlinGenerator( 4, .5, 1.44, 8, .5*model->aabb->extents().max(),
    //  1, Vector(.6, .45, 1), Vector(.32,.11,.69), Vector(1,0,0),Vector(.5,0,0) ) ; 

    window->cdPop() ;
  }

  if( sceneNumber == Scenes::KickingGirl )
  {
    window->cd( DIR_BASE( "/res/models/kicking" ) ) ;
    Model *model = new Model( "kicking girl.obj", FileType::OBJ, MeshType::Indexed, Material::White ) ;
    model->meshGroup->calculateNormalsSmoothedAcrossMeshes() ;
    model->updateMesh() ;
    window->scene->addShape( model ) ;
    window->cdPop() ;
  }
  
  if( sceneNumber == Scenes::Tree )
  {
    window->cd( DIR_BASE( "/res/models/tree" ) ) ;
    Model *model = new Model( "tree1f.obj", FileType::OBJ, MeshType::Indexed, Material::White ) ;
    model->meshGroup->calculateNormalsSmoothedAcrossMeshes() ;
    model->updateMesh() ;
    window->scene->addShape( model ) ;
    window->cdPop() ;
  }

  if( sceneNumber == Scenes::Dragon )
  {
    window->cd( DIR_BASE( "/res/models/dragon" ) ) ;
    // 
    Model *model = new Model( "dragonUncut25kv.obj", FileType::OBJ, MeshType::Indexed, modelMaterial ) ;
    window->scene->addShape( model ) ;
    window->cdPop() ;
  }
  
  if( sceneNumber == Scenes::SingleCube )
  {
    //Material m( .4, 0, .6, 0, 1.02 ) ;
    //Cube *cube = new Cube( "cube", Vector(0,4,0), 2, uPatches,vPatches, 2,3,1, m ) ;
    //window->scene->addShape( cube ) ;

    AABB a3 = AABB(0,10).getIntersectionVolume(AABB(5,8)) ;
    AABB a4 = AABB(8,10).getIntersectionVolume(AABB(0,10)) ;

    //window->scene->addShape( new Cube( "cube1", a1.min, a1.max, 2, 2, Material::White ) ) ;
    window->scene->addShape( new Cube( "cube", a3.min, a3.max, 2, 2, Material::White ) ) ;
    window->scene->addShape( new Cube( "cube2", a4.min, a4.max, 2, 2, Material::Red ) ) ;
  }

  if( sceneNumber == Scenes::SingleSphere )
  {
    //          D, E, S, T
    //Material m( 1, 0, Vector(1,1,1,25), 0, 1.6 ) ;

    // Firey
    //Material m( Vector(.1,.1,.1), 0, Vector(.5,.5,.5,25), Vector(.5,.4,.05), 1.6 ) ;
    
    // Like blue fire
    //Material m( Vector(.1,.1,.1), 0, Vector(.5,.5,.5,25), Vector(.01,.2,.4), 1.6 ) ;

    //Material m( Vector(.3,.1,.4), 0, Vector(1,1,1,25), Vector(.25), 1.6 ) ;
    //Material m( Vector(.3,.1,.4), 0, Vector(1,1,1,25), Vector(0), 1.6 ) ;
    //Material m( Vector(.7,.16,.9), 0, Vector(1,1,1,25), Vector(0), 1.6 ) ;
    Material m( Vector(0,0,0), 0, Vector(1,1,1,25), Vector(0), 1.6 ) ;

    //m.specularJitter = .2 ;

    // make it from an icosahedron
    int subd = 3 ;
    Icosahedron *icosahedron = new Icosahedron( "single sphere icosahedron", 1, m ) ;
    Sphere *sp = Sphere::fromShape( "single sphere", icosahedron, icosahedron->r, subd ) ;
    DESTROY( icosahedron ) ;

    //sp->transform( Matrix::Translate( 3, 2.5, 3 ) ) ;
    sp->transform( Matrix::Translate( 0, 1.25, 0 ) ) ;

    //sp->createMesh( defaultMeshType, defaultVertexType ) ;

    //sp->updateMesh() ;
    ///Sphere *sp  = new Sphere( "sphere", Vector(0,2.5,0), 2, m, 22, 22 ) ;
    // purple ball
    //sp->perlinGen = new PerlinGenerator( 4, 2, 2, 12, sp->aabb->extents().max(), 1, Vector(.6, .45, 1), Vector(.32,.11,.69) ) ; 
    
    // marbled ball
    //sp->material.perlinGen[ ColorIndex::DiffuseMaterial ] = new PerlinGenerator( PerlinNoiseType::Marble, 4, sp->aabb->extents().max() ) ;
    
    // funky yellow ball
    //sp->perlinGen = new PerlinGenerator( 4, .5, 1.44, 8, sp->aabb->extents().max(), 1, Vector(.6, .45, 1), Vector(.32,.11,.69) ) ; 

    //sp->shProject( window->sh ) ;
    //sp->shProjection->generateVisualization( 32, 32, Vector(0,25,0) )jkhZ c ;
    
    window->scene->addShape( sp ) ;
  }

  if( sceneNumber == Scenes::GlassPlate )
  {
    // put a glass plate at the origin
    // ground plane
    real h = 12 ;
    
    Cube *cube = new Cube( Vector(-h,-1,-h), Vector(h,0,h) ) ;
    cube->name = "Glass plate" ;
    info( "Generating %s", cube->name.c_str() ) ;
    cube->material = Material( Vector(1,1,1), 0, Vector(0,0,0,25), Vector(.1,.1,.1), 1.6 ) ;
    cube->createMesh( MeshType::Nonindexed, defaultVertexType, uPatches,vPatches ) ;
    //cube->tessellate( tess ) ; // skpi for rt.

    window->scene->addShape( cube ) ;
  }

  if( sceneNumber == Scenes::Walls )
  {

    AABB boundsWalls(
    Vector( -boundSize,  0, -boundSize ),
    Vector(  boundSize,  boundSize,  boundSize ) ) ;

    // unicolor.
    Material matWalls = Material( Vector(.8,.8,.8), 0, .1, 0, 1.33 ) ;
    
    // turns it inside out.
    Cube * walls = new Cube( "walls", boundsWalls.max, boundsWalls.min, uPatches,vPatches, matWalls ) ;
    //walls->meshGroup->meshes[0]->transform( Matrix::Scale( -1,-1,-1 ) ) ; // turn the box inside out,
    // so the INSIDES are now 
    walls->tessellate( tess ) ;

    window->scene->addShape( walls ) ;
  }
  
  if( sceneNumber == Scenes::WallsCeilingLight )
  {
    real lightSize = boundSize/3;
    
    // add an area light source
    Material lightMat( 0, 20, 0, 0, 0 ) ;
    
    Cube *light = new Cube( "area light",
      Vector( -lightSize, boundSize - boundSize*.02, -lightSize ),
      Vector( lightSize, boundSize, lightSize ),
      uPatches,vPatches,
      lightMat ) ;
    //light->tessellate( tess ) ;
    window->scene->addShape( light ) ;
  }

  if( sceneNumber == Scenes::Teapot )
  {
    window->cd( DIR_BASE( "/res/models/newell_teaset" ) ) ;
    
    Material teamat( Vector(.5,0,0), 0, Vector(0), 0, 1 ) ; // red teapot white highlight
    //Material teamat( Vector(1,1,1), 0, Vector(1,1,1), Vector(0,0,0), 1.66 ) ;
    //Material teamat( 0, 0, 1, Vector(.5,.5,.5), Vector( 2.410, 2.430, 2.450 ) ) ;
    //teamat.specularJitter = 0.12 ; // COOK
    //teamat.transmissiveJitter = 0.02 ;
    
    BezierPatch *bp = new BezierPatch( MeshType::Indexed, VertexType::VT10NC10, "teapot", 16,16, teamat ) ;
    bp->transform( Matrix::RotationX( RADIANS(-90.0) ) ) ;
    bp->meshGroup->calculateNormalsSmoothedAcrossMeshes() ;
    bp->updateMesh() ;
    
    window->scene->addShape( bp ) ;

    // take the shprojection make a viz and add it to viz's (does it automatically)
    ////bp->shProject() ;
    ////bp->shProjection->generateVisualization( 64,64, bp->getCentroid() + Vector( 2, 5, 0 ) ) ;

    // Yellow funky ball
    ////bp->material.perlinGen[ ColorIndex::DiffuseMaterial ] = new PerlinGenerator(
    ////  4, .5, 1.44, 8, 2*bp->aabb->extents().max(), 1, Vector(.6, .45, 1), Vector(.32,.11,.69) ) ; 

    window->cdPop() ;
  }

  if( sceneNumber == Scenes::Lucy )
  {
    window->cd( DIR_BASE( "/res/models/lucy" ) ) ;
    Material lucMat( 1,0,1,0,Vector( 2.410, 2.430, 2.450 ) ) ;
    //Material lucMat( Vector(1,1,1),0,0,0,1 ) ;
    //lucMat.specularJitter = 0.2 ;
    //lucMat.transmissiveJitter = 0.021 ;
    Model *model = new Model( "lucy66kvnormals.obj", FileType::OBJ, MeshType::Indexed, VertexType::VT10NC10, lucMat ) ;
    //model->meshGroup->calculateNormalsSmoothedAcrossMeshes() ;
    //model->updateMesh() ;
    window->scene->addShape( model ) ;
    window->cdPop() ;




    ///////AABB boundsWalls(
    ///////Vector( -20,  0, -20 ),
    ///////Vector(  20, 40,  20 ) ) ;
    ///////
    ///////// unicolor.
    ///////Material matWalls = Material( .5, 0, .1, 0, 1.33 ) ;
    ///////Cube * walls = new Cube( "walls", boundsWalls.max, boundsWalls.min, uPatches,vPatches, matWalls ) ;
    /////////walls->tessellate( tess ) ;
    ///////window->scene->addShape( walls ) ;
    ///////
    ///////Material m( 0,1,0,0,1 ) ;
    ///////Sphere *light = new Sphere( "light", Vector( 19, 28, 19 ), 2, m, 4,4 ) ;
    ///////window->scene->addShape( light ) ;
  }

  if( sceneNumber == Scenes::Tearoom )
  {
    // make a complete scene with teapot and walls and a table
    
    /*
    //teapot
    window->cd( DIR_BASE( "/res/models/newell_teaset" ) ) ;
    BezierPatch *bp = new BezierPatch( MeshType::Indexed, VertexType::VT10NC10, "teapot", 8,8,
      Material( Vector(.2,.04,.8), 0, 1, 0, 1.5 ) ) ;
    bp->transform(
      Matrix::Translate( 2, 2, -2 ) // move it on top of table
    * Matrix::RotationY( RADIANS(-35.0) ) // twist it a bit
    * Matrix::RotationX( RADIANS(-90.0) ) // put lid at TOP of teapot
    * Matrix::Scale(.25) // shrink it
    ) ;

    bp->meshGroup->calculateNormalsSmoothedAcrossMeshes() ;
    bp->updateMesh() ;
    window->scene->addShape( bp ) ;
    window->cdPop() ;
    */

    //donut
    Torus* t = new Torus( "torus", 0, .1, .15,
      Material( Vector(.7,.2,.01), 0, 0, 0, 1.5 ),
      13, 13 ) ;
    t->transform( Matrix::RotationX( RADIANS(-115.0) )*Matrix::Translate( 1.5,2,-1.9 ) ) ;
    window->scene->addShape( t ) ;

    //walls
    AABB boundsWalls( Vector(-4,0,-4), Vector(4,5,4) ) ;

    // turns it inside out.
    uPatches=vPatches=25;
    Cube * walls = new Cube( "walls", boundsWalls.max, boundsWalls.min, uPatches,vPatches,
      Material( Vector(.8,.8,.8), 0, Vector(.1,.1,.1), 0, 1 ) ) ;
    //walls->tessellate( tess ) ;
    window->scene->addShape( walls ) ;

    // light source
    Sphere* light = Sphere::fromIcosahedron( "Light", .2, 0, Material( 1, 1, 0, 0, 1 ) ) ;
    light->transform( Matrix::Translate( 1, 2, 0 ) ) ;
    window->scene->addShape( light ) ;
  }

  if( sceneNumber == Scenes::BezierObject )
  {
    // create something random

    //BezierPatch bp
  }

  if( sceneNumber == Scenes::SingleTriangle )
  {
    Model *model = new Model( "single tri", Material( 1, 0, 0, 0, 1 ) ) ;
    
    real s = 5 ;
    model->addTri( Vector( s, -s, -s ), Vector( s, -s, s ), Vector( s, s, s ) ) ;

    window->scene->addShape( model ) ; // creates vb
  }

  if( sceneNumber == Scenes::Tori )
  {
    //Torus* t = new Torus( "torus", 0, 2, 3, Material( 1, 0, 0, 0, 1.5 ), 32, 32 ) ;
    //t->transform( Matrix::Translate( 0, 3, 0 ) * Matrix::RotationX( PI/2 ) ) ;
    Torus* t = new Torus( "torus", 0, 2, 3, Material( 1, 0, 0, 0, 1.5 ), 32, 32 ) ;
    window->scene->addShape( t ) ;
  }

  if( sceneNumber == Scenes::SphereFromOctahedron )
  {
    // make a sphere based on an octahedron
    //Octahedron *o1 = new Octahedron( "oct", 3, Material(1,0,.2,.6,1.5) ) ;
    Octahedron *o1 = new Octahedron( "oct", 3, Material(1,0,0,0,1.5) ) ;
    window->scene->addShape( Sphere::fromShape( "sphere from oct", o1, o1->r, 3 ) ) ;
    DESTROY( o1 ) ;

    //Icosahedron *o1 = new Icosahedron( "oct", 2, Material(1,0,.2,.6,1.5) ) ;
    //window->scene->addShape( Sphere::fromShape( o1, o1->r, 3 ) ) ;
    //DESTROY( o1 ) ;

    // scenes from other shapes
    //window->scene->addShape( Sphere::fromShape( i1, 4.0, 2 ) ) ;
    //window->scene->addShape( Sphere::fromShape( (new Cube( -1, 1 ))->createMesh(defaultMeshType,defaultVertexType,1,1), 4.0, 5 ) ) ;
    //window->scene->addShape( Sphere::fromShape( (new Tet( "tet", 0, 3, Material::White ))->createMesh(defaultMeshType,defaultVertexType), 4.0, 5 ) ) ;
  }

  if( sceneNumber == Scenes::Icosahedra )
  {
    for( int i = 0 ; i < 4 ; i++ )
      window->scene->addShape( ( new Icosahedron( "icos", randFloat( minShapeSize, maxShapeSize ),
      Material::randomSolid( matLow, matHigh ) ) )->randomPosAndRot( bounds ) ) ;
  }

  if( sceneNumber == Scenes::Sea )
  {
    real s = 25 ; //17 ;
    Quad *q = new Quad( "sea", Vector(s,0,-s), Vector(-s,0,-s), Vector(-s,0,s), Vector(s,0,s),
      uPatches,vPatches,

      // DIFFUSE OPAQUE BLUE WATER (WRONG BUT COOL LOOKING IN SHADER)
      Material( Vector(0.01,0.5,0.7), 0,
                Vector(1,1,1,25),Vector(0),1 )
      
      // CORRECT (specular), but it really depends on angle.
      //Material( 0,0,Vector(1,1,1,25),Vector(.9,.9,.9),1.33 ) // specular (correct)
    );
    q->tessellate( tess ) ;
    window->scene->addShape( q ) ;

    for( int j = 0 ; j < q->meshGroup->meshes.size() ; j++ )
    {
      for( int k = 0 ; k < q->meshGroup->meshes[j]->verts.size() ; k++ )
      {
        AllVertex& vertex = q->meshGroup->meshes[j]->verts[k] ;

        for( int w = 0 ; w < window->waveGroup.size() ; w++ )
        {
          // apply the wave motion
          Vector::wave( 0.45, window->waveGroup[w], vertex.pos, vertex.norm ) ;

          // Also .. 
        }
      }

      // you changed the elements, now update the tris collection
      q->meshGroup->meshes[j]->updateTrisFromVertsAndFaces() ;
    }

    q->updateMesh() ;
    
  }

  if( sceneNumber == Scenes::SeaFloor )
  {
    real s = 25 ;
    real y = -6 ;
    Quad *q = new Quad( "sea floor", Vector(s,y,-s), Vector(-s,y,-s), Vector(-s,y,s), Vector(s,y,s),
      uPatches,vPatches,
      Material( Vector(0.6,0.5,0.2),0,Vector(0,0,0,25),0,1 )
    ) ;
    
    //q->tessellate( tess ) ;
    window->scene->addShape( q ) ;
  }
  
  if( sceneNumber == Scenes::PlaneFloor )
  {
    real s = 8 ;
    uPatches=vPatches= 50 ;
    
    Quad *q = new Quad( MeshType::Indexed, VertexType::VT10NC10, "plane floor", Vector(s,0,-s), Vector(-s,0,-s), Vector(-s,0,s), Vector(s,0,s),
      uPatches,vPatches,
      Material( Vector(.8,.8,.8),0,Vector(1,1,1,25),0,1 )
    );
    //q->tessellate( tess ) ;
    window->scene->addShape( q ) ;


    // CAUSTIC CEILING
    /*
    s*=4;
    float c=5 ;
    q = new Quad( MeshType::Indexed, VertexType::VT10NC10, 
      "ceiling",
      Vector(s,c,-s), Vector(-s,c,-s), Vector(-s,c,s), Vector(s,c,s),
      1,1,
      Material( Vector(0,0,0),0,Vector(0,0,0,25),Vector(1,1,1),Vector(1.33,1.55,1.66) )
    );
    //q->tessellate( tess ) ;
    window->scene->addShape( q ) ;
    */
  }
  
  
  if( sceneNumber == Scenes::PlaneFloorx2 )
  {
    Quad *q;
    real s = 17 ; //6
    uPatches=125,vPatches=125; //5^6*2 tris
    q = new Quad( MeshType::Indexed, VertexType::VT10NC10, "plane floor", Vector(s,0,-s), Vector(-s,0,-s), Vector(-s,0,s), Vector(s,0,s),
      uPatches,vPatches,
      Material( Vector(0.5,0.5,0.5), //Vector(0.01,0.5,0.7),
      0,Vector(0,0,0,25),0,1 ) );
    //q->tessellate( tess ) ;
    window->scene->addShape( q ) ;
    
    // 2nd plane floor, FACES DOWN
    real dist = 8 ;
    q = new Quad( "UPPER plane floor #2", Vector(s,dist,-s), Vector(s,dist,s), Vector(-s,dist,s), Vector(-s,dist,-s), 
      1,1,
      Material( Vector(0.01,0.5,0.7),0,Vector(0,0,0,25),Vector(.9),1 ) );
    //q->tessellate( tess ) ; // blocker SHEET, not tessellated
    window->scene->addShape( q ) ;
  }

  if( sceneNumber == Scenes::MarbleFloor )
  {
    real s = 6 ;
    Material floorMat( Vector(.2,.2,.2),0,Vector(.7,.7,.7,25),0,1 );
    //floorMat.specularJitter = 0.23;
    //floorMat.transmissiveJitter = 0.060;
    Quad *q = new Quad( "plane floor", Vector(s,0,-s), Vector(-s,0,-s), Vector(-s,0,s), Vector(s,0,s),
      uPatches,vPatches,
      floorMat );
    q->tessellate( tess ) ;
    window->scene->addShape( q ) ;
    q->material.perlinGen[ ColorIndex::DiffuseMaterial ] =
      new PerlinGenerator( PerlinNoiseType::Marble, 18, q->aabb->extents().max() ) ;
      
    
  }
  

  if( sceneNumber == Scenes::SmallSpheres )
  {
    int n = 5 ;

    Material m( 0, 0, 0, 1, 1.5 ) ;
    Icosahedron *ic = new Icosahedron( "sms icosahedron", 1, m ) ;

    char bName[255];
    int i = 1 ;
    for( real p = 0 ;  p < 2*PI ; p += (2*PI)/n ) 
    {
      Vector pos = SVector( 3, PI/2, p ).toCartesian() + Vector( 0, 2, 0 );

      sprintf( bName, "sphere %d/%d", i++, n ) ;
      Sphere *sp = Sphere::fromShape( bName, ic, ic->r, 3 ) ;
      sp->transform( Matrix::Translate( pos ) ) ;
      window->scene->addShape( sp ) ;
    }

    DESTROY( ic ) ;
  }
}

void createPointLights( vector<PosNColor>& pts, real sizeEach, real distEach )
{
  // now make shapes based on that.  just usee spheres.
  if( !pts.size() )
    warning( "No cubemap lights to create approx pts out of" ) ;

  for( int i = 0 ; i < pts.size() ; i++ )
  {
    char b[90];
    sprintf( b, "cubemap light %d", i+1 ) ;

    // IT'S JUST EMISSIVE
    Icosahedron* sp = new Icosahedron( b, sizeEach, 
      Material( 0, pts[i].color, 0, 0, 0 ) ) ;

    // Move it out to the point it's supposed to be at.
    // You can scale thsi out further if you wish.
    sp->transform( Matrix::Translate( distEach*pts[i].pos ), Identity ) ;

    window->scene->addShape( sp ) ;
  }
}

void loadCubemap( char* modelname, char* filename )
{
  CubeMap *lt = new CubeMap( modelname, filename,
    3, window->props->getInt( "cubemap::bilinear interpolation" ),
    500 ) ;
  if( !lt->currentSource ) // won't be set if load failed
  {
    error( "Can't load cubemap" ) ;
    return ;
  }

  if( lt->pixelsPerSide != window->fcr->pixelsPerSide )
  {
    warning( "cubemap dim mismatch:  cubemap #px=%d, fullcube renderer #px=%d, changing resolution of cubemap",
    lt->pixelsPerSide, window->fcr->pixelsPerSide ) ;
    lt->downsample( window->fcr->pixelsPerSide ) ;
  }

  lt->faceSave( "C:/vctemp/fullcube/0_TESTCUBESAVE.png" ) ;

  int kept ;
  if( FullCubeRenderer::useRailedQuincunx )
  {
    // RAILED
    info( "Lattice wavelet compression" ) ;
    lt->railedMake() ;
    lt->railedSave( "C:/vctemp/fullcube/railed.png" ) ;

    // At this point (after downsampling) you can generate the IndexMap
    FullCubeRenderer::indexMap.generateRailedLattice( lt ) ;

    // make rotatedColorValues for the light map (since it will be rotated soon)
    kept = lt->railedCompress( FullCubeRenderer::compressionLightmap, true ) ; // this function destroys the original function (wavelet transform is in place).

    lt->railedSaveWaveletRepn( "C:/vctemp/fullcube/0_TESTCUBESAVE_wavelet.png" ) ;
    lt->railedSaveUncompressed( "C:/vctemp/fullcube/0_TESTCUBESAVE_uncomp.png" ) ;
  }
  else
  {
    // NON RAILED
    info( "Haar wavelet compression" ) ;
    kept = lt->faceCompressOriginal( FullCubeRenderer::compressionLightmap, true ) ; // this function destroys the original function (wavelet transform is in place).
    lt->faceSaveWaveletRepn( "C:/vctemp/fullcube/0_TESTCUBESAVE_wavelet.png" ) ;
    lt->faceSaveUncompressed( "C:/vctemp/fullcube/0_TESTCUBESAVE_uncomp.png" ) ;
  }

  int lost = lt->colorValues->size() - kept ;
  info( "Cubemap kept %d / %d, kept %f percent of original components",
    kept, lt->colorValues->size(),
    100.0*kept / lt->colorValues->size() ) ;

  
  
  //lt->isActive = false ;// turn it OFF for now
  window->scene->addCubemap( lt ) ;
  //DESTROY( lt ) ;
}

void generatePointLightsBasedOnCubemap()
{
  CubeMap* lt = window->scene->getActiveCubemap() ;
  if( !lt ) { error( "No cubemap to generate lights based on" ) ; return ; }
  // Make the point light approximation
  info( "Making point light approximation.." ) ;
  vector<PosNColor> pts ;
  real maxmag = 0;
  lt->makePointLightApproximation( 2, window->MAX_LIGHTS, 10000, pts, false, maxmag ) ;
  createPointLights( pts, 10, 125 ) ;
}

// generate a cubemap based on the lights in the scene
void generateCubemapBasedOnSceneLights()
{
  if( window->scene->lights.size() )
  {
    CubeMap* cm = new CubeMap( "cubemap scene lights", 64, window->scene ) ;
    cm->faceCompressOriginal( FullCubeRenderer::compressionLightmap, true ) ;
    window->scene->addCubemap( cm ) ;
  }
  else
    error( "No scene lights to generate cubemap from" ) ;
}

void generateLights( int lightSceneNumber )
{
  // lighting
  // MAIN THREAD

  switch( lightSceneNumber )
  {
    case LightScenes::CubeColor:
      loadCubemap( "colorcube", DIR_BASE( "/res/cubemaps/colorcubeLABELLED.dds" ) ) ;
      break ;

    case LightScenes::CubeColor64:
      loadCubemap( "colorcube64", DIR_BASE( "/res/cubemaps/colorcube64.dds" ) ) ;
      break ;

    case LightScenes::CubeGrace:
      loadCubemap( "grace cross", DIR_BASE( "/res/cubemaps/hdr_grace_cross.dds" ) ) ;
      break ;

    case LightScenes::BlackWhite:
      loadCubemap( "black white", DIR_BASE( "/res/cubemaps/blackWithWhite.dds" ) ) ;
      break ;

    case LightScenes::Teal:
      loadCubemap( "teal", DIR_BASE( "/res/cubemaps/teal256.dds" ) ) ;
      break ;

    case LightScenes::PurpleGalaxy:
      loadCubemap( "purple galaxy", "D:/Dropbox/code/mac/SpaceFighter/ims/myPurple.dds" ) ;
      break ;

    case LightScenes::BlueLobe:
    {
      MathematicalShape *light = new MathematicalShape(
        "bluelobelight", 0, MagFunctions::MagSpherical,
        ColorFunctions::PZBlueLobe, ColorFunctions::PZBlueLobe,
        32,32, Material( 0,1,0,0,1 ) ) ;
      
      light->shProject( light->material.ke ) ;
      light->shGenerateViz( 32, 32, 0 ) ;
      light->transform( Matrix::Translate( 0,0,5 ) ) ;
      window->scene->addShape( light ) ;
    }
    break ;
  
    case LightScenes::RedGreenLobes:
    {
      MathematicalShape *light = new MathematicalShape(
        "redgreenlobes", 0,
        MagFunctions::MagSpherical, ColorFunctions::PYRedPZGreenLobe, ColorFunctions::PYRedPZGreenLobe,
        32,32, Material( 0,1,0,0,1 ) ) ;
      
      light->shProject( light->material.ke ) ;
      light->shGenerateViz( 64, 64, 0 ) ;
      light->transform( Matrix::Translate( 25,-25,25 ) ) ;
      window->scene->addShape( light ) ;

    }
    break ;
  
    case LightScenes::PXWhiteLobe:
    {
      MathematicalShape *light = new MathematicalShape(
        "+x white lobe",
        Vector(0,0,0),
        []( real tElevation, real pAzimuth ) -> real {
          return 4*pow( std::max<real>(sin(tElevation)*cos(pAzimuth),0.0), 33 ) ;
        },
        []( real tElevation, real pAzimuth ) -> Vector {
          return Vector(
            4*pow( std::max<real>(sin(tElevation)*cos(pAzimuth),0.0), 33 ),
            4*pow( std::max<real>(sin(tElevation)*cos(pAzimuth),0.0), 33 ),
            4*pow( std::max<real>(sin(tElevation)*cos(pAzimuth),0.0), 33 )
          ) ;
        },
        64, 64, Material(0,1,0,0,1) ) ;
      
      light->shProject( light->material.ke ) ;
      light->shProjectionViz = light->shProjection->generateVisualization( 64, 64, 0 ) ;
      light->transform( Matrix::Translate( 10, 0, 0 ) ) ;
      window->scene->addShape( light ) ;

    }
    break ;

    case LightScenes::SphereWhite:
    {
      Material whiteEmissive( 0,1,0,0,1 ) ;
      Sphere *light = new Sphere( "spherelightshape",
        0, 8, whiteEmissive, 10, 10 ) ;
      light->shProject( whiteEmissive.ke ) ;
      light->shGenerateViz( 32, 32, 0 ) ;
      light->transform( Matrix::Translate( 20,20,20 ) ) ;
      
      window->scene->addShape( light ) ;
      
    }
    break ;
    
    case LightScenes::HemisphereWhiteX20:
    {
      MersenneTwister::init_genrand( 2000 ) ;

      for( int i = 0 ; i < 20 ; i++ )
      {
        char b[255];
        sprintf( b, "sphere light %d", i+1 ) ;
        
        //Material whiteEmissive( 0,90,0,0,1 ) ; //radiosity check
        Material whiteEmissive( Vector(0,0,0),1,0,0,1 ) ;

        //Sphere *light = new Sphere( b, Vector( randSign(), 1, randSign() )*20*Vector::random(0.5,1), 2, whiteEmissive, 10, 10 ) ;
        real dist=randFloat(38,40);
        
        Sphere *light = new Sphere( b, dist*SVector::randomHemi( Vector(0,1,0) ), 2.45, whiteEmissive, 10, 10 ) ;
        //Sphere *light = new Sphere( b, 0, 0.13, whiteEmissive, 10, 10 ) ;
        light->shaderMotionFunction = [](real t, const Vector& vert)->Vector{
          static real p1 = randFloat(0,2*PI), p2=randFloat( 0, 2*PI ),p3=randFloat( 0, 2*PI ) ;
          return Vector( 5*sin((t+p1)), 3.25+cos(0.5*(t+p2)), 5*sin(3*(t+p3)) ) ;
        } ;
        
        // If the shsamples are NOT catching, (you are getting a 0 projection), then 
        // the solid angle of your light source is too small (no rays are hitting it
        // when cast from the origin)
        // SO try increasing the size of the object, OR 
        light->shProject( light->material.ke ) ;// distance will change the strength of SH lights.. and
        // so will scale.
        // it's a SHAPE based projection, and then that SHAPE is used as a LIGHT.
        // If the shape is big then the light is strong.

        window->scene->addShape( light ) ;
      }
    }
    break ;

    case LightScenes::FlatWhiteX20:
    {
      MersenneTwister::init_genrand( 2000 ) ;
      real s = 10 ;// spread

      for( int i = 0 ; i < 20 ; i++ )
      {
        char b[255];
        sprintf( b, "FLAT sphere light %d", i+1 ) ;
        Material whiteEmissive( 0,1,0,0,1 ) ;
        real dist=randFloat(9,10);
        //FLAT WHITE
        Sphere *light = new Sphere( b, Vector(randFloat(-s,s), dist, randFloat(-s,s) ), 0.45, whiteEmissive, 10, 10 ) ;
        light->shProject( whiteEmissive.ke ) ;
        window->scene->addShape( light ) ;
      }
    }
    break ;

    case LightScenes::Sun:
    {
      Material whiteEmissive( 0,1,0,0,1 ) ; //runtime test
      
      //Sphere *sun = new Sphere( "Sun", Vector( 12, 12, 12 ), 2, whiteEmissive, 10, 10 ) ;
      Sphere *sun = new Sphere( "Sun", Vector( 100, 50, 100 ), 30, whiteEmissive, 10, 10 ) ;
      
      sun->shProject( whiteEmissive.ke ) ;
      
      //window->scene->addFarLight( sun ) ; // VO
      window->scene->addShape( sun ) ; // RT
    }
    break ;

    case LightScenes::CamLight:
    {
      Material whiteEmissive( 0,1,0,0,1 ) ; //runtime test
      
      window->lightForCam = new Sphere( "camLight", Vector( 12, 12, 12 ), 1, whiteEmissive, 10, 10 ) ;
      
      window->lightForCam->shProject( whiteEmissive.ke ) ;
      
      window->scene->addShape( window->lightForCam ) ; // VO
      //window->scene->addShape( camLight ) ;
    }
    break ;

    case LightScenes::Underlamp:
    {
      Material whiteEmissive( 0,200,0,0,1 ) ; //runtime test
      Sphere *lamp = new Sphere( "underlamp", Vector( 0, -25, -5 ), 4, whiteEmissive, 10, 10 ) ;
      lamp->shProject( whiteEmissive.ke ) ;
        
      window->scene->addShape( lamp ) ;
    }
    break ;

    case LightScenes::SkyDome:
    {
      Sphere * sky ;           // the outer sphere (really huge) for creating an atmosphere. needs improve.
      sky = new Sphere( "sky", 0, 25, Material( 0, 1, 0, 0, 1.33 ), 12, 12 ) ;
      
      ////sky->material.perlinGen[ ColorIndex::Emissive ] =
      ////new PerlinGenerator( PerlinNoiseType::Sky, 4,
      ////  sky->aabb->extents().max() ) ;

      window->scene->addShape( sky ) ;
    }
    break ;

    case LightScenes::SphereLightX4:
    {
      for( int i = 0 ; i < 4 ; i++ )
      {
        char b[255];
        sprintf( b, "sphere light %d", i+1 ) ;
        
        Material whiteEmissive( 0,randInt(1,10),0,0,1 ) ;
        Sphere *light = new Sphere( b, SVector( 25, RADIANS(45), 2*PI*(i/4.0) ).toCartesian(), 2, whiteEmissive, 10, 10 ) ;
        light->shProject( whiteEmissive.ke ) ;
        
        window->scene->addShape( light ) ;
      }
    }
    break ;
  }
}

void loadScene()
{
  char buf[ MAX_PATH ] ;
  sprintf( buf, "OUT.wsh" ); // write a default file
  
  if( SAVING && window->open( "Choose a .wsh file to open", DEFAULT_DIRECTORY, DEFAULT_FILTER, buf ) )
  {
    // load entire scene from file
    window->load( buf ) ;
  }
  else
  {
    // wavegen
    for( int i = 0 ; i < 50 ; i++ )
    {
      Vector longAxis = SVector::random(1).toCartesian() ;
      Vector transAxis = longAxis.getRandomPerpendicular() ;

      // WOW that looks even better, IF YOU DISPLACE ALONG BOTH
      // LONGITUDINGAL AND TRANSVERSELY
      //Vector longAxis = SVector( 1, RADIANS(90), randFloat(0, 2*PI) ).toCartesian() ;
      //Vector transAxis(0,1,0);

      ////real freq = randFloat( 0.05, 0.2 ) ; /// OCEAN (slow)
      ////real amp = randFloat( 0.01, 0.05 ) ; /// OCEAN (slow)

      // Pool (best)
      real freq = randFloat( 0.01, 4 ) ;
      real amp = randFloat( 0.01, 0.1 ) ;

      // single frequency
      ////real freq = 4 ;
      ////real amp = randFloat( 0.01, 0.1 ) ;

      // More realistic pool Better for PIXEL SHADER
      ////real freq = randFloat( 0.01, 8 ) ;
      ////real amp = 1/freq ;
      ////clamp( amp, 0., 1. ) ;
      
      // Shadow swaying
      // lf, low amp
      //real freq = randFloat( 0.5, 1 ) ;
      //real amp = 1/(4*freq) ;
      // the more "vertical" it is, the LARGER it's amplitude
      ///real amp = fabs( longAxis.Dot( Vector(0,1,0) ) )/25 ;

      // the more "vertical" it is, the smaller it's amplitude
      //amp = 0.004 + ( 1 - fabs( longAxis.Dot( Vector(0,1,0) ) ) )/4 ; 

      // the more "vertical" it is, the LARGER it's amplitude
      //amp += 0.2*fabs( longAxis.Dot( Vector(0,1,0) ) ) ;

      window->waveGroup.push_back(
        WaveParam( freq, amp, longAxis, transAxis )
      ) ;

      longAxis.writeFloat4( window->gpuCData5.longAxis[ i ] ) ;
      transAxis.writeFloat4( window->gpuCData5.transverseAxis[ i ] ) ;
      window->gpuCData5.timePhaseFreqAmp[ i ][ 2 ] = freq ;
      window->gpuCData5.timePhaseFreqAmp[ i ][ 3 ] = amp ;
    }

    window->updateGPUCData( 5 ) ;

    
    
    // OBJECTS
    //generateScene( Scenes::Bunny ) ;
    //generateScene( Scenes::Teapot ) ;
    //generateScene( Scenes::Lucy ) ;
    //generateScene( Scenes::Tree ) ;
    //generateScene( Scenes::Dragon ) ;
    //generateScene( Scenes::Tearoom ) ;
    //generateScene( Scenes::Shapes ) ;
    //generateScene( Scenes::Shapes1 ) ;
    //generateScene( Scenes::DoubleRefln ) ;
    //generateScene( Scenes::SingleSphere ) ;
    //generateScene( Scenes::GlassPlate ) ;
    //generateScene( Scenes::SingleCube ) ;
    //generateScene( Scenes::SmallSpheres ) ;
    //generateScene( Scenes::CubesX6 ) ;
    generateScene( Scenes::SpheresX4 ) ;
    //generateScene( Scenes::SpheresX25 ) ;

    // FLOORS AND WALLS
    //generateScene( Scenes::Sea ) ;
    //generateScene( Scenes::SeaFloor ) ;
    generateScene( Scenes::PlaneFloor ) ;
    //generateScene( Scenes::PlaneFloorx2 ) ;
    //generateScene( Scenes::Walls ) ;
    //generateScene( Scenes::WallsCeilingLight ) ;
    //generateScene( Scenes::CheckeredPlaneBW ) ;
    //generateScene( Scenes::MarbleFloor ) ;
    //generateScene( Scenes::CheckeredFloor) ;
    //generateScene( Scenes::SolidFloor ) ; // (use plane floor instead)
    //generateScene( Scenes::CornerCase ) ;

    // CREATE THE HEIGHTMAP
    //PerlinGenerator pg( 2, 2, .5, 8, 1, 1, 1 ) ;
    //Model* m = pg.heightmapIndexed( 20, 20, 20, 20, PI/2 ) ;
    //m->transform( Matrix::Translate( -10, 0, -10 ) ) ;
    //window->scene->addShape( m ) ;

    // create the viewing lobe
    // different viewing lobes can be used for different specular effects.
    window->scene->brdfSHSpecular = new MathematicalShape( "sh specular fx lobe", 0,
      MagFunctions::MagPYLobe, ColorFunctions::PYLobe, ColorFunctions::PYLobe,
      32, 32, Material( 1,1,1,0,1 ) ) ;
    
    window->scene->brdfSHSpecular->shProject( 1 ) ;
    //window->scene->brdfSHSpecular->shProjection->generateVisualizationAutoAdd( 32, 32, Vector( 0, 5, 0 ) ) ;
    
    // Shape-based lights
    //generateLights( LightScenes::PXWhiteLobe ) ;
    //generateLights( LightScenes::SphereWhite ) ;
    //generateLights( LightScenes::HemisphereWhiteX20 ) ;
    //generateLights( LightScenes::CamLight ) ;
    generateLights( LightScenes::Sun ) ;
    //generateLights( LightScenes::Underlamp ) ;
    //generateLights( LightScenes::SkyDome ) ;
    //generateLights( LightScenes::SphereLightX4 ) ;

    //generateCubemapBasedOnSceneLights() ;
    
    // Cubemap-based lights
    //generateLights( LightScenes::CubeColor64 ) ;
    //generateLights( LightScenes::CubeGrace ) ;
    //generateLights( LightScenes::PurpleGalaxy ) ;
    //generateLights( LightScenes::CubeColor ) ;
    //generateLights( LightScenes::BlackWhite ) ;
    //generateLights( LightScenes::Teal ) ;
    //generatePointLightsBasedOnCubemap() ; // Need these for WHITTED, VO, 

    window->radCore->assignIds( window->scene );

    if( window->scene->spacePartitioningOn )
      window->scene->computeSpacePartition() ;
  }
}

void testSplit()
{
  Model* m = new Model( "test tri", Material::White ) ;
  m->addTri( Vector(-2,0,1), Vector(1,0,1), Vector(1,0,-1) ) ;
  m->updateMesh() ;

  Triangle&tri = m->meshGroup->meshes[0]->tris[0] ;
  //window->addSolidDebugTri( tri, Vector(1,0,0) ) ;

  list<PhantomTriangle*> lpt, lpt2 ;
  PhantomTriangle *pt = new PhantomTriangle( tri ) ;
  lpt.push_back( pt ) ;
  //drawTris( lpt, Vector(0,0,1) ) ; 

  // test the split tris function.
  // cut them
  Plane p( Vector(0,0,1), 0.8 ) ;
  splitTris( p, lpt, lpt2 ) ;

  info( "split %d tris into %d tris", lpt.size(), lpt2.size() ) ;

  drawTris( lpt2, Vector(1,0,0) ) ;

}

void testMath()
{
  // do 10000 vector adds
  Timer timer ;
  
  Vector sum ;
  for( int i = 0 ; i < 100000 ; i++ )
    sum += Vector::random() ;

  sum.print() ;
  
  printf( "TIME: %f\n", timer.getTime() ) ;
  system( "pause" ) ;
}

void testRandom()
{
  Vector v( 1,0,0 ) ;
  
  window->addDebugLine( 0, 1, v, Vector(1,0,0) ) ;
  for( int i = 0 ; i < 10000 ; i++ )
  {
    // ok
    //window->addDebugLine( 0, Vector(.4,0,0), SVector::random( 1 ).toCartesian(), Vector(1,0,0) ) ;
    //window->addDebugPoint( SVector::random( 1 ).toCartesian(), Vector(1,0,0) ) ;

    //window->addDebugPoint( SVector::randomHemiCosine( 1 ).toCartesian(), Vector(1,0,0) ) ;
    //window->addDebugPoint( SVector::randomHemiCosine( Vector(1,1,1).normalize() ), Vector(1,0,0) ) ;

    // SHOWS RANDOM SPHERE
    //Vector p = SVector::random( 1 ).toCartesian() ;
    //Vector c = Vector::random() ;
    //window->addDebugPoint( p, c ) ;

    // test the pionts DISK LIMITED.BAD
    //Vector r = v.jitterC( randFloat(-1,1) ) ;
    //Vector r = v.getRandomPerpendicular() ;
    //Vector pt = v+( sqrt( randFloat() ) )*r ;
    //window->addDebugPoint( pt.normalize(), 1 ) ;

    // working axis stuff
    //Vector r = v.jitter( PI/4 ) ;
    //window->addDebugPoint( r, 1 ) ;
    //window->addDebugLine( 0, Vector(1,0,0), r, Vector(1,1,0) ) ;

    Vector y(0,1,0) ;
    Vector r = y.jitter( 1e-6 ) ;
    window->addDebugLine( 0, Vector(1,0,0), r, Vector(1,1,0) ) ;

    // Test that randomperpendicular is in a disk
    //Vector r = v.getRandomPerpendicular() ;
    //window->addDebugLine( v, 1, v+r, Vector(1,0,0) ) ;
  }
}

void testSHRuntime()
{
  Timer t ;
  int n = 1000000 ;
  real sum = 0.0 ;
  for( int i = 0 ; i < n ; i ++ )
  {
    //sum += SH::plegendre( 8, 8, randFloat() ) ;
    sum += SH::Tlm( 8, 8, randFloat() ) ;
  }

  printf( "Took %f seconds for %d trials", t.getTime(), n ) ;
}

void testSH()
{
  // generate a viz for each l,m you have created
  for( int l = 0; l < window->shSamps->bands ; l++ )
  {
    for( int m = -l ; m <= l ; m++ )
    {
      //info( "generating sh l=%d, m=%d", l,m ) ;
      
      //SH::Y = SH::YCheby ;
      SH::Y = SH::YSH ;

      window->scene->addShape( SH::generateBasisFuncVisualization( l, m, 64, 64, Vector( m, l, 0 ) ) );

      printf( "Generating viz band l=%d, m=%d       \r", l,m ) ;
    }
  }
  return ;

  MersenneTwister::init_genrand( time(0) ) ;
  CubeMap* cubeMap = new CubeMap( "grace cross", DIR_BASE( "/res/cubemaps/hdr_grace_cross.dds" ),
    1, window->props->getInt( "cubemap::bilinear interpolation" ),
    500 ) ;
  window->scene->lights.push_back( cubeMap ) ;

  SHVector *lightFcn = cubeMap->shProjection ;
  lightFcn->generateVisualization( 128, 128, Vector(0,0,0) ) ;
  plain( "SH CUBE MAP:\n" ) ;
  lightFcn->printZonal() ;

  MathematicalShape* lobeShape = new MathematicalShape( "ms", 0,
    MagFunctions::MagPYLobe, ColorFunctions::ConstantWhite, ColorFunctions::PYLobe,
    32,32, Material( 1,1,1,0,1 ) ) ;
  window->scene->addShape( lobeShape ) ;
  lobeShape->shProject( 1 ) ;
  
  SHVector *lobe = lobeShape->shProjection ;
  lobe->makeZonal() ;
  //plain( "SH lobe:\n" ) ;
  //lobe->printZonal() ;
  ///lobe->generateVisualization( 32, 32, Vector(0,0,0) ) ;

  // try intersecting them by subtraction.
  SHVector *notSeen = lightFcn->subbedFloorCopy( lobe ) ;
  //plain( "DIFF\n" ) ;
  //diff->printZonal() ;

  // now do 
  SHVector *seen = lightFcn->subbedCopy( notSeen ) ;
  seen->generateVisualization( 32, 32, Vector( 0, -4, 0 ) ) ;

}

void init()
{
  //srand( 121 ) ;
  MersenneTwister::init_genrand( 6 ) ;

  window->camera->setPrepos( 0 ) ;

  loadScene() ;
  //testSplit() ;
  //testSHRuntime() ;
  //testSH() ;
  //testRandom() ;
  //testMath() ;
  
}

void update()
{
  real stepSpeed = 0.01, rotateSpeed=0.005 ;

  ///window->scene->moveLights() ;

  // shift speed boost
  if( IS_KEYDOWN( VK_SHIFT ) )  stepSpeed *= 10 ;

  if( window->movementOn )
  {
    if( window->keyIsPressed( VK_UP ) || window->keyIsPressed( 'W' ) )
    {
      window->camera->stepForward( stepSpeed ) ;
    }
  
    if( window->keyIsPressed( VK_LEFT ) || 
        window->keyIsPressed( 'A' ) )//same
    {
      window->camera->stepSide( -stepSpeed ) ;
    }
  
    if( window->keyIsPressed( VK_DOWN ) ||
        window->keyIsPressed( 'S' ) )
    {
      window->camera->stepForward( -stepSpeed ) ;
    }

    if( window->keyIsPressed( VK_RIGHT ) ||
        window->keyIsPressed( 'D' ) )
    {
      window->camera->stepSide( stepSpeed ) ;
    }

    if( window->keyIsPressed( 'Q' ) )
    {
      window->camera->roll( -rotateSpeed ) ; // roll about forward axis ccw
    }
    if( window->keyIsPressed( 'E' ) )
    {
      window->camera->roll( rotateSpeed ) ;
    }
    
    // mouse motion
    real mouseDx = window->getMouseDx() ;
    real mouseDy = window->getMouseDy() ;
    
    //info( "mouse %f %f", mouseDx, mouseDy ) ;
    window->camera->stepYaw( -mouseDx*rotateSpeed ) ;
    window->camera->stepPitch( -mouseDy*rotateSpeed ) ;
  }


  // 
  /*
  // spinning
  window->camera->right = Vector( 1,0,0 ) ;
  window->camera->up = Vector( 0,1,0 ) ;
  window->camera->forward = Vector( 0,0,-1 ) ;
  
  static float s = 0.0f ;
  s += 0.001f ;
  window->camera->eye = Vector( 0, 35, 35 ) * Matrix::RotationY( s )  ;

  window->camera->setLook( 0,5,0 ) ;
  */

  window->step() ;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow )
{
  int windX = +32, windY = 32, windWidth=800, windHeight=600;

  // You should have to create the threadpool. singletons are bad.
  threadPool.mainThreadId = GetCurrentThreadId() ;
  threadPool.mainThread = GetCurrentThread() ;

  // Start up the log.
  logStartup( "C:/vctemp/builds/gtp/lastRunLog.txt", "gtp", windX, windY, windWidth, windHeight ) ;
  
  // Prepare the factorials table.
  prepFactorials() ;

  // Setup the window
  window = new GTPWindow( hInstance, TEXT( "gtp" ),
    windX, windY, // x pos, y pos
    windWidth, windHeight// width, height
  ) ;

  init() ;
  window->programState = Idle ;  // no longer busy

  MSG msg;

  while( 1 )
  {
    if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
      if( msg.message == WM_QUIT )
        break;

      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }
    else
    {
      // if the program is running a render (busy),
      // then try and slow down a bit
      if( window->programState == ProgramState::Busy )
        Sleep( 100 ) ;
      update();

      ///if( window->rtCore->realTimeMode )
      ///{
      ///  // SKIP the rasterizer loop
      ///  window->drawRaytraceOnly() ;
      ///}
      ///else
      {
        window->draw();
      }

      threadPool.execJobMainThread() ; // do only ONE job from the main thread, then loop around again.
      //while( threadPool.execJobMainThread() ) ; // keep running mainthread queued jobs until all done.
      logflush() ; // flush the output log
    }
  }
  
  // kill all threads
  //delete threadPool ;
  //threadPool.pause() ; // pause all threads, because we're about to delete all shared resources
  threadPool.die() ; // must die before window dies
  DESTROY( window ) ;// make window NULL too.

  ////_CrtDumpMemoryLeaks();

  bail( "* * This Computer Program Has Ended * *\n", false ) ; // shuts down log and calls exit process (which terminates running threads)

  // draw to a backbuffer on a separate thread.
  // replace "drawing" message in window with image
  // when done.

  return msg.wParam;
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch( message )
  {
  case WM_CREATE:
    //Beep( 60, 10 );
    return 0;

  case WM_PAINT:
    {
      HDC hdc;
      PAINTSTRUCT ps;
      hdc = BeginPaint( hwnd, &ps );
      EndPaint( hwnd, &ps );
    }
    return 0;

  case WM_INPUT: 
    {
      #pragma region pick up the raw input
      // no processing code here code here
      UINT dwSize;

      GetRawInputData( (HRAWINPUT)lparam, RID_INPUT,
                        NULL, &dwSize, sizeof( RAWINPUTHEADER ) ) ;
      LPBYTE lpb = new BYTE[ dwSize ] ;
      if( lpb == NULL )
      {
        return 0;
      }

      if( GetRawInputData( (HRAWINPUT)lparam, RID_INPUT,
          lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize )
      {
        error( "GetRawInputData doesn't return correct size !" ) ;
      }

      RAWINPUT *raw = (RAWINPUT*)lpb;

      // Check if it was keyboard or mouse input.
      if (raw->header.dwType == RIM_TYPEKEYBOARD) 
      {
        // We don't take keyboard input here.
        // We take it by using GetKeyboardState() function
        // in the window->step() function.
        //printRawKeyboard( raw ) ; // just so you can see
      }
      else if (raw->header.dwType == RIM_TYPEMOUSE)
      {
        //printRawMouse( raw ) ;  // just so you can see
        window->mouseUpdateInput( raw ) ;
      } 

      delete[] lpb ;
      #pragma endregion
    }
    return 0;

  case WM_LBUTTONDOWN:
    {
      int x = LOWORD( lparam ) ;
      int y = HIWORD( lparam ) ;
      //info( "Click @ %d %d", x,y );

      //rppStratifiedTest() ;
      // Ray hit testing
      //hittest( x,y ) ;
    }
    return 0 ;

  case WM_KEYDOWN:
    switch( wparam )
    {
    case VK_ESCAPE:
      PostQuitMessage( 0 );
      break;
    default:
      break;
    }
    return 0;

  case WM_SETFOCUS:
    //info( "Got the focus" ) ;
    // LAST (after WM_ACTIVATE)
    break ;

  case WM_KILLFOCUS:
    //info( "Lost the focus" ) ;
    // LAST (after WM_ACTIVATE)
    break;

  case WM_ACTIVATE:
    if( LOWORD(wparam) == WA_ACTIVE )
    {
      //info( Magenta, "Window active" ) ;
      //1: This happens first
      // activate gets called on startup before the ctor
      // returns
      if( window )  window->activate() ;
    }
    else
    {
      //info( Magenta, "Window DEACTIVATED" ) ;
      if (window)  window->deactivate() ;
    }
    break ;

  case WM_CHAR:
    #pragma region character processing
    switch( wparam )
    {
    case 'b':
      // caustic width
      window->gpuCData0.vo[3] += 0.1f ;
      break ;

    case 'B':
      window->gpuCData0.vo[3] -= 0.1f ;
      break ;

    case 'c':
      {
        {
          MutexLock M( window->scene->mutexDebugLines, INFINITE ) ;
        
          window->debugPoints.clear();
          window->debugLines.clear() ;
          window->debugTris.clear() ;

          window->debugLinesChanged = window->debugPointsChanged = window->debugTrisChanged = true ;
        }
        {
          window->clearVisualizations() ;
        }
        {
          // reset mesh colors from intersection tests
          /////MutexLock LOCK( window->scene->mutexShapeList, INFINITE ) ;
          /////for( int i = 0 ; i < window->scene->shapes.size() ; i++ )
          /////{
          /////  window->scene->shapes[i]->setColor( ColorIndex::DiffuseMaterial, Vector( 1,1,1 ) ) ;
          /////  window->scene->shapes[i]->updateMesh() ;
          /////}
        }
      }
      break;

    case 'g':
      {
        Camera* cam = window->camera ;
        info( Gray, "eye=Vector(%f,%f,%f);\nforward=Vector(%f,%f,%f);\nright=Vector(%f,%f,%f);\nup=Vector(%f,%f,%f);",
          cam->eye.x, cam->eye.y, cam->eye.z, 
          cam->forward.x, cam->forward.y, cam->forward.z, 
          cam->right.x, cam->right.y, cam->right.z, 
          cam->up.x, cam->up.y, cam->up.z
        ) ;
      }
      break ;

    case 'h':
      window->renderScenery = !window->renderScenery ;
      info( "Scenery %s", onOrOff( window->renderScenery ) ) ;
      break ;

    case 'i':
      window->gpuCData0.voI[0] += 0.1f ;
      // OFF because if you hit 'C' by accident, this takes a long time
      //////info( Green, "Lets get some visualizations up." ) ;
      //////
      //////// generate the model and add it to the scene.
      //////for( int i = 0 ; i < window->scene->shapes.size() ; i++ )
      //////{
      //////  threadPool.createJob( "viz", new CallbackObject0<Shape*, void(Shape::*)()>
      //////    ( window->scene->shapes[i], &Shape::shVisualize ) ) ;
      //////}
      
      //info( Green, "Lets get some visualizations up." ) ;
      //// generate the model and add it to the scene.
      //for( int i = 0 ; i < window->scene->shapes.size() ; i++ )
      //{
      //  threadPool.createJob( "viz", new CallbackObject0<Shape*, void(Shape::*)()>
      //    ( window->scene->shapes[i], &Shape::shVisualize ) ) ;
      //}
      break ;

    case 'I':
      window->gpuCData0.voI[0] -= 0.1f ;
      break ;

    case 'j':
      window->gpuCData0.voI[2] += 0.1f ;
      break ;

    case 'J':
      window->gpuCData0.voI[2] -= 0.1f ;
      break ;

    case 'k':
      threadPool.resume() ;
      window->programState = ProgramState::Busy ;
      break ;

    case 'K':
      // stop the threads
      threadPool.pause() ;
      window->programState = ProgramState::Idle ;
      break ;
    
    case 'l':
      window->gpuCData0.voI[3] += 1 ;
      break ;

    case 'L':
      window->gpuCData0.voI[3] -= 1 ;
      break ;

    case 'm':
      window->toggleMovement() ;
      if( window->movementOn )
        info( "You can move again, WASD, arrow keys and mouse" ) ;
      else
        info( "Freezing camera.  You can't move now.  Press 'm' to move again" ) ;
      break;

    case 'M':
      {
        if( window->camera == window->viewCam )
          window->camera = window->lightCam ;
        else
          window->camera = window->viewCam ;

        // delete/rebuildoctree memtest
        //window->scene->computeSpacePartition() ;

        ///if( window->scene->spAll )
        ///  window->scene->spAll->root->generateDebugLines( 0, Vector(1,0,1) ) ;
        ///else
        ///  warning( "No octree, no viz" ) ;
        //window->scene->spExact->root->generateDebugLines( 0, Vector(0,1,0) ) ;
        //window->scene->spMesh->root->generateDebugLines( 0, Vector(1,1,0) ) ;
        //for( int i = 0 ; i < window->scene->shapes.size() ; i++ )
        //  window->scene->shapes[i]->aabb->generateDebugLines( Vector(0,0,1) ) ;

        // show the phantoms
        ////vector<ONode<PhantomTriangle*>*> pts = window->scene->spMesh->all() ;
        ////for( int i = 0 ; i < pts.size() ; i++ )
        ////{
        ////  for( list<PhantomTriangle*>::iterator iter = pts[i]->items.begin() ; 
        ////       iter != pts[i]->items.end() ;
        ////       ++iter )
        ////  {
        ////    //window->addDebugTriLock( *iter, Vector(0,1,0) ) ;
        ////    window->addSolidDebugTri( *iter, Vector(0,1,0) ) ;
        ////  }
        ////}
      }
      break ;

    case 'n':
      // check the normals
      {
        info( White, "Displaying vertex normals AS IN TRIANGLES ARRAY, blue and white" ) ;
        MutexLock LOCK( window->scene->mutexDebugLines, INFINITE ) ;
        for( int i = 0 ; i < window->scene->shapes.size() ; i++ )
        {
          Shape *s = window->scene->shapes[i];
          
          s->meshGroup->each( [](Mesh*mesh){
            for( int j = 0 ; j < mesh->tris.size() ; j++ )
            {
              Triangle& tri = mesh->tris[j] ;
              Vector& n = tri.normal ;
              window->addDebugLine( tri.a, Vector(0,0,1), tri.a + .5*n, Vector(1,1,1) ) ;
              window->addDebugLine( tri.b, Vector(0,0,1), tri.b + .5*n, Vector(1,1,1) ) ;
              window->addDebugLine( tri.c, Vector(0,0,1), tri.c + .5*n, Vector(1,1,1) ) ;
            }
          });
        }
      }
      break ;

    case 'N':
      {
        info( Yellow, "Displaying vertex normals, red and yellow" ) ;
        MutexLock LOCK( window->scene->mutexDebugLines, INFINITE ) ;
        for( int i = 0 ; i < window->scene->shapes.size() ; i++ )
        {
          Shape *s = window->scene->shapes[i];
          s->meshGroup->each( [](Mesh*mesh){
            for( int j = 0 ; j < mesh->verts.size() ; j++ )
            {
              AllVertex& v = mesh->verts[j] ;
              window->addDebugLine( v.pos, Vector(1,0,0), v.pos + v.norm, Vector(1,1,0) ) ;
            }
          });
        }
      }
      break ;
    
    case 'o':
      window->gpuCData0.voI[1] += 0.1f ;
      break ;

    case 'O':
      window->gpuCData0.voI[1] -= 0.1f ;
      break ;

    case 'p':
      window->screenshot() ;
      break;
      
    case 'P':
      {
        // take 5 shots, moving the rotation angle 
        window->screenshotMulti( 6, .05, 0.1 ) ;
        // render to cubemaps and save
        ////window->fcr->renderAt( window->camera->eye ) ;
        ////Hemicube hemi( window, 1, 256 ) ;
        ////hemi.renderToSurfaces( window->scene, window->camera->eye, window->camera->forward, window->camera->up ) ;
        ////hemi.saveSurfacesToFiles() ;
      }
      break ;

    case 'r':
      if( window->programState != ProgramState::Busy )
      {
        info( ">> Tracing.." ) ;
        window->programState = ProgramState::Busy ;
        window->bakeRotationsIntoLights() ;
        
        window->rtCore->raytrace(
          window->camera->eye,
          window->camera->getLook(),
          window->camera->up,
          window->scene
        ) ;
      }
      else info( "Busy" ) ;
      break;

    case 'R':
      window->rtCore->realTimeMode = !window->rtCore->realTimeMode ;
      info( "Real-time ray trace mode %s", onOrOff( window->rtCore->realTimeMode ) ) ;
      break;

    case 't':
      ///window->rtCore->showBg = !window->rtCore->showBg ;
      ///info( "RTCore show bg %s", onOrOff( window->rtCore->showBg ) ) ;
      break ;

    case 'T':
      // camlight move mode.  You control the light source
      break ;

    case 'v':
      // change shadow width
      if( window->gpuCData0.vo[1] > 0.01f )
        window->gpuCData0.vo[1] += 0.01f ;
      else
        window->gpuCData0.vo[1] += 0.001f ;
       
      break ;

    case 'V':
      if( window->gpuCData0.vo[1] > 0.01f )
        window->gpuCData0.vo[1] -= 0.01f ;
      else
        window->gpuCData0.vo[1] -= 0.001f ;
      break ;

    case 'x':
      
      if( window->expos > 0.05 )
        window->expos+=.01 ;
      else
        window->expos+=.001 ;
      break ;

    case 'X':
      if( window->expos > 0.05 )
        window->expos-=.01 ;
      else
        window->expos-=.001 ;
      break ;

    case 'y':
      cycleFlag( window->shSamps->bandsToRender, 1, window->shSamps->bands ) ;
      break ;

    case 'Y':
      decycleFlag( window->shSamps->bandsToRender, 1, window->shSamps->bands ) ;
      break;
    
    case 'z':
      cycleFlag( window->gpuCData0.vo[0], 0, 2 ) ;
      break ;

    case 'Z':
      decycleFlag( window->gpuCData0.vo[0], 0, 2 ) ;
      break ;

    case '1':
      cycleFlag( window->shadingMode, 0, ShadingMode::WaveletShaded ) ;
      break ;  

    case '!':
      decycleFlag( window->shadingMode, 0, ShadingMode::WaveletShaded ) ;
      break; 

    case '2':
      // toggle wireframe
      window->toggleFillMode() ;
      break ;

    case '3':
      // toggle rasterizer
      cycleFlag( window->renderingMode, 1, 8 ) ;
      printf( "Rendering mode %s", RenderingModeName[ window->renderingMode ] ) ;
      break ;

    case '#':
      decycleFlag( window->renderingMode, 1, 8 ) ;
      break ;

    case '4':
      cycleFlag( window->textDisplay, 0, 1 ) ;
      break ;

    case '$':
      // toggle tex sampling
      cycleFlag( window->ss, 0, TextureSamplingMode::AnisotropicSampling ) ;
      window->setTextureSamplingMode( (TextureSamplingMode)window->ss ) ;
      break ;

    case '5':
      {
        // toggle cubemap
        CubeMap* cm = window->scene->getActiveCubemap() ;
        if( cm )
        {
          cm->isActive = ! cm->isActive ;
          info( "Cubemap %s", onOrOff( cm->isActive ) ) ;
          DESTROY( window->scene->shCombinedViz ) ; // invalidate this, so it gets remade
        }
        else
          info( "No active cubemap" ) ;
      }
      break ;
    
    case '%':
      {
        // 
      }
      break ;

    case '6':
      window->subwindow = !window->subwindow ;
      info( "Subwindow %s", onOrOff( window->subwindow ) ) ;
      break ;
    
    case '^':
      window->renderSHLight = !window->renderSHLight ;
      info( "SH repn %s", onOrOff( window->renderSHLight ) ) ;
      break ;
     
    case '7':
      if( window->programState != ProgramState::Busy )
      {
        window->programState = ProgramState::Busy ;
        
        window->shadingMode = ShadingMode::Radiosity ;
        window->renderingMode = RenderingMode::Rasterize ;
        
        info( Red, ">> Starting radiosity" ) ;

        // just a one off render
        window->radCore->radiosity( window->scene, 
          FormFactorDetermination::FormFactorsHemicube,
          MatrixSolverMethod::GaussSeidel ) ;
        
        // Does several snapshots of radiosity
        //window->radCore->scheduleRadiosity( window->scene,
        //  FormFactorDetermination::FormFactorsHemicube, MatrixSolverMethod::GaussSeidel,
        //  8, 10 ) ;
      }
      else info( "Busy" ) ;
      break ;

    case '&':
      //if( window->programState != ProgramState::Busy )
      {
        window->programState = ProgramState::Busy ;
        // multiple runs are allowed, because you may have
        // rotated the light sources
        info( Red, ">> Starting radiosity raycasted" ) ;
        window->radCore->radiosity( window->scene,
          FormFactorDetermination::FormFactorsRaycast, MatrixSolverMethod::GaussSeidel ) ;
      }
      //else info( "Busy" ) ;
      break ;

    case '8':
      //if( window->programState != ProgramState::Busy )
      {
        window->programState = ProgramState::Busy ;
        static bool alreadyRan = false ;
        if( alreadyRan )
        {
          info( "VO already ran before" ) ;
          window->programState = ProgramState::Idle ;
          break ;
        }
        alreadyRan=true; 

        info( Red, ">> Starting vo raycasted" ) ;
        window->radCore->vectorOccluders( window->scene, FormFactorDetermination::FormFactorsRaycast ) ;
      }
      //else info( "Busy" ) ;
      break ;

    case '*':
      /*
      if( window->programState != ProgramState::Busy )
      {
        window->programState = ProgramState::Busy ;
        static bool alreadyStarted = false ;
        
        if( alreadyStarted )
        {
          info( "computation already started" ) ;
          window->programState = ProgramState::Idle ;
          break ;
        }
        alreadyStarted=true; 

        window->timer.start() ;
        info( Red, ">> Starting vector occluders" ) ;
        window->radCore->vectorOccluders( window->scene, FormFactorDetermination::FormFactorsHemicube ) ;
      }
      */
      break ;

    case '9':
      if( window->programState != ProgramState::Busy )
      {
        window->programState = ProgramState::Busy ;
        static bool alreadyRan = false ;
        
        if( alreadyRan )
        {
          info( "wavelet computation already started" ) ;
          window->programState = ProgramState::Idle ;
          break ;
        }
        alreadyRan=true; 

        // With large # verts this takes too much ram.
        //window->doInterreflect = false ; // do you want bounced interreflections?

        info( "Wavelet/full cube renders" ) ;
        window->fcr->renderCubesAtEachVertex( CompressionMode::HaarWaveletCubemap ) ;
      }
      else info( "Busy" ) ;
      break ;

    case '(':
      if( window->programState != ProgramState::Busy )
      {
        window->programState = ProgramState::Busy ;
        window->doInterreflect = false ; // do you want bounced interreflections? (VERY EXPENSIVE MEMORYWISE FOR WAVELET)

        info( Red, ">> Starting SH/full cube renders" ) ;
        window->fcr->renderCubesAtEachVertex( CompressionMode::SphericalHarmonics ) ;
      }
      else info( "Busy" ) ;
      break ;

    case '0':
      //if( window->programState != ProgramState::Busy )
      {
        window->programState = ProgramState::Busy ;
        static bool alreadyStarted = false ;
        if( alreadyStarted )
        {
          info( "%s computation already run", SH::kernelName ) ;
          window->programState = ProgramState::Idle ;
          break ;
        }
        alreadyStarted=true; 

        info( ">> Starting %s computation", SH::kernelName ) ;
        
        window->shCompute() ;
      }
      //else info( "Busy" ) ;
      break ;

    case '[':
      window->subwindowBackDist-=1;  // move back
      break ;
    case ']':
      window->subwindowBackDist+=1;  // move forward
      break ;

    case '=':
    case '+':
      // change rotation
      //window->rot+=0.01 ;
      window->rot++;
      break ;

    case '-':
      // change rotation
      //window->rot-=0.01 ;
      window->rot-- ;
      break ;

    case '/':
      // lucy right side
      cycleFlag( window->camera->prepos, 0, 13 ) ;
      window->camera->setPrepos( window->camera->prepos ) ;
      break ;
    
    case '?': // reset to default camera state
      window->camera->setPrepos( 5 ) ;
      break ;  

    case '.':
      window->camera->setPrepos( 1 ) ;
      break ;

    case ',':
      window->camera->setPrepos( 8 ) ;
      break ;

    case '<':
      //window->scene->spAll->root->generateDebugLines( 0, Vector(1,0,1) ) ;
      //window->scene->spExact->root->generateDebugLines( 0, Vector(0,1,0) ) ;
      //window->scene->spMesh->generateDebugLines( Vector(1,1,0) ) ;

      // shapse1 with torus as ellipse
      window->camera->setPrepos( 11 ) ;
      break ;

    case '>':
      // dragon view
      window->camera->setPrepos( 10 ) ;
      break ;

    }
    #pragma endregion
    return 0 ; // WM_CHAR

  case WM_SIZE:
    {
      int width = LOWORD( lparam ) ;
      int height = HIWORD( lparam ) ;
      info( "> RESIZED TO width=%d height=%d\n", width, height ) ;
    }
    break; // don't return

  case WM_SYSCOMMAND:
    {
      switch( wparam )
      {
      case SC_MOVE:
        // moving the window
        // don't interfere with this one, otherwise your window
        // won't move normally when the user tries to move it!
        break;

      case SC_SCREENSAVE:     // screensaver wants to begin
        info( "! Stopping the screensaver.." ) ;
        return 0;           // returning 0 PREVENTS those things from happening

      case SC_MONITORPOWER:   // monitor wants to shut off - powersaver mode
        info( "! Stopping the monitor power saver.." ) ;
        return 0;           // returning 0 PREVENTS monitor from turning off
      } // end wparam inner switch
    } //end case WM_SYSCOMMAND
    break ;
  
  case WM_DESTROY:
    PostQuitMessage( 0 ) ;
    return 0;
  }

  return DefWindowProc( hwnd, message, wparam, lparam );
}

void hittest( int x, int y )
{
  // cast a ray
  MutexLock LOCK( window->scene->mutexDebugLines, INFINITE ) ;
  MutexLock LOCK2( window->scene->mutexShapeList, INFINITE ) ;

  //Ray ray( window->camera->eye, (window->camera->getLook() - window->camera->eye).normalize(), 2000 ) ;
      
  // shoot a ray from mouse click pos into scene

  // move the viewing plane
  window->rtCore->viewingPlane->persp( 45, (real)window->rtCore->viewingPlane->cols / window->rtCore->viewingPlane->rows, 1, 1000 ) ;
  //window->rtCore->viewingPlane->orient( window->camera->eye, window->camera->getLook(), window->camera->up ) ;
  window->rtCore->viewingPlane->orient( window->camera->eye, window->camera->right, window->camera->up, window->camera->forward ) ;
  Ray ray = window->rtCore->viewingPlane->getRay( y, x, 0, 0 ) ;
      
  window->addDebugLine( ray.startPos, Vector(1,1,0), ray.getEndPoint(), Vector( 1,1,0 ) ) ;

  window->addDebugLine( window->rtCore->viewingPlane->a, Vector(1,0,0), window->rtCore->viewingPlane->b, Vector( 1,1,0 ) ) ;
  window->addDebugLine( window->rtCore->viewingPlane->b, Vector(1,1,0), window->rtCore->viewingPlane->c, Vector( 1,0,1 ) ) ;
  window->addDebugLine( window->rtCore->viewingPlane->c, Vector(1,0,1), window->rtCore->viewingPlane->d, Vector( 1,1,1 ) ) ;
  window->addDebugLine( window->rtCore->viewingPlane->d, Vector(1,1,1), window->rtCore->viewingPlane->a, Vector( 1,0,0 ) ) ;
        
  // draw octree nodes we hit.
  vector<ONode<Shape*>*> exactNodes ;
  window->scene->spExact->intersectsNodes( ray, exactNodes ) ;
  info( "%d EXACT shape nodes hit", exactNodes.size() ) ;
  //for( list<ONode<Shape*>*>::iterator iter = exactNodes.begin() ; iter != exactNodes.end(); ++iter )
  for( auto node : exactNodes )
  {
    node->generateDebugLines( Vector(1,0,0) ) ;
    // change hit shapes red
    for( auto shape : node->items )
      shape->setColor( ColorIndex::DiffuseMaterial, Vector(1,0,0) )->updateMesh() ;
  }

  vector<ONode<PhantomTriangle*>*> meshNodes ;
  window->scene->spMesh->intersectsNodes( ray, meshNodes ) ;
  info( "%d mesh nodes hit", meshNodes.size() ) ;
  set<Mesh*> dirtyMeshes ;
  for( auto node : meshNodes )
  {
    node->generateDebugLines( Vector(1,0,0) ) ;

    // colorize the hit triangles
    for( auto pt : node->items )
    {
      if( pt->tri->intersects( ray, NULL ) )
        pt->tri->setColor( ColorIndex::DiffuseMaterial, Vector( 1,0,0 ) ) ;
      else
        // candidates that were missed
        pt->tri->setColor( ColorIndex::DiffuseMaterial, Vector(1,0,1) ) ;
      dirtyMeshes.insert( pt->tri->meshOwner ) ;
    }
  }


  // after colorizing, just update the MESHES that got intersected
  for( set<Mesh*>::iterator iter = dirtyMeshes.begin() ; iter!=dirtyMeshes.end() ; ++iter )
    (*iter)->updateVertexBuffer() ;
}

void rppStratifiedTest()
{
  #if 0
  // use the "strat" and gen dots
  MutexLock me( window->scene->mutexDebugLines, INFINITE ) ;
      
  int rpp = 150 ;
  int rootRPP = (int)sqrt( (float)rpp ) ;
  int nbins = rootRPP*rootRPP ;

  Vector a(-1,-1), b(-1,1), c(1,1), d(1,-1);
  window->addDebugLine( a, 1, b, 1 ); 
  window->addDebugLine( b, 1, c, 1 ); 
  window->addDebugLine( c, 1, d, 1 ); 
  window->addDebugLine( d, 1, a, 1 ); 
  for( int c = 0 ; c < nbins ; c++ )
  {
    // subrow and subcol are the mini bins within the pixel
    int subrow = c / rootRPP ;
    int subcol = c % rootRPP ;

    Vector rowBounds( -1 + subrow*(2./rootRPP), -1 + (subrow+1)*(2./rootRPP), 0 ),
            colBounds( -1 + subcol*(2./rootRPP), -1 + (subcol+1)*(2./rootRPP), 0 ) ;
        
    real jitterRow = randFloat( rowBounds.x, rowBounds.y ) ;
    real jitterCol = randFloat( colBounds.x, colBounds.y ) ;

    window->addDebugLine( Vector( rowBounds.x, 1 ), 1, Vector( rowBounds.x, -1 ), 1 ) ; 
    window->addDebugLine( Vector( rowBounds.y, 1 ), 1, Vector( rowBounds.y, -1 ), 1 ) ; 
    window->addDebugLine( Vector( 1, colBounds.x ), 1, Vector( -1, colBounds.x ), 1 ) ; 
    window->addDebugLine( Vector( 1, colBounds.y ), 1, Vector( -1, colBounds.y ), 1 ) ; 

    Vector pt( jitterRow, jitterCol, 0 ) ;
    window->addDebugPoint( pt, Vector(1,1,0) ); 
  }

  // shoot the remaining rays in white
  for( int c = 0 ; c < rpp - nbins ; c++ )
  {
    int subrow = rand() % rootRPP ;
    int subcol = rand() % rootRPP ;

    Vector rowBounds( -1 + subrow*(2./rootRPP), -1 + (subrow+1)*(2./rootRPP), 0 ),
            colBounds( -1 + subcol*(2./rootRPP), -1 + (subcol+1)*(2./rootRPP), 0 ) ;
        
    real jitterRow = randFloat( rowBounds.x, rowBounds.y ) ;
    real jitterCol = randFloat( colBounds.x, colBounds.y ) ;

    Vector pt( jitterRow, jitterCol, 0 ) ;
    window->addDebugPoint( pt, 1 ); 
  }
  #endif

  #if 0
  int rpp = 4 ;
  for( int c = 0 ; c < rpp ; c++ )
  {
    real jitterRow = randFloat( -1, 1 ) ;
    real jitterCol = randFloat( -1, 1 ) ;

    Vector pt( jitterRow, jitterCol, 0 ) ;
    window->addDebugPoint( pt, 1 ); 
  }
  #endif
}