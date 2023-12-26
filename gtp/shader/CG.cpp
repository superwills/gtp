#if 0
#include "CG.h"

#ifdef d3d11
CGcontext CGD3D11::cgContext ;      // basically handle to cg engine
CGprofile CGD3D11::vertexProfile ;
const char **CGD3D11::vertexProfileOpts;

CGprofile CGD3D11::pxProfile ;
const char **CGD3D11::pxProfileOpts;

bool CGD3D11::isInit = false ;

#else
#ifdef d3d10
CGcontext CGD3D10::cgContext ;      // basically handle to cg engine
CGprofile CGD3D10::vertexProfile ;
const char **CGD3D10::vertexProfileOpts;

CGprofile CGD3D10::pxProfile ;
const char **CGD3D10::pxProfileOpts;

bool CGD3D10::isInit = false ;
#else
#ifdef d3d9
CGcontext CGD3D9::cgContext ;      // basically handle to cg engine
CGprofile CGD3D9::vertexProfile ;
const char **CGD3D9::vertexProfileOpts;

CGprofile CGD3D9::pxProfile ;
const char **CGD3D9::pxProfileOpts;

bool CGD3D9::isInit = false ;
#endif
#endif
#endif

#endif