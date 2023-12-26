#ifndef FULLCUBE_H
#define FULLCUBE_H

class D3D11Window ;
struct D3D11Surface ;
class Scene ;

#include "../geometry/CubeMap.h"  // for CubeFace enum
#include "../math/IndexMap.h"

// a class that renders an AXIS ALIGNED, full cube
// like Hemicube, only it _doesn't_ care about form
// factors.

// Where am I going to put
// the rendered cubes?
// Every vertex gets __6__ images.
// I'll use CubeMap*'s at each vertex.
// this can be stored .. guess where .. INSIDE THE VERTICES!!

struct FullCubeRenderer
{
  static RayCollection* rc ;
  
  int pixelsPerSide ;
  int compressionMode ;
  Scene *scene ;

  static real compressionLightmap, compressionVisibilityMaps ;
  static bool useRailedQuincunx ; // whether or not to use railed cubemaps.

  D3D11Surface* surfaces[ 6 ] ;

  // the visitation order for the wavelet transform
  static IndexMap indexMap ;

  // attach me to a scene now, so you
  // don't have to keep passing one in
  // (it makes no sense to change the scene
  // midway thru)
  FullCubeRenderer( Scene* iScene, int pxPerSide ) ;

  // Taken out
  //void pregenFFCos() ;

  // You should use the pregenned ones in the RayCollection
  real ffCos( int pxPerSide, int row, int col ) ;

  void initSurfaces() ;

  void renderSetup() ;

  void renderAt( const Vector& eye ) ;

  // renders
  void renderCubesAtEachVertex( int iCompressionMode ) ;

  // Compresses a range of vertices in the array.
  ////void compressVizInfo( vector<AllVertex>* verts, int startIndex, int endIndex ) ; // not using this style.
  
  // Actually compresses a single vertex
  void compressVizInfo( AllVertex *vertex ) ;

  // Traps a thread until you set FINISHED_SUBMITTING_JOBS to true
  bool FINISHED_SUBMITTING_JOBS ;
  int threadsCreated ;
  vector< AllVertex* > readyVerts ;
  // Greedy (cpu busy-waiting) thread.
  void greedyCompressor() ;

  void light() ;


};


#endif