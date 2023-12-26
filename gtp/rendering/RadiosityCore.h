#ifndef RADIOSITYCORE_H
#define RADIOSITYCORE_H

#include "VectorOccludersData.h"
#include "../math/EigenUtil.h"  //this file needs eigen.
#include <D3D11.h>

class D3D11Window ;
struct D3D11Surface ;
struct VizFunc ;
struct Triangle ;
class Scene ;
struct Hemicube ;
struct Shape ;
struct AllVertex ;
struct Texcoording ;

/// Enumeration of available
/// solver methods.
enum MatrixSolverMethod
{
  LUDecomposition = 0,
  AInverse        = 1,

  /// Iterative techniques below
  Jacobi          = 2,
  GaussSeidel     = 3,
  Southwell       = 4
} ;

/// RadCore can either rasterize the solution,
/// or raytrace it.
enum FormFactorDetermination
{
  FormFactorsHemicube,
  ////RasterizedTetrahedron, // I don't even intend to provide this, hemicubes are fast enough
  FormFactorsRaycast
} ;

enum VisibilityDetermination
{
  VizFullcube,
  VizRaycast
} ;

extern
char* ColorChannelIndexName[] ;

extern
char* SolutionMethodName[] ;

class RadiosityCore
{
public:
  /// The solution method to use
  /// for the radiosity solution
  /// The only reason I persist these is
  /// so that you can have them shown on the UI.
  int matrixSolverMethod ;
  int formFactorDetType ;

  // Vectors = NxN
  // 1024 means 1M vectors,  2048 means 4M vectors,
  // 4096 means 16M vectors, 8192 means 67M vectors
  int VOTextureSize ;

  // If you're using a hemicube in a radiosity solution,
  // use this many hemi px per side
  int hemicubePixelsPerSide ;

  /// Acceptable tolerance to solve
  /// for iterative solvers before
  /// stopping iteration.
  /// The tolerance is the sum of the
  /// norm squared of the residue vector
  real iterativeTolerance ;

  D3D11Window* win ;


  D3D11Surface* vectorOccludersTex ;// textures can be MUCH bigger than cbuffers.


  #pragma region matrices
  /// There is only ONE form factor matrix
  /// (and not 3) because it has strictly
  /// to do with geometry and not material
  /// properties
  Eigen::MatrixXf* FFs ;

  /// light emission of patches,
  /// initial scene
  /// The values in this vector
  /// are pulled out from scene
  /// geometry, any of the DisplayTriangles
  /// that have an initial radiosity value.
  //Eigen::VectorXf* exitance[ 3 ] ; // one for each of the rgb channels

  /// This is the final exitance vector 
  /// which solves the matrix A,
  /// to give the exitance values.
  //Eigen::VectorXf* fEx[ 3 ] ; // final exitances, one for each of the color channels
  #pragma endregion

private:
  // size of the matrix.  using LONG LONG is just showing off, really.
  // you can't solve 13,000x13,000 without running out of memory.
  // with 8gb ram, max is about 45k tris
  // sqrt( ( 8 * 2^30 ) / 4 )
  // 16gb it's 65k tris
  long long NUMPATCHES ;
  long long FFsSizeInBytes ;

  // I keep this array of pointers for convenience and
  // simpler iteration thru all scene tris. It is also used
  // to quickly get tris BY ID (which isn't possible by looking into SCENE directly).
  vector<Triangle*> tris ;

  // The current radiosity patch Id, about to be given to
  // the next triangle that needs to be assigned an id.
  static int currentId ;

public:
  /// Creates the radiosity core.
  RadiosityCore( D3D11Window* iWin, int iHemicubePixelsPerSide ) ;

  int getNextId() ;

  Shape* getShapeOwner( int id ) ;

  // assign ids to every patch in the scene
  void assignIds( Scene * scene ) ;

  // boots the solver: compute form factors and solve radiosity
  void scheduleRadiosity( Scene* scene, FormFactorDetermination iFormFactorDetType, MatrixSolverMethod iMatrixSolverMethod,
    int numShots, real angleIncr ) ;
  void radiosity( Scene* scene, FormFactorDetermination iFormFactorDetType, MatrixSolverMethod iMatrixSolverMethod ) ;
  void calculateFormFactorsHemicubes( Scene* scene ) ;
  void calculateFormFactorsRaytraced( Scene* scene ) ;

  // 1024 means 1M vectors, 2048 means 4M vectors, 4096 means 16M vectors, 8192 means 67M vectors
  Texcoording createVectorOccludersTex( int size ) ;

  // park all scene vertices' data into voData
  void smoothVectorOccludersData( Scene* scene ) ;
  void parkAll( Scene* scene ) ;
  void park( Texcoording& ctex, AllVertex& vertex ) ;
  void drawDebug( AllVertex& vertex ) ;
  void vectorOccluders( Scene* scene, FormFactorDetermination iFormFactorDetType ) ;
  void calculateVectorOccludersHemicubes( Scene * scene ) ;
  void qVectorOccludersRaytraced( Scene * scene ) ;

private:
  void setAveragePatchColor( int channel, AllVertex* vertex, Eigen::VectorXf* fEx, Triangle *patchOwner ) ;

  void updatePatchRadiosities( int channel, Eigen::VectorXf* fEx ) ;

  void checkFFs() ;

  void solve() ;

  void dropFFsToDisk() ;

  void recoverFFsFromDisk() ;

  /// Solves radiosity of the room
  /// by computing final exitance vector
  /// given the matrix of form factors
  /// and the individual surface reflectivities.
  void solveRadiosity( int channel ) ;

} ;

#endif