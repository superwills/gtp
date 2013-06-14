#ifndef HEMICUBE_H
#define HEMICUBE_H

// Hemicube is __almost__ a singleton class.
// You should only have 1 hemicube in the
// radiosity renderer, then move that around
// and render to the surface multiple times.

// to increase the values of the depth buffer,
// INCREASE HEMI_NEAR.
// The Z-buffer is only 0 exactly at HEMI_NEAR.
// After that, the value goes up FAST, like ln(x),
// and it only reaches 1.0 at HEMI_FAR.
// It will be .5 about 1/8 of the way to HEMI_FAR.
//#define HEMI_NEAR 0.035
//#define HEMI_FAR  25

// Trying to combat depth buffer rpoblems
//#define HEMI_NEAR 0.5
//#define HEMI_FAR  100
#define HEMI_NEAR 1
#define HEMI_FAR  25

#define HEMI_FOV  90.0

#include "../window/D3D11Window.h"
#include "../math/Vector.h"
#include "../geometry/Triangle.h"
#include "../window/ISurface.h"

struct D3D11Surface ;
class Window ;

#include "../window/D3DDrawingVertex.h"
#include "../math/EigenUtil.h"
#include "VizFunc.h"

extern char* SidesNames[] ;

////////////////////
/// THE HEMICUBE ///
/// There's only one and it is moved about the scene.
/// even though we have multiple cores,
/// there's only one gpu and all d3d device stuff
/// must run off main thread. So 1 hemicube.
struct Hemicube
{
  D3D11Window *win ;

  enum HemiSides {
    FRONT=0,// the only full face
    Top, Right, Bottom, Left
    // consider renaming as
    // TOP, Left, Right, Back, Front
  } ;

  real sideLen ; // length of long side of hemi in world space coords

  /// Number of pixels across
  /// the wider sides of the hemicube
  int pixelsPerSide ;

  // 8 vertices that are the corners of
  // the hemicube.  
  // These values are generated once (in ctor) and cached.
  Vector A, B, C, D,   E, F, G, H ;

  // The hemicube has 5 "surfaces",
  // one full NxN surface, and
  // 4 half Nx(N/2) surfaces.
  D3D11Surface* surfaces[ 5 ] ;
  // We use pointers to surfaces to delay
  // instantiation until we are ready
  // These 5 surfaces are created once
  // and re-used throughout the program
  // as the hemicube is MOVED from
  // patch to patch

  /// There are 2 form factor cosines
  /// matrices:  one for the top of the
  /// hemicube, and one for the __side(s)__
  /// of the hemicube.
  /// A "form factor" here is the solid angle
  /// of the unit sphere a given pixel on
  /// a cube subtends.
  // UNFORTUNATELY THIS MEMBER GIVES THE HEMICUBE
  // DEPENDENCE ON EIGEN/increases compile time.
  // But that's what is.
  // Actually there's no need to use an Eigen matrix here.
  // You can just use an array of float.
  Eigen::MatrixXf formFactorCosines[ 2 ] ;
  ////vector< real > formFactorCosines[ 2 ] ;

  // 1d array but will be indexed by (row,col) as 
  // (row*pixelsPerSide + col).
  // Going to store the entire thing (cache it)
  // even though only the top quarter of the front face need
  // store (rotate about NORMAL), and HALF one side side
  // need store (if TOP side, reflect about TOP vector, and
  // rotate about NORMAL).
  //////vector<Vector> formFactorDirectionVectors[ 5 ] ;
  
  //!!DEBUG.  Used to display little cubes
  // where all the hemicubes
  // in the scene have been placed by
  // the radiosity algorithm.
  vector<VertexC> verts ;
  TVertexBuffer<VertexC> *vb ;

  // sidelength here really just
  // has to do with hemicube visualization,
  // not it's performance/computation.
  Hemicube( D3D11Window* iWin, real iSideLen, int iPixelsPerSide ) ;

  // never got used
  ////Vector& formFactorDirectionVector( int face, int row, int col ) ;

private:
  /// Pre-computes form factor cosines
  /// for a hemicube of (pixelsPerSide)
  /// Called once on hemicube construction
  void computeFormFactorCosines() ;

  /// Positions the hemicube over the
  /// patch desired
  /// A Hemicube is positioned and oriented around
  /// a polygonal patch.  To conserve resources
  /// there is only 1 hemicube in the program at a time
  /// (as it has 5 surfaces!)
  /// This function is used internally by 
  /// the "positionOverPatch" function.

  /// Accepts a surface side to render to, and
  /// a viewport to use.  Because when you change
  /// the target surface the viewport is automatically
  /// adjusted, we need to readjust it each time the
  /// target surface is changed.  
  void renderToSurface( HemiSides surfaceSide, Scene *scene, const Vector& eye, const Vector& lookDir, const Vector &upDir ) ;

public:
  // Actually renders to surfaces.
  // offset should be AT LEAST 1e-9, for radiosity patch renders, if not more
  // for per-vertex renders
  void renderToSurfaces( Scene *scene, const Vector& eye, const Vector& forward, const Vector& up, real offset ) ;

  // calculates form factors for the
  // entire scene by placing the hemicube
  // on each patch, then rasterizing the
  // environment onto each of its faces
  //
  // after this is done each of the surfaces
  // is run through and for factors are
  // tabulated between (patch hemicube is on)
  // and all other patches in the scene
  /// Trigger a render to the hemicube face surfaces,
  /// and write the form factors discovered into the ffs matrix.
  void formFactors( Scene *scene, const Triangle& tri, Eigen::MatrixXf* FFs ) ;

  /// there are problems with AO from hemicube,
  /// because of depth buffer problems
  /// try fullcube
  /// implements ambient occlusion finding via hemicubes.
  //VizFunc* ambientOcclusion() ;



  /// USED BY VO (RADIOSITY)
  /// For a POSITION, look direction, up vector,
  /// finds the relationship between your vantage point
  /// and the patches you see.
  /// So now you can tell the type of interreflection
  /// you should get.
  /// This is used for a type of "per-vertex" radiosity.
  /// Returns 2 collections (by parameter):
  ///   results: integer ids of patches you see
  ///            WEIGHTS or % of view taken up by those patches
  void voFormFactors( Scene *scene, const Vector& eye, const Vector& forward, const Vector& up,
       map<int, real> &patchesSeenAndTheirFFs ) ; // where to store the indices of touched triangles, form factors

  

  /// Gets you the area of a side
  real getArea( HemiSides surfaceSide ) ;

  // save the hemicube's surfaces to files so you can see them
  void saveSurfacesToFiles() ;

  


} ;

#endif

