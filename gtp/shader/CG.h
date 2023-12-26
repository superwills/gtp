#if 0
#ifndef CG_H
#define CG_H

#include <stdio.h>
#include "../util/StdWilUtil.h"

#include <Cg/cg.h>
#pragma comment(lib, "cg.lib")





#ifdef d3d11
  
#include <Cg/cgD3D11.h>

#pragma comment(lib, "cg.lib")
#pragma comment(lib, "cgD3D11.lib")

class CGD3D11
{
  friend class VertexShaderD3D11 ; // these may touch CG's privates
  friend class PixelShaderD3D11 ;

  // CG
  static CGcontext cgContext ;      // basically handle to cg engine
  static CGprofile vertexProfile ;
  static const char **vertexProfileOpts;

  static CGprofile pxProfile ;
  static const char **pxProfileOpts;

  static bool isInit ;

public:
  static bool init( ID3D11Device* gpu )
  {
    // Now init the shader
    cgContext = cgCreateContext();
    if( NULL==getLastError("creating context") )
      isInit = true ;
  
    cgSetParameterSettingMode( cgContext, CG_DEFERRED_PARAMETER_SETTING ) ;

    // You have to tell cg what gpu you are using
    cgD3D11SetDevice( cgContext, gpu ); // give cg the gpu device
  
    // a profile specifies the output language of the shader,
    vertexProfile = cgD3D11GetLatestVertexProfile();
    getLastError("getting latest vertex profile");

    // Note that you cannot go above vs_3_0 if you are on d3d9,
    printf( "*** You got vertex profile '%s' ***\n", cgGetProfileString( vertexProfile ) ) ;

    // Get vertex profile opts
    vertexProfileOpts = cgD3D11GetOptimalOptions( vertexProfile ) ;
    getLastError("getting latest profile options");
    
    pxProfile = cgD3D11GetLatestPixelProfile();
    getLastError("getting latest pixel profile");

    // Note that you cannot go above ps_3_0 if you are on d3d9,
    printf( "*** You got pixel profile '%s' ***\n", cgGetProfileString( pxProfile ) ) ;

    pxProfileOpts = cgD3D11GetOptimalOptions( pxProfile );
    getLastError("getting latest fragment profile options");

    return true ;
  }

  static bool getLastError( const char* message )
  {
    CGerror cgerror;
    const char *errString = cgGetLastErrorString(&cgerror);
  
    if(cgerror != CG_NO_ERROR)
    {
      if( cgerror == CG_COMPILER_ERROR )
        printf( "Error '%s'\n%s\n The compiler says: %s", message, errString, cgGetLastListing( cgContext ) ) ;
      else //runtime error
        printf( "Error '%s'\n%s\n", message, errString ) ;

      return true ;
    }
    else
      return false ; // no error
  }

  static void shutdown()
  {
    // You have to unset the device, or your cg program will crash on exit.
    cgD3D11SetDevice( cgContext, NULL ) ;
    cgDestroyContext( cgContext );
  }
} ;

// 1.  compile
// 2.  load
// 3.  bind
// 4.  draw

// You should extend this class with specific shaders.
class VertexShaderD3D11
{
protected:
  CGprogram vs ; // the Vertex shader object itself. need this
  // to getNamedParameter.

public:
  ID3D11InputLayout *layout ;

  ~VertexShaderD3D11()
  {
    destroy() ;
  }

  // 1 compile (on construction)
  bool compile( char* file, char* mainFcnName )
  {
    if( !CGD3D11::isInit )
    {
      puts( "CG not initialized" ) ;
      return false ;
    }

    info( Yellow, "Compiling VertexShader '%s'", file ) ;

    // If you saved your CG program in a separate file
    // (called vShader.cg), then you could load it
    // this way
    vs = cgCreateProgramFromFile(
        CGD3D11::cgContext,      // Cg runtime context
        CG_SOURCE,          // Program in human-readable form
        file,       // Name of file containing program
        CGD3D11::vertexProfile,
        mainFcnName,            // Entry function name
        CGD3D11::vertexProfileOpts ) ;     // Pass optimal compiler options
    
    return !CGD3D11::getLastError("creating vertex program from file");
  }

  // 2 load
  bool load()
  {
    ///////////////////
    // LOAD the program
    cgD3D11LoadProgram(vs, 0);
    return !CGD3D11::getLastError("loading vertex program");
  }

  // 3 bind
  bool bind()
  {
    cgD3D11BindProgram( vs );
    return !CGD3D11::getLastError("binding vertex program");
  }

  bool unbind()
  {
    cgD3D11UnbindProgram( vs ) ;
    return !CGD3D11::getLastError("unbinding vertex program");
  }

  ID3DBlob * getBlob()
  {
    ID3DBlob* blob = cgD3D11GetCompiledProgram( vs );
    if( !blob )
      error( "Could not get compiled program, musta failed" ) ;
    return blob ;
  }

  bool destroy()
  {
    cgDestroyProgram( vs );
    return !CGD3D11::getLastError("destroying vertex program");
  }

  virtual void getParams() PURE ;
};

class PixelShaderD3D11
{
protected:
  CGprogram ps ; // the pixel shader

public:
  ID3D11InputLayout *layout ;

  ~PixelShaderD3D11()
  {
    destroy() ;
  }

  bool compile( char* file, char* mainFcnName )
  {
    if( !CGD3D11::isInit )
    {
      puts( "CG not initialized" ) ;
      return false ;
    }

    info( Yellow, "Compiling PixelShader '%s'", file ) ;

    // If you saved your CG program in a separate file
    // (called vShader.cg), then you could load it
    // this way
    ps = cgCreateProgramFromFile(
        CGD3D11::cgContext,      // Cg runtime context
        CG_SOURCE,          // Program in human-readable form
        file,       // Name of file containing program
        CGD3D11::pxProfile,
        mainFcnName,            // Entry function name
        CGD3D11::pxProfileOpts ) ;     // Pass optimal compiler options
    
    return !CGD3D11::getLastError("creating pixel program from file");
  }

  // 2 load
  bool load()
  {
    ///////////////////
    // LOAD the programs
    cgD3D11LoadProgram(ps, false);
    return !CGD3D11::getLastError("loading px program");
  }

  // 3 bind
  bool bind()
  {
    cgD3D11BindProgram( ps );
    //!! I'd LIKE TO bind the right vertexlayout here as well,
    // but that requires passing the D3D11Window, which means
    // changing the interface OR requiring a global pointer.
    return !CGD3D11::getLastError("binding px program");
  }
  
  bool unbind()
  {
    cgD3D11UnbindProgram( ps ) ;
    return !CGD3D11::getLastError("unbinding px program");
  }

  bool destroy()
  {
    cgDestroyProgram( ps );
    return !CGD3D11::getLastError("destroying px program");
  }

  void setTexture( CGparameter &tex, ID3D11Texture2D* d3d11tex )
  {
    cgD3D11SetTextureParameter( tex, d3d11tex ) ;

    // I'd like to set a default sampler state
    ///cgD3D11SetSamplerStateParameter( theTexture, ss ) ;
  }

  virtual void getParams() PURE ;
};

#else
  #ifdef d3d10
    #include <Cg/cgD3D10.h>
    #pragma comment(lib, "cgD3D10.lib")


  #else
    // stuck with d3d9
    #include <Cg/cgD3D9.h>
    #pragma comment(lib, "cgD3D9.lib")

class CGD3D9
{
  friend class VertexShaderD3D9 ; // these may touch CG's privates
  friend class PixelShaderD3D9 ;

  // CG
  static CGcontext cgContext ;      // basically handle to cg engine
  static CGprofile vertexProfile ;
  static const char **vertexProfileOpts;

  static CGprofile pxProfile ;
  static const char **pxProfileOpts;

  static bool isInit ;

public:


  static bool init( IDirect3DDevice9* gpu )
  {
    // Now init the shader
    cgContext = cgCreateContext();
    if( NULL==getLastError("creating context") )
      isInit = true ;
  
    //cgSetParameterSettingMode( cgContext, CG_DEFERRED_PARAMETER_SETTING ) ;

    // You have to tell cg what gpu you are using
    cgD3D9SetDevice( gpu ); // give cg the gpu device
  
    // a profile specifies the output language of the shader,
    vertexProfile = cgD3D9GetLatestVertexProfile();
    getLastError("getting latest vertex profile");

    // Note that you cannot go above vs_3_0 if you are on d3d9,
    printf( "*** You got vertex profile '%s' ***\n", cgGetProfileString( vertexProfile ) ) ;

    // Get vertex profile opts
    vertexProfileOpts = cgD3D9GetOptimalOptions( vertexProfile ) ;
    getLastError("getting latest profile options");
    
    pxProfile = cgD3D9GetLatestPixelProfile();
    getLastError("getting latest pixel profile");

    // Note that you cannot go above ps_3_0 if you are on d3d9,
    printf( "*** You got pixel profile '%s' ***\n", cgGetProfileString( pxProfile ) ) ;

    pxProfileOpts = cgD3D9GetOptimalOptions( pxProfile );
    getLastError("getting latest fragment profile options");

    return true ;
  }

  static bool getLastError( const char* message )
  {
    CGerror cgerror;
    const char *errString = cgGetLastErrorString(&cgerror);
  
    if(cgerror != CG_NO_ERROR)
    {
      if( cgerror == CG_COMPILER_ERROR )
        error( "Error '%s'\n%s\n The compiler says: %s", message, errString, cgGetLastListing( cgContext ) ) ;
      else //runtime error
        error( "Error '%s'\n%s\n", message, errString ) ;

      return true ;
    }
    else
      return false ; // no error
  }

  static void shutdown()
  {
    // You have to unset the device, or your cg program will crash on exit.
    cgD3D9SetDevice( NULL ) ;
    cgDestroyContext( cgContext );
  }
} ;

// 1.  compile
// 2.  load
// 3.  bind
// 4.  draw

// You should extend this class with specific shaders.
class VertexShaderD3D9
{
protected:
  CGprogram vs ; // the Vertex shader object itself. need this
  // to getNamedParameter.

public:
  // 1 compile (on construction)
  bool compile( char* file, char* mainFcnName )
  {
    if( !CGD3D9::isInit )
    {
      error( "CG not initialized" ) ;
      return false ;
    }

    info( Yellow, "Compiling VertexShader '%s'", file ) ;
    // If you saved your CG program in a separate file
    // (called vShader.cg), then you could load it
    // this way
    vs = cgCreateProgramFromFile(
        CGD3D9::cgContext,      // Cg runtime context
        CG_SOURCE,          // Program in human-readable form
        file,       // Name of file containing program
        CGD3D9::vertexProfile,
        mainFcnName,            // Entry function name
        CGD3D9::vertexProfileOpts ) ;     // Pass optimal compiler options
    
    return !CGD3D9::getLastError("creating vertex program from file");
  }

  // 2 load
  bool load()
  {
    ///////////////////
    // LOAD the programs
    // The last parameter is the assembly flags.
    // e.g. D3DXSHADER_DEBUG (See http://msdn.microsoft.com/en-us/library/bb205441(VS.85).aspx)
    cgD3D9LoadProgram(vs, false, 0);
    return !CGD3D9::getLastError("loading vertex program");
  }

  // 3 bind
  bool bind()
  {
    cgD3D9BindProgram( vs );
    return !CGD3D9::getLastError("binding vertex program");
  }

  bool unbind()
  {
    cgD3D9UnbindProgram( vs ) ;
    return !CGD3D9::getLastError("unbinding vertex program");
  }

  bool destroy()
  {
    cgDestroyProgram( vs );
    return !CGD3D9::getLastError("destroying vertex program");
  }

  virtual void getParams() PURE ;
};

class PixelShaderD3D9
{
protected:
  CGprogram ps ; // the pixel shader

public:
  bool compile( char* file, char* mainFcnName )
  {
    if( !CGD3D9::isInit )
    {
      puts( "CG not initialized" ) ;
      return false ;
    }

    info( Yellow, "Compiling PxShader '%s'", file ) ;
    // If you saved your CG program in a separate file
    // (called vShader.cg), then you could load it
    // this way
    ps = cgCreateProgramFromFile(
        CGD3D9::cgContext,      // Cg runtime context
        CG_SOURCE,          // Program in human-readable form
        file,       // Name of file containing program
        CGD3D9::pxProfile,
        mainFcnName,            // Entry function name
        CGD3D9::pxProfileOpts ) ;     // Pass optimal compiler options
    
    return !CGD3D9::getLastError("creating pixel program from file");
  }

  // 2 load
  bool load()
  {
    ///////////////////
    // LOAD the programs
    // The last parameter is the assembly flags.
    // e.g. D3DXSHADER_DEBUG (See http://msdn.microsoft.com/en-us/library/bb205441(VS.85).aspx)
    cgD3D9LoadProgram(ps, false, 0);
    return !CGD3D9::getLastError("loading px program");
  }

  // 3 bind
  bool bind()
  {
    cgD3D9BindProgram( ps );
    return !CGD3D9::getLastError("binding px program");
  }
  
  bool unbind()
  {
    cgD3D9UnbindProgram( ps ) ;
    return !CGD3D9::getLastError("unbinding px program");
  }

  bool destroy()
  {
    cgDestroyProgram( ps );
    return !CGD3D9::getLastError("destroying px program");
  }

  void setTexture( CGparameter tex, IDirect3DTexture9* d3d9tex )
  {
    cgD3D9SetTexture( tex, d3d9tex ) ;
  }

  virtual void getParams() PURE ;
};

  #endif
#endif












#endif
#endif