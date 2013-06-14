#if 0
#ifndef SHADERS_H
#define SHADERS_H

#include "../window/D3DWindow.h"
#include "CG.h"

#define GETPARAM(SHADERNAME,PARAMNAME) PARAMNAME=cgGetNamedParameter(SHADERNAME, #PARAMNAME) ; CG::getLastError("Getting "#PARAMNAME" parameter");

class vVNCfShader : public VertexShader
{
public:
  vVNCfShader( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    getParams() ;  
    load() ;
  }

  CGparameter mvp ; // the modelview-projection matrix,
  // which each vertex needs to be multiplied by
  // in the vertex shader

  void getParams()
  {
    ///////////////////
    // GET PARAMS
    // you can get/set the params after you create the programs
    // well, basically anytime after compilation,
    // but obviously BEFORE you try to get/set them using cgSetParameter
    mvp = cgGetNamedParameter( vs, "modelViewProj" ); 
    CG::getLastError("Getting modelViewProj parameter");
  }
} ;

class PxVNCfShader : public PixelShader
{
public:
  CGparameter diffLightPos ;
  CGparameter diffLightColor ;

  CGparameter specLightPos ;
  CGparameter specLightColor ;

  CGparameter eyePos ;

  PxVNCfShader( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    getParams() ;  
    load() ;
  }

  void getParams()
  {
    GETPARAM( ps, diffLightPos ) ;
    GETPARAM( ps, diffLightColor ) ;
    GETPARAM( ps, specLightPos ) ;
    GETPARAM( ps, specLightColor ) ;

    GETPARAM( ps, eyePos ) ;
  }
} ;


class vVCfShader : public VertexShader
{
public:
  CGparameter mvp ;

  vVCfShader( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    getParams() ;  
    load() ;
  }

  void getParams() override
  {
    ///////////////////
    // GET PARAMS
    // you can get/set the params after you create the programs
    // well, basically anytime after compilation,
    // but obviously BEFORE you try to get/set them using cgSetParameter
    mvp = cgGetNamedParameter( vs, "modelViewProj" );
    CG::getLastError("Getting modelViewProj parameter");
  }
} ;

class PxVCfShader : public PixelShader
{
public:
  PxVCfShader( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    getParams() ;  
    load() ;
  }
  void getParams() override {}
} ;


class vVTCfShader : public VertexShader
{
public:
  CGparameter mvp ;

  vVTCfShader( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    getParams() ;  
    load() ;
  }

  void getParams() override
  {
    ///////////////////
    // GET PARAMS
    // you can get/set the params after you create the programs
    // well, basically anytime after compilation,
    // but obviously BEFORE you try to get/set them using cgSetParameter
    mvp = cgGetNamedParameter( vs, "modelViewProj" );
    CG::getLastError("Getting modelViewProj parameter");
  }
} ;

class PxVTCfShader : public PixelShader
{
public:
  CGparameter theTexture ;

  PxVTCfShader( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    getParams() ;  
    load() ;
  }

  void getParams() override
  {
    theTexture = cgGetNamedParameter( ps, "theTexture" );
    CG::getLastError("Getting theTexture parameter");
  }
} ;


class VSurface : public VertexShader
{
public:
  VSurface( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    load() ;
  }
  void getParams() override {}
} ;

class PxSurface : public PixelShader
{
public:
  CGparameter theTexture ;

  PxSurface( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    getParams() ;  
    load() ;
  }

  void getParams() override
  {
    theTexture = cgGetNamedParameter( ps, "theTexture" );
    CG::getLastError("Getting theTexture parameter");
  }
} ;

class vSH : public VertexShader
{
public:
  CGparameter mvp ;

  vSH( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    getParams() ;
    load() ;
  }

  void getParams() override
  {
    mvp = cgGetNamedParameter( vs, "modelViewProj" );
    CG::getLastError("Getting modelViewProj parameter");    
  }
};

class pSH : public PixelShader
{
public:
  pSH( char* file, char* mainFcnName )
  {
    compile( file, mainFcnName ) ;
    load() ;
  }

  void getParams() override { }
};


#endif

#endif