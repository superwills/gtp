#ifndef RAYTRACINGCORE_H
#define RAYTRACINGCORE_H

#include "ViewingPlane.h"
#include "../math/Vector.h"
#include "../scene/Scene.h"
#include "../geometry/Ray.h"
#include "../window/ISurface.h"
#include "../util/StopWatch.h"

class Scene ;
struct D3D11Surface ;
struct RayCollection ;

// The raytracer should accept:
//   1)  a scene to trace
//   2)  a camera eye position and direction
//   3)  trace mode options (like # rays to use per pixel etc)
// The raytracer should output:
//   1)  an array of pixels
//   2)  possibly an array of depth values
//   THAT IS ALL.
// 

// Let fogmode be a framebuffer thing.
enum FogMode {
  FogNone, FogLinear, FogExp, FogExp2
} ;

// need a FRAMEBUFFER class with depth and #bounces per pixel (use in normalization step)
struct Fragment
{
  Vector color ;
  Vector normal ;  // interpolated normal of surface at this fragment
  real depth ;
  int bounces ;
  bool bgHit ;

  static Fragment Zero ;

  Fragment(){
    zero();
  }

  Fragment( const Vector& iColor, const Vector& iNormal, real iDepth, int iBounces, bool iBgHit )
  {
    set( iColor, iNormal, iDepth, iBounces, iBgHit ) ;
  }

  void set( const Vector& iColor, const Vector& iNormal, real iDepth, int iBounces, bool iBgHit )
  {
    color = iColor ;
    normal = iNormal ;
    depth = iDepth ;
    bounces = iBounces ;
    bgHit = iBgHit ;
  }

  // zero the fragment
  void zero()
  {
    set(0,0,0,0,0) ;
  }

  bool isDirectBackgroundHit()
  {
    // a ray went and hit the bg without bouncing.
    return bgHit && bounces==0 ;
  }
  
  Vector foggedColor( int fogMode, Vector fogColor, real density, real furthestDistance )
  {
    // DEPTH is already eye distance (eye to point this color occurs at).

    // LINEAR.  This is how much of the original color you get. The rest, is the fog color.
    real blendFactor ;

    switch( fogMode )
    {
    case FogMode::FogLinear:
      blendFactor = ( furthestDistance - depth ) / furthestDistance ;
      if( blendFactor < 0 )  blendFactor = 0 ;
      break ;
    case FogMode::FogExp:
      blendFactor = exp( -(density*depth) ) ;
      break ;
    case FogMode::FogExp2:
      blendFactor = exp( -(SQUARE(density*depth)) ) ;
      break ;
  
    case FogMode::FogNone:
    default:
      return color ; // UNFOGGED color
    }
    
    return (blendFactor * color) + (( 1 - blendFactor ) * fogColor) ;
  }

} ;

struct FragmentGroup
{
  // encapsulates list of fragments and supports averaging operations
  list<Fragment> fragments ;

  void add( const Fragment& frag )
  {
    fragments.push_back( frag ) ;
  }

  real getLargestColorComponent()
  {
    real maxComp=1 ;

    for( list<Fragment>::iterator frag = fragments.begin() ; frag != fragments.end() ; ++frag )
    {
      if( frag->bgHit && frag->bounces == 0 ) continue ; // direct background hits aren't considered "colors"
      
      real biggestColor = frag->color.max() ;
      if( biggestColor > maxComp )  maxComp = biggestColor ;
    }

    return maxComp ;
  }

  bool isAllBgHits()
  {
    for( list<Fragment>::iterator frag = fragments.begin() ; frag != fragments.end() ; ++frag )
      if( !frag->bgHit ) return false ; // at least one was not a bg hit
    return true ;
  }

  // Gets you the averaged color for this fragment.
  Vector getSumColor()
  {
    Vector color ;

    for( list<Fragment>::iterator frag = fragments.begin() ; frag != fragments.end() ; ++frag )
      color += frag->color ;

    return color ;
  }

  // Gets you the averaged color for this fragment.
  // YOU DON'T USUALLY USE THIS TO GET THE FINAL COLOR
  // because the raytracer works in the power loss
  // INTO the color value (so you normally just sum them)
  Vector getAvgColor()
  {
    return getSumColor()/fragments.size() ;
  }

  Vector getSumNormal()
  {
    Vector normal ;

    for( list<Fragment>::iterator frag = fragments.begin() ; frag != fragments.end() ; ++frag )
      normal += frag->normal ;

    return normal ;
  }

  Vector getAvgNormal()
  {
    return getSumNormal() / fragments.size() ;
  }

  real getSumDepth()
  {
    real depth=0 ;

    // this means this function returns _0_ depth for
    // REALLY_FAR fragments
    for( list<Fragment>::iterator frag = fragments.begin() ; frag != fragments.end() ; ++frag )
      if( frag->depth != 1e4 ) // not REALLY_FAR
        depth += frag->depth ;

    return depth ;
  }

  real getAvgDepth()
  {
    return getSumDepth() / fragments.size() ;
  }

  real getSmallestDepth()
  {
    real smallest = 1e4 ; // well, i need a default value..
    
    for( list<Fragment>::iterator frag = fragments.begin() ; frag != fragments.end() ; ++frag )
      if( frag->depth!=0 && frag->depth < smallest ) // not REALLY_FAR
        smallest = frag->depth ;

    return smallest ; // if this is still 1e4, you're looking at a bg fragment
    //if( smallest == 1e4 ) return 0 ; // avoid returning 1e4 for REALLY_FAR fragments
    //else return smallest ;
  }

  real getLargestDepth()
  {
    real largest = 0 ;
    for( list<Fragment>::iterator frag = fragments.begin() ; frag != fragments.end() ; ++frag )
      if( frag->depth != 1e4 && frag->depth > largest ) // not don't use REALLY_FAR as "largest depth"
        largest = frag->depth ;
    return largest ;
  }

  real getFirstFragmentDepth()
  {
    return fragments.begin()->depth ;
  }
} ;

struct FrameBuffer
{
  // each entry is a LIST of fragments.
  // How you choose to blend them at the end (when
  // filling the pixel buffer) is up to you.
  vector< FragmentGroup > fragmentGroups ;

  // If you don't want to use fragments, you can simply have the raytracer
  // write colors directly in here
  vector< Vector > colors ; // intermediate colorbuffer, for storing averaged fragment colors (or depths as colors or whatever you want to viz.)

  static int fogMode ;
  static Vector fogColor ;
  static real fogFurthestDistance ; // REALLY_FAR.
  static real fogDensity ;

  int rows, cols ;

  FrameBuffer( int iRows, int iCols )
  {
    rows = iRows ;
    cols = iCols ;
    fragmentGroups.resize( rows*cols ) ;
    colors.resize( rows*cols ) ;

    // setup fog defaults
    fogMode = FogLinear ;
    fogColor = 1 ;
    fogFurthestDistance = 1e4 ;
    fogDensity = .0001 ;
  }

  inline int size(){ return fragmentGroups.size() ; } // conv

  void clear()
  {
    for( int i = 0 ; i < size() ; i++ )
      fragmentGroups[i].fragments.clear() ;
  }

  void addFragment( int row, int col, const Fragment& fragment ) {
    
    int idx = row*cols + col ;
    fragmentGroups[idx].fragments.push_back( fragment ) ; // append

  }

  real getLargestColorComponent()
  {
    real m = 0 ;
    for( int i = 0 ; i < size() ; i++ )
    {
      FragmentGroup &fg = fragmentGroups[i] ;
      real maxc = fg.getLargestColorComponent() ;
      if( maxc > m ) m=maxc ;
    }
    return m ;
  }

  real getFurthestFragment()
  {
    real d = 0 ;
    for( int i = 0 ; i < size() ; i++ )
    {
      FragmentGroup &fg = fragmentGroups[i] ;
      real depth = fg.getLargestDepth() ;
      if( depth < 1e4 && depth > d )
        d = depth ;
    }
    return d ;
  }

  real getClosestFragment()
  {
    real c = 1e4 ;
    for( int i = 0 ; i < size() ; i++ )
    {
      FragmentGroup &fg = fragmentGroups[i] ;
      real depth = fg.getSmallestDepth() ;
      if( depth < c )
        c = depth ;
    }

    return c ;
    //if( c == 1e4 ) return 0 ;// no closest fragment (all are REALLY_FAR ie you have an empty scene!)
    //else return c ;
  }

  #pragma region fills the colors array

  void colorRaw( int rpp )
  {
    // just gets sum color from raytracer with no extra post processing
    for( int i = 0 ; i < colors.size() ; i++ )
      colors[i] = fragmentGroups[i].getSumColor()/rpp ;
  }

  // get the largest magnitude color and divide each summed color by it.
  void colorNormalized( int rpp )
  {
    real maxc=getLargestColorComponent()*rpp ;
    //printf( "Maxc=%f\n", maxc ) ;
    for( int i = 0 ; i < colors.size() ; i++ )
    {
      if( fragmentGroups[i].isAllBgHits() )
        colors[i] = fragmentGroups[i].getSumColor()/rpp ; // just divide by rpp for pure bg pixels. this leaves the bg bright.
      else
        colors[i] = fragmentGroups[i].getSumColor()/maxc ; // otherwise, normalize
    }
  }

  // debug
  void colorByNormals( int rpp )
  {
    for( int i = 0 ; i < colors.size() ; i++ )
    {
      colors[i] = fragmentGroups[i].getAvgNormal() ; // avg so rpp doesn't matter
    }
  }

  // 
  void colorWithFog( int rpp )
  {
    colorRaw( rpp );
  }

  void colorByDepth( int rpp )
  {
    real furthestFrag = getFurthestFragment() ;
    real closestFrag = getClosestFragment() ;
    real depthRange = furthestFrag - closestFrag ;
    info( "Furthest fragment %f, closest %f, range %f", furthestFrag, closestFrag, depthRange ) ;
    for( int i = 0 ; i < colors.size() ; i++ )
    {
      real depth = (fragmentGroups[i].getAvgDepth() - closestFrag) / depthRange ;
      colors[i] = Vector(0,depth,0) ;
    }
  }

  void colorByNumFragments( int rpp )
  {
    // 20 is a lot of fragments
    int maxFrags = 1 ;
    for( int i = 0 ; i < colors.size() ; i++ )
      if( fragmentGroups[i].fragments.size() > maxFrags )
        maxFrags = fragmentGroups[i].fragments.size() ;

    printf( "Most frags=%d\n", maxFrags ) ;
    for( int i = 0 ; i < colors.size() ; i++ )
    {
      real perc = ((real)fragmentGroups[i].fragments.size()) / maxFrags ;
      colors[i] = Vector(perc,0,0) ;
    }
  }

  #pragma endregion
} ;


enum TraceType{
  Whitted, Distributed, Path
} ;

extern const char* TraceTypeName[] ;

class RaytracingCore
{
  /// The number of bounces to use
  /// before terminating the back-tracing
  /// of a particular ray
public:
  // when do we drop a ray?
  // after maxBounces..
  int maxBounces ; // 3 is very good.

  // ..or, when it's power is below a certain interreflection threshold
  real interreflectionThreshold² ;

  bool realTimeMode ;

private:
  int rows, cols ;

  // I keep a copy of the active cubemap WHEN YOU START the ray trace,
  // and use it throughout the trace.
  CubeMap* cubeMap ;

  // A cached set of the view rays used to create
  // the scene, these only have to be reconstructed
  // if the viewing angle changes OR if you really want
  // new random rays (still in the same general directions)
  // for each frame rendered.
  // the latter could be done by jittering the
  // rays a bit more.
  vector<Ray> viewRays ;

public:
  static RayCollection* rc ;
  
private:
  // number 
  int raysPerPixel, sqrtRaysPerPixel ;
  bool stratified ;
  TraceType traceType ;
  int raysDistributed ;
  
  int raysCubeMapLighting ;
  //StopWatch stopWatch ;
  
  int numJobs ;
  int numJobsDone ;

public:
  bool showBg ; // toggles whether the bg renders

  ViewingPlane *viewingPlane ;
  //vector<Vector> pixels ; //switch to floating point colors until buffer flip
  FrameBuffer *frameBuffer ;
  vector<ByteColor> bytePixels ; // keep this around because you're going to alloc/dealloc too much each frame if you discard it.
  // really when rt is done you can get rid of "PIXELS" but you don't ERALLY need to saev the 15 odd MB of ram.

  inline int getWidth() {return cols;}  
  inline int getHeight() {return rows;}  
  inline int getNumPixels() {return rows*cols;}
  
  RaytracingCore( int iNumRows, int iNumCols,
    int iRaysPerPixel,
    bool iStratified,
    string iTraceType,
    int iRaysDistributed,
    int iRaysCubeMapLighting,
    int iMaxBounces, real iInterreflectionThreshold,
    bool iShowBg
  ) ;
  ~RaytracingCore() ;

  void clear( Vector color ) ;

  // writes the pixels in "pixels" to bytePixels
  void writePixelsToByteArray() ;

  Vector castRTSH( Ray& ray, Scene *scene ) ;

  // ray is the ray along which you are casting
  // scene is the scene to cast the ray into
  // rgbJuice is the red,green,blue "strength" left in the ray
  // mediaEta is the eta of the material the ray was fired FROM
  // bounceNum is the number of the bounce
  // fragments: every time a fragment is generated it is added to the list
  // for post-processing.
  // Original (if statemented) function.
  Vector cast( Ray& ray, Scene *scene ) ;

  // uses russian roulette style sampling
  //Vector castRoulette( Ray& ray, Scene *scene ) ;

  // brdf
  Vector brdfCast( Ray& ray, Scene *scene ) ;

  void traceRectangle( int startRow, int endRow, int startCol, int endCol, Scene *scene ) ;

  void doneSingleJob() ;
  void doneCompletely() ;
  void raytrace( const Vector& eye, const Vector& look, const Vector& up, Scene *scene ) ;

  void copyToSurface( D3D11Surface *surf ) ;

  
} ;

#endif