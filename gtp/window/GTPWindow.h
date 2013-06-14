#ifndef GTP_WINDOW_H
#define GTP_WINDOW_H

#include "D3D11Window.h"
#include "InputMan.h"
#include "Properties.h"

#include "../util/StdWilUtil.h"
#include "../util/StopWatch.h"

#include "../scene/Scene.h"
#include "../threading/ThreadPool.h"
#include "../rendering/RayCollection.h"

#include <vector>
#include <map>
using namespace std ;

struct Camera ;
class Scene ;
struct D3D11Surface ;
class SH ;
struct SHSampleCollection ;
class SHVector ;
struct ISurface ;
class RaytracingCore ;
class RadiosityCore ;
class VectorOccluders ;
struct FullCubeRenderer ;
struct Cube ;
struct SHSample ;
struct Raycaster ;

// bitwise or'able
enum RenderingMode
{
  Rasterize  = 1 << 0,//1
  Raytrace   = 1 << 1,//2
  DebugLines = 1 << 2,//4
  OctreeViz  = 1 << 3 //8 // since this state is last,
  // the other states won't combine with it
} ;

enum ShadingMode
{
  PlainDiffuse = 0, // original diffuse shape colors
  PhongLit,         // phong lighting
  PhongLitVertex,
  SHLitCPU,            // shlighting
  SHLitGPU,            // shlighting
  HemicubeID,       // render ids for hemicube
  Radiosity,
  VectorOccluders,
  WaveletShaded
} ;

enum CompressionMode
{
  NoCompression,//0
  SphericalHarmonics,//1
  HaarWaveletCubemap,//2
  LatticeWaveletCubemap,//3
  SphericalWavelet//4
} ;
extern char* CompressionModeName[] ;

// these cannot be combined
enum TextDisplay
{
  NoText,
  InfoOnly
} ;

enum SpacePartitionType
{
  PartitionOctree,
  PartitionKDTree,
  PartitionBSPTree
} ;

enum ProgramState
{
  // Idle means ready to send commands,
  // start ray trace, etc
  Idle,

  // Either raytracing or SH computing,
  // so you can't start ANOTHER rt while this one is running.
  // Also the update interval is reduced, and a sleep call is made
  // in the render loop to make the _computer_ more responsive 
  // and usable (for other tasks) when a render is happening.
  Busy
} ;

extern
char *RenderingModeName[];

extern
char *ShadingModeName[] ;

extern
char *TextDisplayName[];

extern
char *VertexColorizationModeName[];

// don't let the compiler pad it.
#pragma pack( push )
#pragma pack( 1 )
struct HeaderSHFile
{
  char magic[4];
  char major, minor;

  int numShapes ;
  int numLights ;

  int shBands ;
  int shSamples ;     // purely informational: #samples used to calc sh coeffs
  int dataStart ; 
} ;
#pragma pack( pop )

extern char* DEFAULT_DIRECTORY ;
extern char* DEFAULT_FILTER ;
extern char* DEFAULT_PROPERTIES_FILE ;

class GTPWindow : public InputMan, public D3D11Window
{
public:
  Properties *props ;
  ProgramState programState ;
  RayCollection *rayCollection ; // a collection of ray direction vectors evenly distributed on the sphere.
  
  SHSampleCollection *shSamps ;
  

  real rot ; // rotation of the lightmap
  real subwindowBackDist ;

  // these get cached for re-use
  Matrix proj, view, rotationMatrix ;

  bool subwindow ;
  D3D11Surface * texSubwindow ;
  D3D11Surface * texRaytrace ;

  /// Whether to use a raytracer or
  /// a rasterizer when drawing
  int renderingMode ;
  int shadingMode ;
  bool renderScenery ; // primarily the cubemap
  bool renderSHLight ;  // render the light in SH form or no

  /// What sort of text overlay to
  /// show, options enumerated by
  /// enum TextDisplay
  int textDisplay ;
  
  // current sampler state mode
  int ss ;

  bool shCubeMapViz ;
  bool shDidFinishRunning, waveletDidFinishRunning, doInterreflect ;

  // fstop
  real expos ;
  real maxColor ; // measured maximum in final color

  HANDLE mutexPbs ;
  list<ProgressBar*> pbs ;

private:
  /// The furthest text allowed
  /// do be displayed
  real textDisplayBackplane ;

public:
  // other globally accessible objects
  Scene *scene ;
  Camera *camera ; // the current camera you're controlling
  Camera *viewCam ;// the viewing camera
  Camera *lightCam ; // the 'camera' for moving the light
  Shape *lightForCam ; // the light ON the camera
  RaytracingCore *rtCore ;
  RadiosityCore *radCore ;
  Raycaster *raycaster ;
  FullCubeRenderer *fcr ;

  int spacePartitionType ;

  FPSCounter clock ;

  // A global timer that long processes can reset/use.
  // Only 1 of these, so if it gets reset, then you are SOL.
  Timer timer ;

  bool movementOn ;

  /// array of simple points that is drawn out once
  /// per frame, used in debugging.
  bool debugPointsChanged,debugLinesChanged,debugTrisChanged ;
  vector<VertexC> debugPoints ;
  vector<VertexC> debugLines ;
  vector<VertexC> debugTris ;

  TVertexBuffer<VertexC> * axisLines ;
  TVertexBuffer<VertexC> * axisPoints ;

  VertexBuffer *vbDebugPoints, *vbDebugLines, *vbDebugTris ;

  int MAX_LIGHTS ;

  vector<WaveParam> waveGroup ;

public:
  GTPWindow( HINSTANCE hInst, TCHAR* windowTitleBar, int windowXPos, int windowYPos, int windowWidth, int windowHeight ) ;
  ~GTPWindow() ;

  void textDisplayBackplaneFurther() ;
  void textDisplayBackplaneNearer() ;

  void screenshotMulti( int numShots, real timeInterval, real angleIncrementSize ) ;

  void step() ;

  /// Whether to use the mouse and keyboard
  /// to control the camera position and viewing angle
  void toggleMovement() ;

  void initTextures() ;
  
  // Bake in the current rotation matrix
  // by changing the LIGHTSOURCE geometry
  // this makes the ray tracer correct, as well as
  // the 
  void bakeRotationsIntoLights() ;

  void rotateLightsShaderOnly() ;

  void smoothAO() ;

  void drawAllText() ;

  void drawAllScenery() ;

  // Render the visualizations
  void drawAllViz() ;

  void drawDebug() ;

  void drawOctree() ;

  // Got fed up of dealing with missing variables in
  // the D3D11Window base class, so I moved this function
  // up to the GTPWindow class.
  void renderWithOpacity( Scene * scene ) ;

  // Selects a SHADER (depending on scene rendering mode)
  // and renders the scenes
  void rasterize() ;

  void clearVisualizations() ;
  
  void testSHCubemapRot() ;
  void testSHConv() ;
  void testSHRotation() ;
  void testSHSub() ;
  void testSHDir() ;

  void drawSubwindow( Matrix& mproj ) ;
  void drawSurfaces() ;
  void updateGPUData() ;
  void drawRaytraceOnly() ;
  void draw() ;
  
  // ACTIVATES vcDEBUG SHADER
  void drawAxes() ;

  // LOCK REQUIRED
  void addDebugPoint( const Vector& pt, const Vector& color ) ;
  void addDebugLine( const Vector& start, const Vector& startColor, const Vector& end, const Vector& endColor ) ;

  

  // these lock for you
  void addDebugLineLock( const Vector& start, const Vector& startColor, const Vector& end, const Vector& endColor ) ;
  void addDebugRayLock( const Ray& ray, const Vector& startColor, const Vector& endColor ) ;
  void addDebugTriLock( const Triangle& tri, const Vector& color ) ;
  void addDebugTriLock( PhantomTriangle* pTri, const Vector& color ) ;

  void addSolidDebugTri( Vector offset, Triangle& tri, const Vector& color ) ;
  void addSolidDebugTri( Triangle& tri, const Vector& color ) ;
  void addSolidDebugTri( Vector offset, PhantomTriangle* pTri, const Vector& color ) ;
  void addSolidDebugTri( PhantomTriangle* pTri, const Vector& color ) ;

  void drawDebugPoints() ;
  void drawDebugLines() ;
  void drawDebugTris() ;
  
  // Saves the sh file
  void shSave() ;

  // boots the sh solver
  void shCompute() ;

  int getColorIndexForShadingMode( int mode ) ;

  // updates maxColor in this class
  void updateMaxColor( int colorIndex ) ;

  // normalize baked in colors after lighting
  // this is purely aesthetic (so you can see
  // the colors on the limited range display),
  // and doesn't affect accuracy of _computation_
  void colorNormalize( ColorIndex colorIndex ) ;

  // update the vertex buffers for
  // each shape (effectively updating colors)
  void updateVertexBuffers() ;
  
  void save( char * outname ) ;

  void load( char * infile ) ;

} ;

// "Superglobal" -- instantiated in main.cpp
extern GTPWindow *window ;


#endif