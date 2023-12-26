#include "GTPWindow.h"
#include "ISurface.h"
#include "Camera.h"
#include "D3D11Texture.h"

#include "../rendering/RaytracingCore.h"
#include "../rendering/RadiosityCore.h"
#include "../rendering/Hemicube.h"

#include "../geometry/AABB.h"
#include "../geometry/Sphere.h"
#include "../geometry/Model.h"
#include "../geometry/Cube.h"
#include "../geometry/Tetrahedron.h"
#include "../geometry/MathematicalShape.h"
#include "../scene/Octree.h"
#include "../math/SHSample.h"
#include "../math/SHVector.h"
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
  "Phong lit px",
  "Phong lit vertex",
  "Spherical cpu",
  "Spherical gpu",
  "Hemicube IDs",
  "Radiosity",
  "Occlusion",
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
  MAX_LIGHTS = 32 ; // shader variable.. hard coded maximum for # lights
  // the shader supports (cbuffer size)

  props = new Properties( DEFAULT_PROPERTIES_FILE ) ;
  programState = Busy ;  // busy while booting up

  // make the ray collection
  rayCollection = new RayCollection( props->getInt( "ray::collection size" ) ) ;

  // Now tie up the static member references that need to use this rayCollection
  // (for shorter access patterns as well as to remove dependence on knowing the
  // definition of GTPWindow to use the rayCollection)
  FullCubeRenderer::rc = CubeMap::rc = Raycaster::rc = RaytracingCore::rc = SHSample::rc = rayCollection ;
  SH::kernelName = "SH" ;

  // hook this up to YSH, or YCheby.  Do this before initializing the shsamplecollection,
  // because the shsamplecollection will be based on this
  if( props->getInt( "sh::chebyshev" ) ) // if you are using chebyshev, then select it here
  {
    SH::Y = SH::YCheby ;
    SH::kernel = SH::Chebyshev;
    SH::kernelName = "CH" ;
  }
  doInterreflect = props->getInt( "sh::interreflect" ) ;

  // initializes the collection of shsamples, which resides
  // YOU CAN ONLY DO THIS AFTER SHSample::rc has been created!
  shSamps = new SHSampleCollection( props->getInt( "sh::bands" ), props->getInt( "sh::samples" ), props->getInt( "sh::samples to cast" ) ) ;

  // need this to be ready when creating lights/loading light maps,
  // so create it first.
  renderScenery = true ; 
  renderSHLight = false ; // start with original
  shDidFinishRunning = false ;
  waveletDidFinishRunning = false ;
  rot = 0.0 ;
  subwindowBackDist = -5 ;
  scene = new Scene() ;
  scene->useFarLights = props->getInt( "scene::use far lights" ) ;
  scene->perlinSkyOn = props->getInt( "ray::perlin sky on" ) ; // is the sky on or not?

  viewCam = new Camera() ;
  lightCam = new Camera() ;
  camera = viewCam ; // default
  lightForCam = NULL ;

  rtCore = new RaytracingCore( windowHeight, windowWidth,
    props->getInt( "ray::rays per pixel" ),
    props->getInt( "ray::stratified" ),
    props->getString( "ray::trace type" ),
    props->getInt( "ray::rays distributed" ),
    props->getInt( "ray::rays cubemap lighting" ),
    props->getInt( "ray::num bounces" ),
    props->getDouble( "ray::termination energy" ),
    props->getInt( "ray::show bg" )
  ) ;

  radCore = new RadiosityCore( this, props->getInt( "radiosity::hemicube pixels per side" ) ) ;
  raycaster = new Raycaster( props->getInt( "ray::num caster rays" ) ) ;

  fcr = new FullCubeRenderer( scene, props->getInt( "wavelet::pixels per side" ) ) ;
  FullCubeRenderer::compressionLightmap = props->getDouble( "wavelet::compression light map" ) ;
  FullCubeRenderer::compressionVisibilityMaps = props->getDouble( "wavelet::compression vis map" ) ;
  FullCubeRenderer::useRailedQuincunx = props->getInt( "wavelet::use railed quincunx" ) ;

  // polygonal tree
  // overwrite the default options
  ONode<PhantomTriangle*>::maxDepth = props->getInt( "space partitioning::max depth" ) ;
  ONode<PhantomTriangle*>::maxItems = props->getInt( "space partitioning::max items" ) ;
  ONode<PhantomTriangle*>::splitting = props->getInt("space partitioning::split" ) ;

  string partType = props->getString( "space partitioning::split type" ) ; //k or o or ok
  if( partType=="k" )
    spacePartitionType = SpacePartitionType::PartitionKDTree ;
  else
    spacePartitionType = SpacePartitionType::PartitionOctree ;
  if( partType.size() > 1 )
  {
    // 2nd must be 'k', ie octree with kdstyle subdivisions
    Octree<Shape*>::useKDDivisions = Octree<PhantomTriangle*>::useKDDivisions = true ;
  }
  initTextures() ; // for rendering scene to texture
  // for the radiosity/rt combined render
  
  initInputMan( hwnd, windowWidth, windowHeight ) ;

  subwindow = true ;

  renderingMode = RenderingMode::Rasterize | RenderingMode::DebugLines ;//| RenderingMode::Raytrace ;

  ss=TextureSamplingMode::Point;
  setTextureSamplingMode( (TextureSamplingMode)ss ) ;

  scene->spacePartitioningOn = props->getInt( "space partitioning::on" ) ;
  movementOn = false ;

  textDisplay = TextDisplay::InfoOnly ;

  // start cut off at about 98% of far plane
  textDisplayBackplane = HEMI_FAR*0.98 ;
  
  char* shaderFile = "shader/effect.hlsl";
  expos = 1.0 ;

  // Init some of these values.
  gpuCData0.vo[0] = 1 ; // Whether to use AO or VO
  gpuCData0.vo[1] = 0.2 ; // The "vo shadow exposure" value
  gpuCData0.vo[2] = 0 ; // TIME
  gpuCData0.vo[3] = 3.8 ; // caustic exposure
  
  // Init some of these values.
  gpuCData0.voI[0] = 1 ; // diffuse from diffuse interreflection strength
  gpuCData0.voI[1] = 1 ; // diffuse from specular interreflection strength
  gpuCData0.voI[2] = 1 ; // specular from specular interreflection strength
  gpuCData0.voI[3] = 4 ; // specular interreflection exponent

  shaderVcDebug = new VCShader( this, shaderFile, "vvc", shaderFile, "pxvc" ) ;
  
  //This doesn't actually get used, becausee i stopped using vnc format
  shaderVncPlain = new VNCShader( this, shaderFile, "vvnc", shaderFile, "pxvncUnlit" ) ;

  shaderVtcSurfaces = new VTCShader( this, shaderFile, "vvtc", shaderFile, "pxvtc" ) ;
  
  shaderSHLitCPU = new VT10NC10Shader( this, shaderFile, "vGSHCPU_vnc", shaderFile, "pxvncUnlit" ) ;
  shaderSHLitGPU = new VT10NC10Shader( this, shaderFile, "vGSHGPU_vnc", shaderFile, "pxvncUnlit" ) ;
  shaderPhongLit = new VT10NC10Shader( this, shaderFile, "vGFullPhong_vncccc", shaderFile, "pxvnccccFullPhong" ) ;
  shaderPhongLitVertex = new VT10NC10Shader( this, shaderFile, "vGFullPhong_vnc", shaderFile, "pxvncUnlit" ) ;
  
  shaderPlain = new VT10NC10Shader( this, shaderFile, "vGDiffuse_vnc", shaderFile, "pxvncUnlit" ) ;

  // This is ONLY used for plastering the cubemap texture onto
  // an object.
  shaderCubemap = new VT10NC10Shader( this, shaderFile, "vGDiffuse_vnc", shaderFile, "pxvncCubeMap" ) ;

  // for rendering shapes by their ids.
  shaderID = new VT10NC10Shader( this, shaderFile, "vGID_vnc", shaderFile, "pxvncUnlit" ) ;

  shaderRadiosity = new VT10NC10Shader( this, shaderFile, "vGRadiosity_vnc", shaderFile, "pxvncUnlit" ) ;
  
  // Per vertex VO
  shaderVectorOccluders = new VT10NC10Shader( this, shaderFile, "vGVectorOccluders_vnc", shaderFile, "pxvncVO" ) ;
  
  // Per pixel VO
  //shaderVectorOccluders = new VT10NC10Shader( this, shaderFile, "vGVectorOccluders_pxVO", shaderFile, "pxVO" ) ;

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

void GTPWindow::screenshotMulti( int numShots, real timeInterval, real angleIncrementSize )
{
  // just returns until it is time to shoot.
  for( int i = 1 ; i <= numShots; i++ )
  {
    ///printf( "Adding a job\n" ) ;
    threadPool.createJobForMainThread( "multishot", new Callback0( [i,angleIncrementSize](){
      // take a shot
      //puts( "SCREENIE" ) ;
      char b[255];
      sprintf( b,"%s %d (%.0f deg)",
        RenderingModeName[ window->renderingMode ], //name
        i, /// picture #
        window->rot ) ; // rotation
      window->screenshot( b ) ;
      window->rot += angleIncrementSize ;
    } ), 2 + i*timeInterval ) ; // 2 is the dummy offset.. before even starting
  }
}

void GTPWindow::step()
{
  clock.update() ;

  D3D11Window::step() ;  

  inputManStep() ;

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
    0,DXGI_FORMAT_R8G8B8A8_UNORM,4 ) ;

  // Create the texture which we'll
  // draw our ray-traced pixels to
  texRaytrace = new D3D11Surface( this, 0,0,
    getWidth(),getHeight(),
    0,1,
    0.02, // put this BEHIND the texSubwindow
    SurfaceType::ColorBuffer,
    0,DXGI_FORMAT_R8G8B8A8_UNORM,4 ) ; // (behind d2d text texture)
}

void GTPWindow::bakeRotationsIntoLights()
{
  // transformation means you have to break down and re-build the space partition
  if( rot && scene->lights.size() )
  {
    for( int i = 0 ; i < scene->lights.size() ; i++ )
      scene->lights[i]->transform( rotationMatrix ) ;
  
    // rot = 0 ; // reset this so the lights don't rotate AGAIN..
  
    info( "Rebuilding space partition due to rotations being baked into light sources.." ) ;
    if( scene->spacePartitioningOn )
      scene->computeSpacePartition() ;
    //rotationMatrix = Matrix() ;//identity
  }
}

void GTPWindow::rotateLightsShaderOnly()
{
  #if 0
  for( int i = 0 ; i < window->scene->lights.size() ; i++ )
  {
    Shape* light = window->scene->lights[i] ;
    if( light->isCubeMap ) continue ;

    // move it back to the origin, then move it somewhere else
    
    // Modulate the position by the pattern
    //Vector newPos = light->shaderMotionFunction( window->timer.total_time, light->getCentroid() ) ;
    Vector newPos = light->shaderMotionFunction( window->rot, light->getCentroid() ) ;
    if( newPos.w == -1 )
    {
      error("stop, %s is faulty", light->name.c_str()) ;
    }
    light->meshGroup->transformSetWorld( Matrix::Translate( newPos ) ) ;
  }
  #else
  // rotate lights in the shader only, so their geometry
  // appears to move.  ray tracer will not see this.
  for( int i = 0 ; i < scene->lights.size() ; i++ )
    if( !scene->lights[i]->isCubeMap )
      scene->lights[i]->meshGroup->transformSetWorld( rotationMatrix ) ;  // DON'T modify the geometry. do in gpu.
        
  #endif
}

void GTPWindow::smoothAO()
{
  // You get a MARGINAL performance gain for using vanilla loops over functors
  // (something like 0.5 seconds for 100 runs of smoothing 12,500 vertices (16.5s vanilla loop, vs 17s lambda loop)).
  //scene->eachShape( [](Shape *shape) {
  //  shape->meshGroup->smoothTexcoord( TexcoordIndex::VectorOccludersIndex, 3 ) ;
  //} ) ;

  // ST
  // The AO at every vertex is totally different.
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
  {
    //scene->shapes[i]->meshGroup->smoothTexcoord( TexcoordIndex::VectorOccludersIndex, 3 ) ;
    scene->shapes[i]->meshGroup->smoothOperator( &AllVertex::ambientOcclusion ) ;
    scene->shapes[i]->meshGroup->smoothOperator( &AllVertex::texcoord, TexcoordIndex::VectorOccludersIndex, 3 ) ;
  }

}

void GTPWindow::drawAllText()
{
  if( textDisplay == TextDisplay::NoText ) // not none
    return ;

  beginText() ;
  ByteColor white( 255,255,255,240 ) ;
  
  int left = 12, top = 12, width=200, height=100, options = DT_LEFT ;
  
  drawText( Arial18, "gtp", white, left, top += 12, width, height, options ) ;
  
  char shadingModeStr[80] ;
  sprintf( shadingModeStr, "" ) ;
  switch( shadingMode )
  {
    case ShadingMode::SHLitCPU:
    case ShadingMode::SHLitGPU:
      // the second string specifies legendre or chebyshev
      if( SH::kernel==SH::Legendre )
        sprintf( shadingModeStr, "/Legendre %d", shSamps->bandsToRender ) ;
      else
        sprintf( shadingModeStr, "/Chebyshev %d", shSamps->bandsToRender ) ;
      break ;

    case ShadingMode::VectorOccluders:
      if( gpuCData0.vo[0] == 0 )
        sprintf( shadingModeStr, "/AO a=%.2f", gpuCData0.vo[1] ) ;
      else if( gpuCData0.vo[0] == 1 )
        sprintf( shadingModeStr, "/VO s=%.2f c=%.2f dd=%.2f ds=%.2f ss=%.2f e=%.0f", 
        gpuCData0.vo[1], gpuCData0.vo[3], gpuCData0.voI[0], 
        gpuCData0.voI[1], gpuCData0.voI[2], gpuCData0.voI[3] ) ;
      else if( gpuCData0.vo[0] == 2 )
        sprintf( shadingModeStr, "/VOw s=%.2f c=%.2f dd=%.2f ds=%.2f ss=%.2f e=%.0f", 
        gpuCData0.vo[1], gpuCData0.vo[3], gpuCData0.voI[0], 
        gpuCData0.voI[1], gpuCData0.voI[2], gpuCData0.voI[3] ) ;
      break ;

    
  }

  static char buf[2048];
  static char pos[60];
  camera->eye.printInto( pos ) ;
  sprintf( buf,
    ///"%s\n" // textdisplay type
    "%s\n" // rendering mode
    "%s%s\n" // shading mode
    "expos: %.2f maxColor: %.2f\n" // exposure, maxColor
    /////"light: %s\n" // light name // ALL LIGHTSNOW AcTIVE
    "%d shapes\n" // # shapes
    ///"%d viz\n" // # viz
    "%d verts %d faces\n"
    //"light %s\n" // light name (removed)
    "%.1f fps / runtime %.2f\n" // current fps
    //"light pos (%.2f, %.2f, %.2f)\n" // light position (removed)
    //"texmode %s\n" // texture sampling mode
    "rotation %.2lf°\n"
    "%s / pos %s"
    ,
    ////TextDisplayName[ textDisplay ],
    RenderingModeName[ renderingMode ],
    ShadingModeName[ shadingMode ],
    shadingModeStr,// additional informatino about shading mode depending on runtime params
    expos, maxColor,
    /////scene->getActiveLight()?scene->getActiveLight()->name.c_str():"NONE",
    scene->shapes.size(),
    ///scene->viz.size(),
    scene->countVertices(), scene->countFaces(), // refresh the vertex count
    ////scene->getActiveLight() ? scene->getActiveLight()->name.c_str():"no lights",
    clock.fps, clock.total_time,
    ////lightPos.x,lightPos.y,lightPos.z,
    //TextureSamplingModeName[ss]
    rot, 
    movementOn?"move":"frozen",
    pos
  ) ;

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
  // you should turn OFF culling
  // you must select the shader prior to calling this function
  // scenery includes lights that are "far"

  shaderPlain->activate() ;
  for( int i = 0 ; i < scene->scenery.size() ; i++ )
    scene->scenery[i]->draw() ;

  // Farlights include all lights that are not shapes
  shaderPlain->activate() ;
  for( int i = 0 ; i < scene->farLights.size() ; i++ )
    if( scene->farLights[i]->isActive )
      scene->farLights[i]->draw() ;
  
  CubeMap* cm = scene->getActiveCubemap() ;
  if( cm )  cm->draw() ;
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
  ///scene->spMesh->
}
  
void GTPWindow::renderWithOpacity( Scene * scene )
{
  // to render with opacity you must backface cull,
  
  CullMode oldCullMode = cullmode ;
  
  cullMode( CullMode::CullClockwise ) ; /// TURN ON CULLING

  MutexLock LOCK( scene->mutexShapeList, INFINITE );
  
  depthWrite( true ) ;
  
  // draw solid shapes
  #if 0
  //!!HIDE THE TREE by setting i=1 and adding the tree first.
  // THIS IS NOT THE NORMAL RULE
  for( int i = renderScenery?0:1 ; i < scene->opaqueShapes.size() ; i++ )
    scene->opaqueShapes[ i ]->draw() ;
  #else
  // Normal rule
  for( int i = 0 ; i < scene->opaqueShapes.size() ; i++ )
    scene->opaqueShapes[ i ]->draw() ;
  #endif

  depthWrite( false ) ;

  // Now you'd need to sort the other shapes to being a back-to-front order
  ////bool sort = true;
  ////if( sort )
  ////{
  ////  list<Shape*> transShapes ;
  ////  for( int i = 0 ; i < scene->translucentShapes.size() ; i++ )
  ////  {
  ////    real dist2 = (scene->translucentShapes[ i ]->getCentroid() - EYE).len2() ;
  ////  }
  ////}
  ////else // don't sort
    for( int i = 0 ; i < scene->translucentShapes.size() ; i++ )
      scene->translucentShapes[ i ]->draw() ;

  // draw the trans shapes back-to-front
  depthWrite( true ) ;

  cullMode( oldCullMode ) ; // put back the old setting
}

void GTPWindow::rasterize()
{
  alphaBlend( true ) ;

  // draw the scenery first, so it appears behind
  if( renderScenery )
    drawAllScenery() ;
  
  // SH shaders
  if( shadingMode == ShadingMode::SHLitCPU )
  {
    shaderSHLitCPU->activate() ;
  }
  else if( shadingMode == ShadingMode::SHLitGPU )
  {
    // if this has been precomputed
    if( raycaster->shTex )
      for( int i = 0 ; i < scene->shapes.size() ; i++ )
        scene->shapes[i]->setOnlyTextureTo( raycaster->shTex ) ;
    shaderSHLitGPU->activate() ;
  }
  else if( shadingMode == ShadingMode::PhongLit )
  {
    shaderPhongLit->activate() ;
  }
  else if( shadingMode == ShadingMode::PhongLitVertex )
  {
    shaderPhongLitVertex->activate() ;
  }
  else if( shadingMode == ShadingMode::HemicubeID )
  {
    alphaBlend( false ) ; // turn this off for id rendering.
    shaderID->activate() ;
  }
  else if( shadingMode == ShadingMode::Radiosity )
    shaderRadiosity->activate() ;
  else if( shadingMode == ShadingMode::VectorOccluders )
  {
    if( radCore->vectorOccludersTex )
      for( int i = 0 ; i < scene->shapes.size() ; i++ )
        scene->shapes[i]->setOnlyTextureTo( radCore->vectorOccludersTex ) ;

    shaderVectorOccluders->activate() ;
  }
  else if( shadingMode == ShadingMode::WaveletShaded )
    shaderWavelet->activate() ;
  else
    shaderPlain->activate() ;
 
  // now the scene, with opacity  
  renderWithOpacity( scene ) ;
  //render( scene ) ;  

  //!!ILLUSTRASTE THE CUBEMAP
  /////((CubeMap*)scene->lights[0])->viz->draw() ;
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
  CubeMap* cubeMap = scene->getActiveCubemap() ;
  if( !cubeMap )
  {
    error( "no cubemap" ) ;
    return ;
  }
  
  ////////
  //VIZ
  clearVisualizations(); 
  SHVector* xRot = cubeMap->shProjection->rotate( Matrix::RotationX( RADIANS(rot) ) ) ;
  SHVector* yRot = cubeMap->shProjection->rotate( Matrix::RotationY( RADIANS(rot) ) ) ;
  SHVector* zRot = cubeMap->shProjection->rotate( Matrix::RotationZ( RADIANS(rot) ) ) ;
  xRot->generateVisualization( 18, 18, Vector( 2,0,0 ) ) ;
  yRot->generateVisualization( 18, 18, Vector( 0,2,0 ) ) ;
  zRot->generateVisualization( 18, 18, Vector( 0,0,2 ) ) ;
  delete xRot ; delete yRot ; delete zRot ;
  ////////
}

void GTPWindow::testSHConv()
{
  CubeMap* cubeMap = scene->getActiveCubemap() ;
  if( !cubeMap )
  {
    error( "Active light not a cubemap" ) ;
    return ;
  }
  
  static real ro = 0.0 ;
  ro += 0.01 ;
  
  // clear old viz
  clearVisualizations() ;

  // make rotate copy of cube map sh.
  SHVector* cm = cubeMap->shProjection->rotate( Matrix::RotationZ( ro ) ) ;
  cm->generateVisualization( 32, 32, Vector(0,0,0) ) ;

  SHVector *zh = scene->shapes[0]->shProjection ;

  //zh->generateVisualization( 32, 32, Vector(0,4,0) ) ;

  SHVector* conv = zh->conv( cm ) ;
  conv->generateVisualization( 32, 32, Vector( 4,0,0 ) ) ;
}

void GTPWindow::testSHRotation()
{
  SHVector *rp = new SHVector() ;
  
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
  //baseAxis = Matrix::RotationZ( rotY ) << baseAxis ;

  Matrix r = Matrix::Rotation( baseAxis, rads ) ; // * Matrix::RotationX( rotY ) ;
  addDebugLine( 0, Vector(.5,0,0), 8*baseAxis, Vector(1,0,0) ) ;

  // Delete the old one.
  clearVisualizations() ;
  
  SHVector *old = rp ;
  rp = rp->rotate( r ) ;
  DESTROY(old) ;
  rp->generateVisualization( 64, 64, 0 ) ;
}

void GTPWindow::testSHSub()
{
  clearVisualizations() ;
  CubeMap* cubeMap = scene->getActiveCubemap() ;
  if( !cubeMap )
  {
    error( "no cubemap" ) ;
    return ;
  }
  
  if( !cubeMap || !cubeMap->shProjection )
  {
    error( "active light not cube map or no sh proj" ) ;
    return ;
  }
  rot += 1;

  // Take the light function, and rotate it.
  SHVector *lightFcn = cubeMap->shProjection->rotate( Matrix::RotationZ( RADIANS( rot ) ) ) ;
  lightFcn->generateVisualization( 32, 32, 0 ) ;

  // Get the lobe which cuts the piece of the light function you want.
  SHVector *lobe = scene->shapes[0]->shProjection ;

  // SUB OUT the lobe from the light function.
  SHVector *notSeen = lightFcn->subbedFloorCopy( lobe ) ;

  // now do take the original light function,
  // and subtract away the 
  SHVector *seen = lightFcn->subbedFloorCopy( notSeen ) ;
  
  seen->generateVisualization( 32, 32, Vector( 0, 10, 10 ) ) ;
}

void GTPWindow::testSHDir()
{
  CubeMap* cubeMap = scene->getActiveCubemap() ;
  if( !cubeMap )
  {
    error( "Active light not a cubemap" ) ;
    return ;
  }
  
  clearVisualizations() ;
  
  // draw the rotated light function
  Vector look = camera->getLook().normalize() ;

  // we'd get the axis by reflecting the vector from the EYE to the
  // VERTEX IN QUESTION (ie look vector to a particular vertex)
  real t = acos( look.y ) ;//look % Vector(0,1,0)

  Vector axis = Vector(0,1,0) << look ;

  Matrix m = Matrix::Rotation( axis, t ) ;

  //SHRotationMatrix shm( shSamps->bands, m ) ;
  SHVector *rotatedLightFcn = cubeMap->shProjection->rotate( m ) ; 

  // viz that
  rotatedLightFcn->generateVisualization( 32, 32, Vector(0,5,0) ) ;

  // destroy it
  DESTROY( rotatedLightFcn ) ;
}

void GTPWindow::drawSubwindow( Matrix& mproj )
{
  // unset all 
  texSubwindow->rtBind() ;
  texSubwindow->clear( ByteColor(0,0,0,180) ) ; // the subwindow can be clear in the background

  // translate to origin, but use the eye a fixed distance from the object

  //Matrix mview = Matrix::LookAt( Vector(0,0,subwindowBackDist), camera->forward, camera->up ) ;
  //Matrix mview = Matrix::Translate( 0,0,subwindowBackDist ) * Matrix::ViewingFace( camera->right, camera->up, camera->forward );
  Matrix mview = Matrix::ViewingFace( camera->right, camera->up, camera->forward ) * 
    Matrix::Translate( 0,0,subwindowBackDist ) ;
  setModelviewProjection( mview*mproj ) ;

  drawAxes() ;//activates vcDebug
  //drawDebug() ;
  //drawAllViz() ;

  // use the plain shader
  shaderPlain->activate() ;
  

  // Use the combined visualization
  if( window->renderSHLight )
  {
    if( window->scene->shCombinedViz )
    {
      window->scene->shCombinedViz->meshGroup->meshes[0]->world = rotationMatrix ;
      window->scene->shCombinedViz->draw() ;
    }
    else
    {
      // you can make it now
      warning( "No sh viz to draw" ) ;
      SHVector* lights = scene->sumAllLights() ; // sum all the lights and delete the result,
      delete lights ;
      // side effects create shviz
    }
  }
  else
  {
    // wants real light
    // draw any other light sources
    CubeMap *activeLightCM = scene->getActiveCubemap() ;
    if(activeLightCM)
    {
      activeLightCM->viz->meshGroup->meshes[0]->world = rotationMatrix ;
      activeLightCM->viz->draw() ;
    }
    else
    {
      // put the lights in the subwindow
      for( int i = 0 ; i < scene->lights.size() ; i++ )
        if( scene->lights[i]->isActive )
        {
          scene->lights[i]->meshGroup->meshes[0]->world = rotationMatrix ;
          scene->lights[i]->draw() ;
        }
    }
  }
    
    
  
  texSubwindow->rtUnbind() ;
}

void GTPWindow::drawSurfaces()
{
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
    setModelviewProjection( Identity ) ;

    shaderVtcSurfaces->activate() ;
    texRaytrace->draw() ;
  }

  if( subwindow )
  {
    Matrix xform = Matrix::Scale( 0.25, 0.25, 1 ) * Matrix::Translate( 0.73, 0.73, 0 ) ;
    setModelviewProjection( xform ) ;

    shaderVtcSurfaces->activate() ;
    texSubwindow->draw() ;
  }
  
  if( textDisplay ) {
    setModelviewProjection( Identity ) ;
    alphaBlend( true ) ; // THIS MUST BE ON OTHERWISE THE TEXT PLANE BLOCKS THE STUFF YOU SHOULD SEE IN THE SCENE
    drawAllText() ; // draw alll the text, includes begin/endText calls.
  }
  
  setFillMode( oldMode ) ;
}

void GTPWindow::updateGPUData()
{
  // Update the gpu shader variables
  // use one active light, for now
  setWorld( Identity ) ;
  camera->eye.writeFloat4( gpuCData0.eyePos ) ;
  
  // no ambient
  Vector(0,0,0).writeFloat4( gpuCData0.ambientLightColor[0] ) ;
  gpuCData0.activeLights[ 0 ] = 0 ; // ambient

  int numValidLights=0;
  for( int i = 0 ; i < scene->lights.size() && numValidLights < MAX_LIGHTS ; i++ )
  {
    Shape* light = scene->lights[i];
    
    // if it's NOT a cubemap, then it has a point.
    if( light->isCubeMap )  continue ; // skip
    //if( !light->isActive ) continue ; //skip
    numValidLights++;

    // Just moves the lights by the rotation matrix
    Vector pos = light->getCentroid() * rotationMatrix ;

    // Actually move the lights by TIME and centroid
    //Vector pos = light->shaderMotionFunction( timer.total_time, light->getCentroid() ) ;

    // Move the lights by the ROTATION ANGLE and the centroid.
    //Vector pos = light->shaderMotionFunction( window->rot, light->getCentroid() ) ;

    Vector lightColor = scene->lights[i]->material.ke ;
    
    // COPY THE LIGHT INTO BOTH SPECULAR AND DIFFUSE
    pos.writeFloat4( gpuCData0.diffuseLightPos[i] ) ;
    lightColor.writeFloat4( gpuCData0.diffuseLightColor[i] ) ;
    pos.writeFloat4( gpuCData0.specularLightPos[i] ) ;
    lightColor.writeFloat4( gpuCData0.specularLightColor[i] );
  }

  gpuCData0.activeLights[ 1 ] = numValidLights ; // diffuse
  gpuCData0.activeLights[ 2 ] = numValidLights ; // specular
  //info( "There are %d lights", numValidLights ) ;
  
  gpuCData0.expos[0] = expos ;
  gpuCData0.expos[1] = maxColor ;
  updateGPUCData( 0 ) ; // entry is the lights.
  
  // update the texture rotation matrix
  rotationMatrix.transposedCopy().writeFloat16RowMajor( gpuCData3.texRot ) ;
  updateGPUCData( 3 ) ; 

  
}

void GTPWindow::drawRaytraceOnly()
{
  // pull the data from rtCore
  rtCore->writePixelsToByteArray() ;
  texRaytrace->copyFromByteArray( (BYTE*)&rtCore->bytePixels[0], 4, rtCore->getNumPixels() ) ;
  // I don't lock the raytracer's pixels array,
  // so this doesn't have a huge performance impact
  // (ie it _doesn't_ halt the raytracer)

  // we are drawing in the canonical view volume here straight away.
  //Matrix xform = Matrix::Translate( 0.73, -0.73, 0 ) * Matrix::Scale( 0.25, 0.25, 1 ) ;
  setModelviewProjection(Identity) ;

  shaderVtcSurfaces->activate() ;
  texRaytrace->draw() ;
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

  // HIDE BACKGROUND
  clear( 0 ) ;

  // SHOW BACKGROUND
  ///clear( scene->bgColor.toByteColor() ) ; // clearing to full alpha so screenshots have a background

  ////testSHRotation() ;
  ////testSHConv() ;
  ////testSHCubemapRot() ;
  ////testSHSub() ;
  ////testSHDir() ;
  #pragma region rotation matrices for the scene
  gpuCData0.vo[2] = clock.total_time ; //1.0/60. ; // add 1/60 seconds. this represents "time".
  ///printf( "Total time %f\r", clock.total_time ) ;
  //rot += .5 ;
  if( rot >= 360 ) rot -= 360 ;
  else if( rot <= -360 ) rot += 360 ;
  real radians = RADIANS( rot ) ;
  
  // moving axis
  #define BORINGAXIS 1

  #if BORINGAXIS
  //BORINGAXIS
  Vector baseAxis( 0,1,0 ) ;   // use y-axis (for boring animations)
  #else
  Vector baseAxis(1,0,0) ;
  baseAxis.rotateY( radians ) ;  // rotate the axis, looks more interesting
  #endif
  
  
  rotationMatrix = Matrix::Rotation( baseAxis, radians ) ;

  // Rotate the lights
  rotateLightsShaderOnly() ;
  #pragma endregion

  int ci = getColorIndexForShadingMode( (ShadingMode)shadingMode ) ;
  if( ci == -1 )
    maxColor = 1.0 ; // assume it's 1.0
  
  updateGPUData() ;

  if( waveletDidFinishRunning && shadingMode==ShadingMode::WaveletShaded )
  {
    CubeMap* cubeMap = scene->getActiveCubemap() ;
    
    if( !cubeMap )
    {
      error( "need a cubemap to wavelet light" ) ;
      return ;
    }
  
    static real lastRot = 2000 ; // ensure this runs at least once (rotation of 2000 is not possible, it will wrap)
    if( rot != lastRot )
    {
      printf( "Wavelet rotating to %f degrees\r", rot ) ;
      cubeMap->rotate( rotationMatrix ) ;
      cubeMap->compress() ; // compress it right away, to wavelet domain.
      lastRot = rot ;
    }

    //window->scene->cubeMap->rotateWavelet( matrixRotation ) ; // this doesn't work, you can't rotate a wavelet the same way
    // you rotate a space function
    scene->waveletLight() ;
    window->colorNormalize( ColorIndex::WaveletComputed ) ;
    window->updateVertexBuffers() ;
  }
  else if( shDidFinishRunning )
  {
    static real lastRot = 2000 ; // ensure this runs at least once (rotation of 2000 is not possible, it will wrap)
    if( rot != lastRot )
    {
      printf( "SH rotating to %f degrees\r", rot ) ;
      scene->shRotateAllLights( rotationMatrix ) ;
      lastRot = rot ;
    }
    if( shadingMode==ShadingMode::SHLitCPU )
    {
      //CPU
      scene->shLightCPU( rotationMatrix ) ;
      colorNormalize( ColorIndex::SHComputed ) ;
      updateVertexBuffers() ;
    }
    else if( shadingMode==ShadingMode::SHLitGPU )
    {
      //GPU
      scene->shLightGPU( rotationMatrix ) ;
    }
  }
  //else if( shadingMode == ShadingMode::Radiosity )
  //  colorNormalize( ColorIndex::RadiosityComp ) ;

  // make projection and look matrices
  Matrix mproj = Matrix::ProjPerspectiveRH( RADIANS( 45 ), (real)window->getWidth()/window->getHeight(), .1, 1000 ) ;
  Matrix mview = Matrix::LookAt( camera->eye, camera->getLook(), camera->up ) ;
  
  
  setModelviewProjection( mview*mproj ) ; // flushes to gpu, which also flushes eye data
  //setModelviewProjection( mproj.transpose()*mview.transpose() ) ; // flushes to gpu, which also flushes eye data

  // RASTERIZATION
  if( containsFlag(renderingMode, Rasterize) )
    rasterize() ;
  
  if( containsFlag( renderingMode, RenderingMode::DebugLines ) )
  {
    drawAxes() ;
    drawDebug() ;
    drawAllViz() ;  
  }

  if( containsFlag( renderingMode, RenderingMode::OctreeViz ) )  drawOctree() ; 

  if( subwindow )  drawSubwindow( mproj ) ;

  drawSurfaces() ;
  
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

void GTPWindow::addDebugRayLock( const Ray& ray, const Vector& startColor, const Vector& endColor )
{
  MutexLock lock( scene->mutexDebugLines, INFINITE ) ;
  addDebugLineLock( ray.startPos, startColor, ray.getEndPoint(), endColor ) ;
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

void GTPWindow::shCompute()
{
  shDidFinishRunning = false ;

  char pbName[255];
  sprintf( pbName, "%s shadow pass 1", SH::kernelName ) ;
  ParallelBatch *batchSH1 = new ParallelBatch( pbName, NULL ) ;

  sprintf( pbName, "%s interreflect pass 2", SH::kernelName ) ;
  ParallelBatch *batchSH2 = new ParallelBatch( pbName, new Callback0( [](){
    
    // JOB TO DO WHEN BATCH2 IS __DONE__
    // SMOOTH SH (if you use a random start point in the samples)

    // add the interreflections together
    if( window->doInterreflect )
      window->scene->shAddInterreflections() ; // can be done on ANY thread
    else
    {
      // just set up the shDiffuseSummed pointer to copy the 1-pass diffuse parameter
      window->scene->eachVert( [](AllVertex*v){
        v->shDiffuseSummed=new SHVector( *v->shDiffuseAmbient ) ; // copy because dtor will screw up if not
      }) ;
    }
    
    // not bothering with this anymore
    // because this is computed in the shader now.
    // call SHLIGHT
    //window->scene->shLight( Matrix() ) ; // identity for rotation to start with
    // then color normalize
    //window->colorNormalize( ColorIndex::SHComputed ) ;

    threadPool.createJobForMainThread( "write sh texcoords",
    new Callback0( [](){
      window->raycaster->createSHTexcoords( window->scene ) ; //writes diffuse components as texcoords for gpu sh comps
      window->updateVertexBuffers() ;
      window->shDidFinishRunning = true ;
      window->programState = ProgramState::Idle ;
    } ) ) ;
    
  } ) ) ;
    
  // Prepare the batches
  raycaster->QSHPrecomputation( batchSH1, batchSH2, scene, 120 ) ;

  // Submit batch1 before batch2 (order matters)
  threadPool.addBatch( batchSH1 ) ;
  threadPool.addBatch( batchSH2 ) ;
}

int GTPWindow::getColorIndexForShadingMode( int mode )
{
  switch( mode )
  {
    case ShadingMode::Radiosity:
      return ColorIndex::RadiosityComp;
    case ShadingMode::SHLitCPU:
    case ShadingMode::SHLitGPU:
      return ColorIndex::SHComputed;

    case ShadingMode::WaveletShaded:
      return ColorIndex::WaveletComputed;

    default:
      return -1 ;
  }
}

void GTPWindow::updateMaxColor( int colorIndex )
{
  maxColor = 0 ;
  // find the maximum (brightest) color, all channels.
  for( int s = 0 ; s < scene->shapes.size() ; s++ )
  {
    Shape *shape = scene->shapes[s];
    
    for( int mn = 0 ; mn < shape->meshGroup->meshes.size() ; mn ++ )
    {
      Mesh* mesh = shape->meshGroup->meshes[ mn ] ;
      for( int i = 0 ; i < mesh->verts.size() ; i++ )
        clamp<real>( maxColor, mesh->verts[i].color[ colorIndex ].max(), 1e6 ) ; // bound below by the color,
        // so by the end you have the max in each channel here
    }
  }
    
  //printf( "MAX COLOR %f     \r", maxColor ) ;
}

void GTPWindow::colorNormalize( ColorIndex colorIndex )
{
  // retrieve the maxColor for the shading mode
  updateMaxColor( colorIndex ) ;

  real YD = expos * ( (expos/maxColor) + 1 ) / ( expos+1 ) ;

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
        //mesh->verts[i].color[ colorIndex ] /= largest ; //preserve color balance
        mesh->verts[i].color[ colorIndex ] *= YD ; //preserve color balance
    }
  }
}

void GTPWindow::updateVertexBuffers()
{
  // if you're not the main thread, you need to
  // ask this to happen ON THE MAIN THREAD
  if( ! threadPool.onMainThread() )
  {
    warning( "A call to updateVertexBuffers was made on NOT the main thread. Re-issuing.." ) ;
    threadPool.createJobForMainThread( "update vbs", new Callback0( [](){window->updateVertexBuffers();} ) ) ;
    return ;
  }

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
  shSamps = new SHSampleCollection( shHeader.shBands, shHeader.shSamples, shHeader.shSamples/10 ) ;

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
  scene->computeSpacePartition() ;


}

