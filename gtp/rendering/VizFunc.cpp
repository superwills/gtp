#include "VizFunc.h"
#include "RayCollection.h"
#include "../geometry/Ray.h"
#include "../geometry/Intersection.h"
#include "../geometry/Triangle.h"
#include "../scene/Scene.h"
#include "../threading/ParallelizableBatch.h"
#include "../window/GTPWindow.h"
#include "FullCubeRenderer.h"
#include "../math/SHSample.h"

RayCollection* VizFunc::rc ;

VizFunc::VizFunc()
{
  rayIndices = new vector<int>;
  rayWeights = new vector<real>;
}

void VizFunc::cleanup()
{
  DESTROY( rayIndices ) ;
  DESTROY( rayWeights ) ;
}

VizFunc::~VizFunc()
{
  cleanup() ;
}

SHVector* VizFunc::toSH( int bands )
{
  // projection to SH.
  return 0 ;
}

CubeMap* VizFunc::toCubeMap( int pxPerSide )
{
 return 0;
}






