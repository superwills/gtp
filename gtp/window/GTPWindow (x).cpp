#include "GTPWindow.h"
#include "ISurface.h"
#include "Camera3D.h"
#include "D3D11Texture.h"

#include "../rendering/RaytracingCore.h"
#include "../rendering/RadiosityCore.h"
#include "../rendering/Hemicube.h"

#include "../geometry/AABB.h"
#include "../geometry/Sphere.h"
#include "../geometry/Model.h"
#include "../geometry/Cube.h"
#include "../geometry/Tetrahedron.h"
#include "../scene/Octree.h"
#include "../math/SHSample.h"
#include "../math/SHProjection.h"
#include "../threading/ParallelizableBatch.h"
#include "../rendering/FullCubeRenderer.h"
#include "../rendering/Raycaster.h"

char *RenderingModeName[] =
{
  "Nothing",    //0
  "Rasterize",  //1
  "Raytrace",   //2
  "Rast + RT",  //3
  "Debug Lines",//4
  "debug+Rast", //5
  "debug+RT",   //6
  "all",        //7
  "octree viz"
} ;

char *ShadingModeName[] =
{
  "Unlit color",
  "Phong lit",
  "Spherical",
  "Hemicube IDs",
  "Radiosity",
  "Fake Radiosity",
  "Wavelet"
} ;

char* CompressionTypeName[] =
{
  "No Compression",
  "Spherical Harmonics",
  "Haar Wavelet Cubemap",
  "Lattice Wavelet Cubemap",
  "Spherical Wavelet"
} ;

char *TextDisplayName[] =
{
  "No Text", // (never displays!)
  "Info only"
} ;

char *VertexColorizationModeName[] =
{
  "Nothing",
  "Debug color",
  "Averages",
  "Rad and avg",
  "Radiosity"
  
} ;



char* DEFAULT_DIRECTORY = "C:/CPP/Projects/2012/gtp/gtp" ;
char* DEFAULT_FILTER = "Spherical-harmonic model-data files (*.wsh)\0*.wsh\0All files (*.*)\0*.*\0\0" ;
char* DEFAULT_PROPERTIES_FILE = "options.json" ;

GTPWindow::GTPWindow( HINSTANCE hInst, TCHAR* windowTitleBar,
                    int windowXPos, int windowYPos,
                    int windowWidth, int windowHeight ) :
  D3D11Window( hInst, windowTitleBar,
             windowXPos, windowYPos,
             windowWidth, windowHeight )
{
  props = new Properties( DEFAULT_PROPERTIES_FILE ) ;

  // make the ray collection
  rayCollection = new RayCollection( props->getInt( "ray::collection size" ) ) ;

  // Now tie up the static member references that need to use this rayCollection
  // (for shorter access patterns as well as to remove dependence on knowing the
  // definition of GTPWindow to use the rayCollection)
  CubeMap::rc = Raycaster::rc = RaytracingCore::rc = SHSample::rc = rayCollection ;

  // hook this up to YSH, or YCheby.  Do this before initializing the shsamplecollection,
  // because the shsamplecollection will be based on this
  if( props->getInt( "sh::chebyshev" ) ) // if you are using chebyshev, then select it here
  {
    SH::Y = SH::YCheby ;
    SH::kernel = SH::Chebyshev;
  }

  // initializes the collection of shsamples, which resides
  // YOU CAN ONLY DO THIS AFTER SHSample::rc has been created!
  shSamps = new SHSampleCollection( props->getInt( "sh::bands" ), props->getInt( "sh::samples" ) ) ;

  // need this to be ready when creating lights/loading light maps,
  // so create it first.
  renderScenery = true ; 
  orthographicProjection = false ;
  shDidFinishRunning = false ;
  waveletDidFinishRunning = false ;
  rot = 0.0 ;
  scene = new Scene() ;

  camera = new Camera3D() ;

  rtCore = new RaytracingCore( windowHeight, windowWidth,
    props->getInt( "ray::rays per pixel" ),
    props->getInt( "ray::stratified" ),
    props->getInt( "ray::rays per light" ),
    props->getInt( "ray::trace to center of light" ),
    props->getInt( "ray::rays monte diffuse" ),
    props->getInt( "ray::rays cubemap lighting" ),
    props->getInt( "ray::num bounces" ),
    props->getDouble( "ray::termination energy" )
  ) ;
  
  radCore = new RadiosityCore( this, props->getInt( "radiosity::hemicube pixels per side" ), props->getInt( "radiosity::rays raycasted" ) ) ;
  raycaster = new Raycaster( props->getInt( "radiosity::rays raycasted" ) ) ;

  fcr = new FullCubeRenderer( scene,
    props->getInt( "cubemap::pixels per side" )
  ) ;
  FullCubeRenderer::compressionLightmap = props->getDouble( "cubemap::compression light map" ) ;
  FullCubeRenderer::compressionVisibilityMaps = props->getDouble( "cubemap::compression vis map" ) ;
  FullCubeRenderer::useRailed = props->getInt( "cubemap::use railed" ) ;

  // polygonal tree
  // overwrite the default options
  ONode<PhantomTriangle*>::maxDepth = props->getInt( "octree::max depth" ) ;
  ONode<PhantomTriangle*>::maxItems = props->getInt( "octree::max items" ) ;
  ONode<PhantomTriangle*>::splitting = props->getInt( "octree::split" ) ;
  

  initTextures() ; // for rendering scene to texture
  // for the radiosity/rt combined render
  
  initInputMan( hwnd, windowWidth, windowHeight ) ;

  subwindow = true ;

  renderingMode = RenderingMode::Rasterize | RenderingMode::DebugLines ;//| RenderingMode::Raytrace ;

  ss=TextureSamplingMode::Point;
  setTextureSamplingMode( (TextureSamplingMode)ss ) ;

  movementOn = false ;

  textDisplay = TextDisplay::InfoOnly ;

  // start cut off at about 98% of far plane
  textDisplayBackplane = HEMI_FAR*0.98 ;
  
  char* shaderFile = "shader/effect.vsh";
  
  
  shaderVcDebug = new VCShader( this, shaderFile, "vvc", shaderFile, "pxvc" ) ;
  shaderVncPlain = new VNCShader( this, shaderFile, "vvnc", shaderFile, "pxvncUnlit" ) ;
  shaderVtcSurfaces = new VTCShader( this, shaderFile, "vvtc", shaderFile, "pxvtc" ) ;
  
  // vGDiffuse_vnc copies over COLOR0 to the pixel shader
  shaderSHLit = new VT10NC10Shader( this, shaderFile, "vGSH_vnc", shaderFile, "pxvncUnlit" ) ;
  
  // vGSH_vnc copies over the COLOR1 to the pixel shader instead of COLOR0
  shaderPhongLit = new VT10NC10Shader( this, shaderFile, "vGFullPhong_vncccc", shaderFile, "pxvnccccFullPhong" ) ;
  
  shaderPlain = new VT10NC10Shader( this, shaderFile, "vGDiffuse_vnc", shaderFile, "pxvncUnlit" ) ;

  // This is ONLY used for plastering the cubemap texture onto
  // an object.
  shaderCubemap = new VT10NC10Shader( this, shaderFile, "vGDiffuse_vnc", shaderFile, "pxvncCubeMap" ) ;

  // for rendering shapes by their ids.
  shaderID = new VT10NC10Shader( this, shaderFile, "vGID_vnc", shaderFile, "pxvncUnlit" ) ;

  shaderRadiosity = new VT10NC10Shader( this, shaderFile, "vGRadiosity_vnc", shaderFile, "pxvncUnlit" ) ;
  
  shaderFakeRadiosity = new VT10NC10Shader( this, shaderFile, "vGFakeRadiosity_vnc", shaderFile, "pxvncUnlit" ) ;

  shaderWavelet = new VT10NC10Shader( this, shaderFile, "vGWavelet_vnc", shaderFile, "pxvncUnlit" ) ;
  
  shaderDecal = new VT10NC10Shader( this, shaderFile, "vGTexDecal_vtc", shaderFile, "pxvtc" ) ;

  shadingMode = ShadingMode::PhongLit ;

  vbDebugLines = vbDebugPoints = vbDebugTris = NULL ;

  mutexPbs = CreateMutexA( 0, 0, "mutex-pbs" ) ;

  // create font
  createFont( Arial12, 12, 100, "Arial" ) ;
  createFont( Arial14, 14, 100, "Arial" ) ;
  createFont( Arial18, 18, 700, "Arial" ) ;
}

GTPWindow::~GTPWindow()
{
  info( "GTPWindow destroy" ) ;
  
  DESTROY( shSamps ) ;
  DESTROY( texSubwindow ) ;
  DESTROY( texRaytrace ) ;

  //foreach( list<ProgressBar*>::iterator, iter, pbs )
  //  DESTROY( *iter ) ;
  pbs.clear() ;

  DESTROY( camera ) ;
  DESTROY( scene ) ;
  DESTROY( rtCore ) ;
  DESTROY( radCore ) ;

  DESTROY( vbDebugPoints ) ;
  DESTROY( vbDebugLines ) ;
  DESTROY( vbDebugTris ) ;
}

void GTPWindow::textDisplayBackplaneFurther()
{
  textDisplayBackplane -= 1.0 ;
  //clamp( textDisplayBackplane, HEMI_NEAR, HEMI_FAR ) ;
  info( "Text display backplane %f", textDisplayBackplane ) ;
}

void GTPWindow::textDisplayBackplaneNearer()
{
  textDisplayBackplane += 1.0 ;
  //clamp( textDisplayBackplane, HEMI_NEAR, HEMI_FAR ) ;
  info( "Text display backplane %f", textDisplayBackplane ) ;
}


void GTPWindow::step()
{
  timer.update() ;

  D3D11Window::step() ;  

  inputManStep() ;

  //t+=0.001;//!!increment angle
  
  //timer.lock( 60 ) ;
}

void GTPWindow::toggleMovement()
{
  movementOn = !movementOn ;
}

void GTPWindow::initTextures()
{
  info( "Creating surfaces, w=%d x h=%d", getWidth(), getHeight() ) ;
  
  // Allocate a render target for the test rt
  texSubwindow = new D3D11Surface( this, 0,0,
    getWidth(),getHeight(),
    0,1,
    0.01,
    SurfaceType::RenderTarget,
    0,DXGI_FORMAT_R8G8B8A8_UNORM ) ;

  // Create the texture which we'll
  // draw our ray-traced pixels to
  texRaytrace = new D3D11Surface( this, 0,0,
    getWidth(),getHeight(),
    0,1,
    0.02, // put this BEHIND the texSubwindow
    SurfaceType::ColorBuffer,
    0,DXGI_FORMAT_R8G8B8A8_UNORM ) ; // (behind d2d text texture)
}

void GTPWindow::drawAllText()
{
  if( textDisplay == TextDisplay::NoText ) // not none
    return ;

  beginText() ;
  ByteColor white( 255,255,255,240 ) ;
  
  int left = 12, top = 12, width=200, height=100, options = DT_LEFT ;
  
  drawText( Arial18, "gtp", white, left, top += 12, width, height, options ) ;
  
  static char buf[2048];
  int pos = sprintf( buf,
    "%s\n" // textdisplay type
    "%s\n" // rendering mode
    "%s%s\n" // shading mode
    "%d shapes\n" // # shapes
    "%d viz\n" // # viz
    "%d verts\n"
    //"light %s\n" // light name (removed)
    "%.1f fps\n" // current fps
    //"light pos (%.2f, %.2f, %.2f)\n" // light position (removed)
    //"texmode %s\n" // texture sampling mode
    "rotation %.2lf°\n"
    ,
    TextDisplayName[ textDisplay ],
    RenderingModeName[ renderingMode ],
    ShadingModeName[ shadingMode ],
    shadingMode == ShadingMode::SHLit ?
      (SH::kernel==SH::Legendre?"/Legendre":"/Chebyshev") : // the second string specifies legendre or chebyshev
      "", // the second string contains nothing
    scene->shapes.size(), scene->viz.size(),
    scene->countVertices(), // refresh the vertex count
    ////scene->getActiveLight() ? scene->getActiveLight()->name.c_str():"no lights",
    timer.fps,
    ////lightPos.x,lightPos.y,lightPos.z,
    //TextureSamplingModeName[ss]
    rot
  ) ;
  camera->eye.printInto( buf+pos ) ;

  drawText( Fonts::Arial12, buf, white, left, top += 20, width, height, options ) ;
  
  {
    MutexLock LOCK( window->mutexPbs, INFINITE ) ;
    foreach( list<ProgressBar*>::iterator, iter, pbs )
      directWrite->drawProgressBar( *(*iter) ) ;
  }

  endText() ;
  
  // this draws the D2D rendered TEXTURE.
  // this should be worked into the D3D11 class
  
  shaderVtcSurfaces->activate() ;
  shaderVtcSurfaces->setTexture( 0, sharedTexResView ) ;
  d2dQuad->drawTriangleStrip() ;
  shaderVtcSurfaces->unsetTexture( 0 ) ;
  
}

void GTPWindow::drawAllScenery()
{
  // these have no face side
  CullMode oldCullMode = cullmode ;
  
  cullMode( CullMode::CullNone ) ; /// TURN OFF CULLING

  // you must select the shader prior to calling
  for( int i = 0 ; i < scene->scenery.size() ; i++ )
    scene->scenery[i]->draw() ;  

  // Use a plain shader
  shaderPlain->activate() ;

  // the cubemap is considered scenery
  if( scene->cubeMap )
  {
    shaderCubemap->activate() ;
    
    // the cubeTex is always bound on slot 2
    scene->cubeMap->activate( 2 ) ;

    scene->cubeMap->draw() ;
  }
  
  cullMode( oldCullMode ) ; // put back the old setting
  //int i = 0 ;
  //while( i < scene->scenery.size() )
  //  scene->scenery[i++]->draw() ;

  // light sources are also scenery
  // NO THEY'RE NOT.. THEY'RE SIMPLY EMISSIVE SHAPES.
  ////shaderPlain->activate() ;
  ////for( int i = 0 ; i < scene->lights.size() ; i++ )
  ////  scene->lights[i]->lightShape->draw() ;
}

void GTPWindow::drawAllViz()
{
  MutexLock LOCK( scene->mutexVizList, INFINITE );
  //shaderPhongLit->activate() ;
  shaderPlain->activate(); 
    
  for( int i = 0 ; i < scene->viz.size() ; i++ )
    scene->viz[i]->draw() ;
}

void GTPWindow::drawDebug()
{
  // now use vcShader
  shaderVcDebug->activate();
  {
    MutexLock mutex( scene->mutexDebugLines, INFINITE ) ;
    drawDebugPoints() ;
    drawDebugLines() ;
    drawDebugTris() ;
  }
}

void GTPWindow::drawOctree()
{
  ///scene->octMesh->
}

void GTPWindow::rasterize()
{
  alphaBlend( true ) ;
  
  // SH shaders
  if( shadingMode == ShadingMode::SHLit )
    shaderSHLit->activate() ;
  else if( shadingMode == ShadingMode::PhongLit )
    shaderPhongLit->activate() ;
  else if( shadingMode == ShadingMode::HemicubeID )
  {
    alphaBlend( false ) ; // turn this off for id rendering.
    shaderID->activate() ;
  }
  else if( shadingMode == ShadingMode::Radiosity )
    shaderRadiosity->activate() ;
  else if( shadingMode == ShadingMode::FakeRadiosity )
    shaderFakeRadiosity->activate() ;
  else if( shadingMode == ShadingMode::WaveletShaded )
    shaderWavelet->activate() ;
  else
    shaderPlain->activate() ;
    
  render( scene ) ;

  // draw the scenery using the same shader
  if( renderScenery )
    drawAllScenery() ;
}

void GTPWindow::clearVisualizations()
{
  MutexLock LOCK( scene->mutexVizList, INFINITE ) ;
  for( int i = 0 ; i < window->scene->viz.size() ; i++ )
    DESTROY( scene->viz[i] ) ;
  scene->viz.clear() ;
}

void GTPWindow::testSHCubemapRot()
{
  if( !scene->cubeMap )
  {
    error( "No cubemap" ) ;
    return ;
  }
  ////////
  //
  //VIZ
  clearVisualizations(); 
  SHProjection* xRot = scene->cubeMap->shProjection->rotate( Matrix::RotationX( RADIANS(rot) ) ) ;
  SHProjection* yRot = scene->cubeMap->shProjection->rotate( Matrix::RotationY( RADIANS(rot) ) ) ;
  SHProjection* zRot = scene->cubeMap->shProjection->rotate( Matrix::RotationZ( RADIANS(rot) ) ) ;
  xRot->generateVisualization( 18, 18, Vector( 2,0,0 ) ) ;
  yRot->generateVisualization( 18, 18, Vector( 0,2,0 ) ) ;
  zRot->generateVisualization( 18, 18, Vector( 0,0,2 ) ) ;
  delete xRot ; delete yRot ; delete zRot ;
  ////////
}

void GTPWindow::testSHConv()
{
  static real ro = 0.0 ;
  ro += 0.01 ;
  
  // clear old viz
  clearVisualizations() ;

  // make rotate copy of cube map sh.
  SHProjection* cm = window->scene->cubeMap->shProjection->rotate( Matrix::RotationZ( ro ) ) ;
  cm->generateVisualization( 32, 32, Vector(0,0,0) ) ;

  SHProjection *zh = scene->shapes[0]->shProjection ;

  //zh->generateVisualization( 32, 32, Vector(0,4,0) ) ;

  SHProjection* conv = zh->conv( cm ) ;
  conv->generateVisualization( 32, 32, Vector( 4,0,0 ) ) ;
}

void GTPWindow::testSHRotation()
{
  SHProjection *rp = new SHProjection() ;
  
  // set function values
  //rp->set( 0, 0, Vector(  0, 0, 5 ) ) ;
  rp->set( 1, 0, Vector( -2, 0, 2 ) ) ;  // FLOWER
  //rp->set( 3, 2, Vector(  0, 5, 0 ) ) ; 
  rp->set( 5, -5, Vector( 0, 3, -4 ) ) ; // FLOWER
  //rp->set( 2, 2, Vector( 3, -2, 0 ) ) ;
  //rp->set( 2, 1, Vector( 2, 2, -8 ) ) ;

  rot += 1 ;
  
  real rads = RADIANS( rot ) ;

  //Matrix r = Matrix::Rotation( Vector(1,1,1).normalize(), rotY ) ; // * Matrix::RotationX( rotY ) ;
  Vector baseAxis(1,0,0) ;

  // rotate the axis 
  baseAxis.rotateY( rads ) ;
  //baseAxis = Matrix::RotationZ( rotY ) × baseAxis ;

  Matrix r = Matrix::Rotation( baseAxis, rads ) ; // * Matrix::RotationX( rotY ) ;
  addDebugLine( 0, Vector(.5,0,0), 8*baseAxis, Vector(1,0,0) ) ;

  // Delete the old one.
  clearVisualizations() ;
  
  SHProjection *old = rp ;
  rp = rp->rotate( r ) ;
  DESTROY(old) ;
  rp->generateVisualization( 64, 64, 0 ) ;
}

void GTPWindow::testSHSub()
{
  clearVisualizations() ;
  if( !scene->cubeMap || !scene->cubeMap->shProjection )
  {
    error( "No cube map sh proj" ) ;
    return ;
  }
  rot += 1;

  // Take the light function.
  SHProjection *lightFcn = scene->cubeMap->shProjection->rotate( Matrix::RotationZ( RADIANS( rot ) ) ) ;
  lightFcn->generateVisualization( 32, 32, 0 ) ;

  // Get the lobe which cuts the piece of the light function you want.
  SHProjection *lobe = scene->shapes[0]->shProjection ;

  // SUB OUT the lobe from the light function.
  SHProjection *notSeen = lightFcn->subbedFloorCopy( lobe ) ;

  // now do take the original light function,
  // and subtract away the 
  SHProjection *seen = lightFcn->subbedFloorCopy( notSeen ) ;
  seen->generateVisualization( 32, 32, Vector( 0, 10, 10 ) ) ;
}

void GTPWindow::testSHDir()
{
  clearVisualizations() ;
  // draw the rotated light function
  Vector look = camera->getLook().normalize() ;

  // we'd get the axis by reflecting the vector from the EYE to the
  // VERTEX IN QUESTION (ie look vector to a particular vertex)
  real t = acos( look.y ) ;//look • Vector(0,1,0)

  Vector axis = Vector(0,1,0) × look ;

  Matrix m = Matrix::Rotation( axis, t ) ;

  //SHRotationMatrix shm( shSamps->bands, m ) ;
  SHProjection *rotatedLightFcn = scene->cubeMap->shProjection->rotate( m ) ; 

  // viz that
  rotatedLightFcn->generateVisualization( 32, 32, Vector(0,5,0) ) ;

  // destroy it
  DESTROY( rotatedLightFcn ) ;
}

void GTPWindow::draw()
{
  if( !beginDrawing() )
    return ; // don't do any more calls if device lost
  
  if( deviceDirty )
  {
    scene->createVertexBuffers() ;
    
    // reallocate the surfaces
    DESTROY( texRaytrace ) ;
    DESTROY( texSubwindow ) ;

    initTextures() ;
    deviceDirty = false ;
  }

  clear( ByteColor(0,0,0,0) ) ;

  ////testSHRotation() ;
  ////testSHConv() ;
  ////testSHCubemapRot() ;
  ////testSHSub() ;
  testSHDir() ;
  #pragma region rotation matrices for the scene
  //rot += .5 ;
  if( rot >= 360 ) rot -= 360 ;
  else if( rot <= -360 ) rot += 360 ;
  
  real radians = RADIANS( rot ) ;

  // ROTATE
  ////Matrix rotMat = Matrix::RotationY( RADIANS(rot) ) ;//simple single axis rotation

  // moving axis
  Vector baseAxis(1,0,0) ;

  // rotate the axis 
  baseAxis.rotateY( radians ) ;
  
  // Cubemap for raytracing AND sh rotations use this same matrix
  rotMat = Matrix::Rotation( baseAxis, radians ) ;
  #pragma endregion

  // Update the gpu shader variables
  // use one active light, for now
  setWorld( Identity ) ;
  camera->eye.writeFloat4( gpuCData0.eyePos ) ;

  Vector(0,0,0).writeFloat4( gpuCData0.ambientLightColor[0] ) ;

  //if( !scene->lights.size() )
  {
    //info( "There are no lights specified, specifying one" ) ;
    Vector pos = Vector( 125,125,125 ) ;
    Vector color = 1 ;

    pos.writeFloat4( gpuCData0.diffuseLightPos[0] ) ;
    color.writeFloat4( gpuCData0.diffuseLightColor[0] ) ;
    
    // SPECULAR
    // multiply color by (1-DIFFUSION)
    pos.writeFloat4( gpuCData0.specularLightPos[0] ) ;
    color.writeFloat4( gpuCData0.specularLightColor[0] );
  
    // assume it shines in the opposite of the position
    // the direction vector must always be normalized
    (-pos).normalize().writeFloat4( gpuCData0.specularLightDir[0] ) ;
  }
  #if 0
  else
  {
    int i ;
    //info( "There are %d lights", scene->lights.size() ) ;
    for( i = 0 ; i < scene->lights.size() - 1; i++ )
    {
      // ok, move the light each frame
      //////t += 0.0005 ;
      //////Matrix rot = Matrix::RotationX( t ) ;
      //////Vector pos = rot × scene->lights[i]->pos ;
      //////scene->lights[i]->lightShape->transform( Matrix::Translate( pos ) ) ;
      Vector pos = scene->lights[i]->getCentroid() + Vector( 0, 20, 0 ) ; // I moved the pos.
      Vector lightColor = scene->lights[i]->material.ke ;
    
      // DIFFUSE
      // multiply color by DIFFUSION
      // Assume a point light source with color as specified by KE
    
      pos.writeFloat4( gpuCData0.diffuseLightPos[i] ) ;
      lightColor.writeFloat4( gpuCData0.diffuseLightColor[i] ) ;
    
      // SPECULAR
      // multiply color by (1-DIFFUSION)
      pos.writeFloat4( gpuCData0.specularLightPos[i] ) ;
      lightColor.writeFloat4( gpuCData0.specularLightColor[i] );

      // assume it shines in the opposite of the position
      // the direction vector must always be normalized
      (-pos).normalize().writeFloat4( gpuCData0.specularLightDir[i] ) ;
    }
  }
  #endif






  // use the same light source for all of em
  gpuCData0.activeLights[ 0 ] = 0 ; // ambient
  gpuCData0.activeLights[ 1 ] = 1 ; // diffuse
  gpuCData0.activeLights[ 2 ] = 1 ; // specular

  //if( scene->lights.size() > 0 ) diffuseLightPos = scene->lights[0]->pos ; // using the light0 object.
  //if( fakeRadiosityLights.size() > 0 )  diffuseLightPos = fakeRadiosityLights[0]->getCentroid() ;
  //if( scene->lights.size() > 1 )  specularLightPos = scene->lights[1]->pos ;
  updateGPUCData( 0 ) ; // this is the lights.

  /////Matrix rotMat = Matrix::RotationY( RADIANS(rot) ) ;
  rotMat.writeFloat16ColumnMajor( gpuCData3.texRot ) ;
  updateGPUCData( 3 ) ; // this is the texture rotation matrix

  /// this rotation will apply to the cubemap viz as well
  if( scene->cubeMap ) 
    scene->cubeMap->viz->meshGroup->meshes[0]->world = rotMat ;

  if( waveletDidFinishRunning && shadingMode==ShadingMode::WaveletShaded )
  {
    scene->cubeMap->rotate( rotMat ) ;
    scene->cubeMap->compress() ; // compress it right away, to wavelet domain.

    //window->scene->cubeMap->rotateWavelet( matrixRotation ) ; // this doesn't work, you can't rotate a wavelet the same way
    // you rotate a space function

    scene->waveletLight() ;
    window->colorNormalize( ColorIndex::WaveletComputed ) ;
    window->updateVertexBuffers() ;
  }
  if( shDidFinishRunning && shadingMode==ShadingMode::SHLit )
  {
    clearVisualizations() ;
    scene->shLight( rotMat ) ;
    colorNormalize( ColorIndex::SHComputed ) ;
    updateVertexBuffers() ;
  }

  /////
  // NEAR PLANE
  Matrix mproj,mlook ;
  if( orthographicProjection )
  {
    Vector eye = camera->eye ;
    real dist = eye.len() ;
    mproj = Matrix::ProjOrtho( -dist,dist, -dist,dist, -1e3,1e3 ) ;
    mlook = Matrix::LookAt( eye, camera->getLook(), camera->up ) ;
  }
  else
  {
    mproj = Matrix::ProjPerspectiveRH( RADIANS( 45 ), (real)window->getWidth()/window->getHeight(), .1, 1000 ) ;
    mlook = Matrix::LookAt( camera->eye, camera->getLook(), camera->up ) ;
  }
  
  setModelviewProjection( mlook, mproj ) ; // flushes to gpu, which also flushes eye data

  //cullMode( CullMode::CullNone ) ;
  if( containsFlag(renderingMode, Rasterize) )
  {
    rasterize() ;
  }
  if( containsFlag( renderingMode, RenderingMode::DebugLines ) )
  {
    shaderPlain->activate() ;
    drawAxes() ;
    drawDebug() ;
    drawAllViz() ;
  }
  if( containsFlag( renderingMode, RenderingMode::OctreeViz ) )
  {
    drawOctree() ;
  }

  /// RENDER SUBWINDOW
  if( subwindow )
  {
    // unset all 
    texSubwindow->rtBind() ;
    texSubwindow->clear( ByteColor(0,0,0,180) ) ; // the subwindow can be clear in the background

    // translate to origin, but use the eye a fixed distance
    // from the object

    // so use the camera FORWARD direction,
    // then translate you BACKWARDS from the face direction (-10),
    mlook = Matrix::Translate( 0,0,-5 ) * Matrix::FaceRowMajor( camera->forward, camera->right, camera->up );
    setModelviewProjection( mlook, mproj ) ;

    drawAxes() ;
    //drawDebug() ;
    //drawAllViz() ;

    // draw the current light being used to light the scene
    if( scene->cubeMap )
    {
      // use the plain shader.
      shaderPlain->activate() ;

      //mlook = Matrix::Translate( 0,0,-5 ) * Matrix::FaceRowMajor( camera->forward, camera->right, camera->up ) * Matrix::RotationY( RADIANS(rot) );
      //setModelviewProjection( mlook, mproj ) ;
      scene->cubeMap->viz->draw() ;
    }
      
    // well here, I need to UNTRANSLATE the light, because it might not
    // be at the origin, but in this viz I need it to appear at the origin
    setModelviewProjection( mlook * rotMat, mproj ) ;
     
    // draw the light
    texSubwindow->rtUnbind() ;
  }


  /////////////////
  // render textures
  FillMode oldMode = fillmode ;
  // always draw solid, even if scene rasterized in wireframe
  setFillMode( FillMode::Solid ) ;
  
  if( containsFlag( renderingMode, RenderingMode::Raytrace ) )
  {
    // pull the data from rtCore
    rtCore->writePixelsToByteArray() ;
    texRaytrace->copyFromByteArray( (BYTE*)&rtCore->bytePixels[0], 4, rtCore->getNumPixels() ) ;
    // I don't lock the raytracer's pixels array,
    // so this doesn't have a huge performance impact
    // (ie it _doesn't_ halt the raytracer)

    // we are drawing in the canonical view volume here straight away.
    //Matrix xform = Matrix::Translate( 0.73, -0.73, 0 ) * Matrix::Scale( 0.25, 0.25, 1 ) ;
    Matrix xform=identity;
    setModelviewProjection( xform, identity ) ;

    shaderVtcSurfaces->activate() ;
    texRaytrace->draw() ;
  }

  if( subwindow )
  {
    Matrix xform = Matrix::Translate( 0.73, 0.73, 0 ) * Matrix::Scale( 0.25, 0.25, 1 ) ;
    setModelviewProjection( xform, identity ) ;

    shaderVtcSurfaces->activate() ;
    texSubwindow->draw() ;
  }
  
  if( textDisplay ) {
    setModelviewProjection( identity, identity ) ;
    alphaBlend( true ) ; // THIS MUST BE ON OTHERWISE THE TEXT PLANE BLOCKS THE STUFF YOU SHOULD SEE IN THE SCENE
    drawAllText() ; // draw alll the text, includes begin/endText calls.
  }
  setFillMode( oldMode ) ;
  
  endDrawing() ;
  present() ;  
}

void GTPWindow::drawAxes()
{
  static bool init = false ;
  if( !init )
  {
    init = true ;
    static real LEN = 500 ;
    static VertexC axis[] = {

      // x-axis is red
      //VertexC( Vector(-LEN, 0, 0), Vector(1, 0, 0) ),
      VertexC( Vector( 0, 0, 0), Vector(1, 0, 0) ),
      VertexC( Vector(+LEN, 0, 0), Vector(1, 0, 0) ),

      // y-axis green
      VertexC( Vector(0, 0, 0), Vector(0, 1, 0) ),
      VertexC( Vector(0, +LEN, 0), Vector(0, 1, 0) ),

      // z-axis blue
      VertexC( Vector(0, 0, 0), Vector(0, 0, 1) ),
      VertexC( Vector(0, 0, +LEN), Vector(0, 0, 1) )

    } ;
    axisLines = new TVertexBuffer<VertexC>( this, axis, 6 ) ;
  
    // Draw points at end of axis.
    static VertexC points[] = {
      VertexC( Vector(LEN, 0, 0), Vector(255, 0, 0) ),
      VertexC( Vector(0, LEN, 0), Vector(0, 255, 0) ),
      VertexC( Vector(0, 0, LEN), Vector(0, 0, 255) )
    } ;
    axisPoints = new TVertexBuffer<VertexC>( this, points, 3 ) ;
  }

  shaderVcDebug->activate() ;

  axisLines->drawLines() ;
  axisPoints->drawPoints() ;
}

void GTPWindow::addDebugPoint( const Vector& pt, const Vector& color )
{
  debugPoints.push_back( VertexC( pt, color ) );
  debugPointsChanged = true ;
}

void GTPWindow::addDebugLine( const Vector& start, const Vector& startColor, const Vector& end, const Vector& endColor )
{
  debugLines.push_back( VertexC( start, startColor ) );
  debugLines.push_back( VertexC( end, endColor ) );
  debugLinesChanged = true ;
}

void GTPWindow::addDebugLineLock( const Vector& start, const Vector& startColor, const Vector& end, const Vector& endColor )
{
  MutexLock lock( scene->mutexDebugLines, INFINITE ) ;
  addDebugLine( start, startColor, end, endColor ) ;
}

void GTPWindow::addDebugRayLock( const Ray& ray, const Vector& startColor, const Vector& endColor, bool tip )
{
  MutexLock lock( scene->mutexDebugLines, INFINITE ) ;
  addDebugLineLock( ray.startPos, startColor, ray.getEndPoint(), endColor ) ;

  // put a tip on, tip is 0.25 units
  if( tip )
  {
    
  }
}

void GTPWindow::addDebugTriLock( const Triangle& tri, const Vector& color )
{
  MutexLock lock( scene->mutexDebugLines, INFINITE ) ;
  
  addDebugLine( tri.a, Vector(1,0,0), tri.b, Vector(0,0,1) ) ;
  addDebugLine( tri.b, Vector(1,0,0), tri.c, Vector(0,0,1) ) ;
  addDebugLine( tri.c, Vector(1,0,0), tri.a, Vector(0,0,1) ) ;
}

void GTPWindow::addDebugTriLock( PhantomTriangle* pTri, const Vector& color )
{
  MutexLock lock( scene->mutexDebugLines, INFINITE ) ;

  addDebugLine( pTri->a, Vector(1,0,0), pTri->b, Vector(0,0,1) ) ;
  addDebugLine( pTri->b, Vector(1,0,0), pTri->c, Vector(0,0,1) ) ;
  addDebugLine( pTri->c, Vector(1,0,0), pTri->a, Vector(0,0,1) ) ;
}

void GTPWindow::addSolidDebugTri( Vector offset, Triangle& tri, const Vector& color )
{
  debugTris.push_back( VertexC( offset+tri.a, tri.vA()->color[ColorIndex::DiffuseMaterial] ) );
  debugTris.push_back( VertexC( offset+tri.b, tri.vB()->color[ColorIndex::DiffuseMaterial] ) );
  debugTris.push_back( VertexC( offset+tri.c, tri.vC()->color[ColorIndex::DiffuseMaterial] ) );
  debugTrisChanged = true ;
}

void GTPWindow::addSolidDebugTri( Triangle& tri, const Vector& color )
{
  addSolidDebugTri(0,tri,color) ;
}

void GTPWindow::addSolidDebugTri( Vector offset, PhantomTriangle* pTri, const Vector& color )
{
  Vector c = pTri->tri->getAverageColor( ColorIndex::DiffuseMaterial ) * color ; // color it with "color" as a stain
  debugTris.push_back( VertexC( offset+pTri->a, c ) );
  debugTris.push_back( VertexC( offset+pTri->b, c ) );
  debugTris.push_back( VertexC( offset+pTri->c, c ) );
  debugTrisChanged = true ;
}

void GTPWindow::addSolidDebugTri( PhantomTriangle* pTri, const Vector& color )
{
  addSolidDebugTri(0,pTri,color);
}

void GTPWindow::drawDebugPoints()
{
  if( debugPointsChanged )
  {
    DESTROY( vbDebugPoints ) ; // destroy the old one

    if( debugPoints.size() > 0 )
      vbDebugPoints = new TVertexBuffer<VertexC>( this, &debugPoints[0], debugPoints.size() ) ;

    debugPointsChanged = false ;
  }

  if( vbDebugPoints )  vbDebugPoints->drawPoints() ;
}

void GTPWindow::drawDebugLines()
{
  if( debugLinesChanged )
  {
    DESTROY( vbDebugLines ) ; // destroy the old one
    
    if( debugLines.size() > 1 )
      vbDebugLines = new TVertexBuffer<VertexC>( this, &debugLines[0], debugLines.size() ) ;

    debugLinesChanged = false ;
  }

  if( vbDebugLines )  vbDebugLines->drawLines() ;
}

void GTPWindow::drawDebugTris()
{
  if( debugTrisChanged )
  {
    DESTROY( vbDebugTris ) ; // destroy the old one
    
    if( debugTris.size() > 1 )
      vbDebugTris = new TVertexBuffer<VertexC>( this, &debugTris[0], debugTris.size() ) ;

    debugTrisChanged = false ;
  }

  if( vbDebugTris )  vbDebugTris->drawTris() ;
}

// CALL ON MAIN THREAD
void GTPWindow::shSave()
{
  char buf[ MAX_PATH ] ;
  sprintf( buf, "OUT.wsh" ); // write a default file
        
  // You cannot call savefile() on NOT the main thread.
  if( SAVING && window->savefile( "Save SH computations/models to a file", DEFAULT_DIRECTORY, DEFAULT_FILTER, buf ) )
  {
    // set up a flag variable that gets checked on the main thread (easiest way to do)
    // alternatively allow job submission TO BE EXEC BY MAIN THREAD AND NOT A WORKER THREAD.
    window->save( buf ) ;
  }
  else
    warning( "File not saved" ) ;
}

void GTPWindow::triggerVertexBufferUpdateOnMainThread()
{
  threadPool.createJobForMainThread( "update vbs",
    new Callback0( [](){
      window->updateVertexBuffers() ;  // ONLY DO THIS ON THE MAIN THREAD, BECAUSE IT TOUCHES THE GRAPHICS DEVICE
    } ) ) ;
}

void GTPWindow::shCompute()
{
  shDidFinishRunning = false ; // true until last line of last callback completes

  ParallelBatch *batchSH1 = new ParallelBatch( "batch::sh1", NULL ) ;
  ParallelBatch *batchSH2 = new ParallelBatch( "batch::sh2", new Callback0( [](){
    // JOB TO DO WHEN BATCH2 IS __DONE__
    info( "sh comps done" ) ;

    // add the interreflections together
    window->scene->shAddInterreflections() ;
    
    // call SHLIGHT
    window->scene->shLight( Matrix() ) ; // identity

    // then color normalize
    window->colorNormalize( ColorIndex::SHComputed ) ;

    // safe to submit the main thread save file callback now
    window->triggerVertexBufferUpdateOnMainThread() ;

    window->shDidFinishRunning = true ;
  } ) ) ;
    
  // Prepare the batches
  raycaster->QSHPrecomputation( shSamps->n, batchSH1, batchSH2, scene, 120 ) ;

  // Submit batch1 before batch2 (order matters)
  threadPool.addBatch( batchSH1 ) ;
  threadPool.addBatch( batchSH2 ) ;
}

void GTPWindow::colorNormalize( ColorIndex colorIndex )
{
  // normalize the colors, for a certain color channel
  real largest = 0 ; // static TEMPORARY HACK - this won't really work when you do sh/wavelet alternatively
  Vector maxColor ;

  if( !largest )
  {
    // find the maximum (brightest) color, all channels.
    for( int s = 0 ; s < scene->shapes.size() ; s++ )
    {
      Shape *shape = scene->shapes[s];
    
      for( int mn = 0 ; mn < shape->meshGroup->meshes.size() ; mn ++ )
      {
        Mesh* mesh = shape->meshGroup->meshes[ mn ] ;
        for( int i = 0 ; i < mesh->verts.size() ; i++ )
          maxColor.clamp( mesh->verts[i].color[ colorIndex ], Vector( 1e6,1e6,1e6 ) ) ; // bound below by the color,
          // so by the end you have the max in each channel here
      }
    }
    
    printf( "MAX COLOR %f, %f, %f     \r", maxColor.x, maxColor.y, maxColor.z ) ;
    largest = max( max( maxColor.x, maxColor.y ), maxColor.z ) ; // what's the largest channel?
  }
  

  // divide by the color.  this KIND OF is nonlinear
  // so really you should divide by a SCALAR (MAX component
  // in the MAXCOLOR vector)
  for( int s = 0 ; s < scene->shapes.size() ; s++ )
  {
    Shape *shape = scene->shapes[s];

    // normalize the colors
    for( int mn = 0 ; mn < shape->meshGroup->meshes.size() ; mn ++ )
    {
      Mesh* mesh = shape->meshGroup->meshes[ mn ] ;
      for( int i = 0 ; i < mesh->verts.size() ; i++ )
        //mesh->verts[i].color[ colorIndex ] /= maxColor ;
        mesh->verts[i].color[ colorIndex ] /= largest ; //preserve color balance
    }
  }
}

void GTPWindow::updateVertexBuffers()
{
  ////info( "Updating vertex buffers.." ) ;
  // You cannot draw while creating/dismantling the vertex buffer
  // (or updating them!)
  {
    MutexLock LOCK( window->scene->mutexShapeList, INFINITE ) ;
    for( int i = 0 ; i < scene->shapes.size() ; i++ )
    {
      Shape *shape = scene->shapes[i];
      for( int mn = 0 ; mn < shape->meshGroup->meshes.size() ; mn++ )
      {
        Mesh *mesh = shape->meshGroup->meshes[mn];
        //mesh->createVertexBuffer() ;// don't re-create it, update it!
        //////for( int j = 0 ; j < mesh->verts.size() ; j++ )
        //////  mesh->verts[ j ].color[ ColorIndex::SHComputed ] = Vector::random() ;
        mesh->updateVertexBuffer() ;
      }
    }
  }//unlock
}

void GTPWindow::save( char* outname )
{
  //////
  // HEADERSHFILE v0.2.0
  //   - 'WSSH'
  //   - ..other HEADERSHFILE members..
  //   - global data such as eyepos
  //   - HEADERCUBEMAP (only 1 is allowed at this time)
  //   // ALL SHAPES FOLLOW: each shape as:
  //   - HEADERSHAPE
  //     - headershape.ShapeType = CubeShape/SphereShape.. ( WRITTEN IN OVERLOAD OF SHAPE::SAVE(FILE*) )
  //     - Shape::Save(FILE*) is called, which does this:
  //     - save material
  //     - HEADERMESHGROUP
  //       - numMeshes etc
  //       - each mesh is then saved with
  //       - HEADERMESH
  //         - meshType
  //         - vertexType
  //         - numTris .. etc
  //         - TRIS
  //         - VERTS (in ALLVERTEX format including vert/norm/color/texcoord/SHDATA)
  //         - FACES
  //     - SHAPE SPECIFIC global data eg for cube, the 8 corners are saved. this USED to be
  //       FIRST in the shape, but was moved to last since logically, the base class object should
  //       be saved first instead.
  //   - HEADERSHAPE of next shape..
  //         

  HeaderSHFile header ;

  header.magic[0] = 'W', header.magic[1] = 'S', header.magic[2] = 'S', header.magic[3] = 'H' ;// WSSH
  header.major = 0 ;
  header.minor = 3 ;
  header.numShapes = scene->shapes.size() ;
  /////header.numLights = scene->lights.size() ;
  header.shBands   = shSamps->bands ;
  header.shSamples = shSamps->n ;
  header.dataStart = sizeof( HeaderSHFile ) ;

  FILE * f = fopen( outname, "wb" ) ; // binary write
  if( !f )  { error( "Could not open %s for writing", outname ) ; } 
  
  // write the header
  int written = 0 ;
  written += sizeof( HeaderSHFile )*fwrite( &header, sizeof( HeaderSHFile ), 1, f ) ;

  // write the eye pos
  written += sizeof( Vector ) * fwrite( &camera->eye, sizeof(Vector), 4, f ) ;

  // save all de lights.
  //////for( int i = 0 ; i < scene->lights.size() ; i++ )
  //////  written += scene->lights[i]->save( f ) ;

  // write every Shape*
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
    written += scene->shapes[i]->save( f ) ; // save yourself to file handle f

  info( "Finished writing %s, %d bytes", outname, written ) ;
  fclose( f ) ;

}

void GTPWindow::load( char* infile )
{
  FILE *f = fopen( infile, "rb" ) ;
  if( !f ) error( "Could not open %s for reading", infile ) ;

  // read the shHeader
  HeaderSHFile shHeader; 
  fread( &shHeader, sizeof(HeaderSHFile), 1, f ) ;

  fread( &camera->eye, sizeof(Vector), 4, f ) ;

  DESTROY( shSamps ) ; // delete existing one
  shSamps = new SHSampleCollection( shHeader.shBands, shHeader.shSamples ) ;

  // load all lights
  //////for( int i = 0 ; i < shHeader.numLights ; i++ )
  //////{
  //////  HeaderLight head ;
  //////  fread( &head, sizeof(HeaderLight), 1, f ) ;
  //////  MathematicalShape *light = MathematicalShape::load( head, f ) ;
  //////  scene->addShape( light ) ;
  //////}

  for( int i = 0 ; i < shHeader.numShapes ; i++ )
  {
    // read the next header
    HeaderShape head ;
    fread( &head, sizeof(HeaderShape), 1, f ) ;

    // the header tells you what shape type to instantiate and
    // how many floats to read for the sh coeffs
    Shape *shape = Shape::load( head, f ) ;  
    scene->addShape( shape ) ;
  }

  fclose( f ) ;

  info( "Loaded %d shapes from %s", shHeader.numShapes, infile ) ;
  
  // octree
  scene->computeOctree() ;


}

