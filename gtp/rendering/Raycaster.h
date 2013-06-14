#ifndef RAYCASTER_H
#define RAYCASTER_H

#include "../math/Vector.h"
#include "../math/EigenUtil.h"
#include "../geometry/Triangle.h"
#include "RayCollection.h"

struct RayCollection ;
class Scene ;
class SHVector ;
struct CubeMap ;
struct Texcoording ;

// Casts rays and feels out surfaces.
// Used both in SH precomputation and
// radiosity (if raytraced).
struct Raycaster
{
  static RayCollection* rc ;
  
  // the current number of rays to use
  int numRaysToUse ;

  real solidAngleRay, dotScaleFactor ;

  vector<Triangle*> *tris ;
  // ^^ These are only used in form factor determination,
  // but not in ambient occlusion.

  //
  D3D11Surface* shTex ;

  Raycaster( int iNumRays );

  void setNumRays( int iNumRays ) ;

  // Queues ambient occlusion
  void QAO( ParallelBatch* pb1, Scene *scene, int vertsPerJob ) ;

  // Queues the sh computation
  void QSHPrecomputation( ParallelBatch* pb1, ParallelBatch* pb2, Scene *scene, int vertsPerJob ) ;

  // Used after rendering cubemaps, because to interreflect we must cast rays
  void QSHPrecomputation_Stage2_Interreflection_Only( ParallelBatch* pb, Scene *scene, int vertsPerJob ) ;

  void QWavelet_Stage2_Interreflection_Only( ParallelBatch* pb, Scene *scene, int vertsPerJob ) ;

  void createSHTexcoords( Scene * scene ) ;

private:

  // just finds the ambient occlusion at a vertex by raycasting, nothing more
  void ambientOcclusion( vector<AllVertex>* verts, int startIndex, int endIndex ) ;

  void ambientOcclusion( AllVertex& vertex ) ;

  void vectorOccluders( vector<AllVertex>* verts, int startIndex, int endIndex ) ;

  void vectorOccluders( AllVertex& vertex ) ;

  // I must be given the array of vertices (which resides in a SHAPE, in a MESHGROUP, in a MESH.)
  void shPrecomputation_Stage1_Ambient( vector<AllVertex>* verts, int startIndex, int endIndex ) ;

  void shPrecomputation_Stage1_Ambient( AllVertex& vertex ) ; // Meat 1

  void shPrecomputation_Stage2_Interreflection( vector<AllVertex>* verts, int startIndex, int endIndex ) ;
  
  void shPrecomputation_Stage2_Interreflection( AllVertex& vertex ) ; // Meat 2

  void writeSHTexcoords( Texcoording & ctex, AllVertex& vertex ) ;

  void wavelet_Stage2_Interreflection( vector<AllVertex>* verts, int startIndex, int endIndex ) ;
  
  void wavelet_Stage2_Interreflection( AllVertex& vertex ) ;



public:
  // This is the function that RadiosityCore calls.
  // Find the form factors in the scene, and put the result in FFs.
  // scene: the scene
  // tris: list of tris
  // ffs: the form factors
  void QFormFactors( ParallelBatch* pb, Scene *scene, vector<Triangle*>* iTris, int trisPerJob, Eigen::MatrixXf* FFs ) ;

  void QVectorOccluders( ParallelBatch* pb, Scene *scene, int vertsPerJob ) ;

private:

  // So, this finds formFactors FOR A SECTION of the tris array.
  // Finding formFactors for only a section of the tris at a time
  // is needed to make the task parallelizable.
  void formFactors( int startIndex, int endIndex, Eigen::MatrixXf* FFs ) ;

  // Finds the form factors for a single tri. meat.
  void formFactors( int indexOfTri, Eigen::MatrixXf* FFs ) ;


public:
  // void sphericalWavelet();

} ;


#endif