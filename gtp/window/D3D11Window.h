#ifndef D3D11WINDOW_H
#define D3D11WINDOW_H

#include <D3D11.h>
#include <D3DX11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d10_1.lib")

#ifdef _DEBUG
#pragma comment(lib,"d3dx11d.lib")
#else
#pragma comment(lib,"d3dx11.lib")
#endif

#include "D3D11Surface.h" // your concrete implementation of a surface
#include "D3D11VB.h"

#include "D3DDrawingVertex.h"

#include "IGraphicsWindow.h"
#include "D3D11Shader.h"

#include "../math/Vector.h"
#include "D3D11VB.h"
#include "DirectWrite.h"

#include <DxErr.h>
#pragma comment(lib, "dxerr.lib")
#pragma comment(lib, "dxgi.lib")
bool DX_CHECK( HRESULT hr, char * msg ) ;

// these can be placed in an array
struct GPUCDATA0_Lights //register0
{
  int activeLights[ 4 ] ; // which light are active? ambient, diffuse, specular,unused
  float ambientLightColor[32] [4];  // 8 floats of 4

  // TECHNICALLY a diffuse light ONLY
  // has a position, never a direction.
  // however, we can save computations
  // if a diffuse light is really far away
  // then all points in the scene will 
  // compute the SAME direction vector towards it.
  float diffuseLightPos[32] [4] ;  // if a diffuse light is POINT,
  // it has a POS and no dir (dirs are EVERYWHERE around point)
  // If a diffuse light is DIR, (really far away) then it has
  // dir and no POS)
  // Use OpenGL convention here: IF w==0, then
  // LIGHT SOURCE IS DIRECTIONAL, AND (x,y,z) DESCRIBE
  // IT'S DIRECTION.
  float diffuseLightColor[32] [4] ;

  float specularLightPos[32][4] ;
  float specularLightDir[32][4];
  float specularLightColor[32][4] ;
  
  float eyePos[4] ;
  float expos[4] ;
  float vo[4] ;
  float voI[4] ;
} ;

struct GPUCDATA1_Modelview //register1
{
  // the concatenated modelview-projection matrix
  float modelViewProj[16] ;  // column major
} ;

struct GPUCDATA2_World
{
  // the concatenated modelview-projection matrix
  float world[16] ;  // column major, world matrix
} ;

struct GPUCDATA3_TextureRotation
{
  float texRot[16] ;
} ;

struct GPUCDATA4_SHCoeffs
{
  float coeffs[10*10][4] ;
  float shOptions[4] ;
} ;

struct GPUCDATA5_Waves
{
  float timePhaseFreqAmp[ 50 ][ 4 ] ; // these 4 packed into a float4
  float longAxis[ 50 ][ 4 ] ;
  float transverseAxis[ 50 ][ 4 ] ;
} ;

extern char* TextureSamplingModeName[] ;

class D3D11Window : public IGraphicsWindow
{
  // my friends can touch my privates
  friend struct D3D11Surface ;
  friend struct D3D11Texture ;
  friend struct CubeMap ;
  friend class D3D11Shader ;
  
  template <typename T> friend class TVertexBuffer ;

protected:
  ID3D11Device *d3d ;

public:
  ID3D11DeviceContext *gpu ;
  
  GPUCDATA0_Lights gpuCData0 ;
  GPUCDATA1_Modelview gpuCData1 ;
  GPUCDATA2_World gpuCData2 ;
  GPUCDATA3_TextureRotation gpuCData3 ;
  GPUCDATA4_SHCoeffs gpuCData4 ;
  GPUCDATA5_Waves gpuCData5 ;
  vector<ID3D11Buffer*> gpuCDataBuffer ;

  IDXGISwapChain * swapChain ;
  ID3D11Texture2D *backbufferSCTex; //global for screenshotting
  ID3D11RenderTargetView * renderTargetView ;
  ID3D11DepthStencilView * depthStencilView ;
  
  // We only keep ONE blendstate and ONE rasterizer state.
  // to change them, retrieve the entire thing, change the setting you like,
  // and set it back.  it is NOT worth keeping the explosion of different
  // BlendState and RasterizerState objects and tracking it all.
  ID3D11BlendState *blendState ;
  ID3D11RasterizerState *rasterizerState ;
  D3D11_VIEWPORT viewport; // I keep this one for the width of the window

  // d2d related
  IDXGIAdapter1 *dxgiAdapter ;

  ID3D11ShaderResourceView* NO_RESOURCE[1] ; // this is the array you use when you want to UNBIND a resource

  // shared tex stuff
  //D3D11Surface *sharedTexSurface ; // not used b/c i need to create it bgra
  ID3D11Texture2D *sharedTex ;
  ID3D11ShaderResourceView* sharedTexResView ;
  ID3D11SamplerState *ssLinear, *ssPoint, *ssAnisotropic ;
  IDXGIKeyedMutex *dxgiKeyedMutex11 ;
  DirectWrite *directWrite ;
  TVertexBuffer<VertexTC> * d2dQuad ;

  //// SHADERS
  // difference in INPUT LAYOUT ie how many vertex coords and tex coords there are
  VCShader *shaderVcDebug ; // for drawing debug stuff, which only has VC
  VNCShader *shaderVncPlain ; // for showing visualizations such as light sources
  VTCShader *shaderVtcSurfaces ; // for drawing surfaces, which only has VTC

  // the rest use the general VTTTTNCCCC vertex format, actually
  VT10NC10Shader *shaderSHLitCPU, *shaderSHLitGPU, *shaderPhongLit, *shaderPhongLitVertex, *shaderPlain, *shaderCubemap,
    *shaderID, *shaderRadiosity, *shaderVectorOccluders, *shaderWavelet,
    *shaderDecal ;
  D3D11Shader *activeShader ;
 
  D3D11Window( HINSTANCE hInst, TCHAR* windowTitleBar, int windowXPos, int windowYPos, int windowWidth, int windowHeight ) ;
  ~D3D11Window() ;

  void activate() ;
  void deactivate() ;
  bool setSize( int width, int height, bool fullScreen ) override ;
  bool fullScreenInMaxResolution() override ; // go fullscreen, in max resolution
  int getWidth() override ;
  int getHeight() override ;

  void setModelviewProjection( const Matrix& mvp ) ;
  void setWorld( const Matrix& mat ) ;

  // just draws all the shapes in the scene
  void render( Scene * scene ) override ;
  void bindScreenRenderTarget() override ;
  void step() override ;

  bool init( int width, int height ) override ;
  void updateGPUCData( int bufferNumber ) ;
  void screenshot() override ;
  void screenshot( char* name ) override ;
  
  void clear( ByteColor toColor ) override ;

  void alphaBlend( bool on ) override ;
  void depthWrite( bool on ) ;

  void setBackgroundColor( ByteColor color ) override ;
  void toggleFillMode() override ;
  void setFillMode( FillMode iFillMode ) override ;
  void cullMode( CullMode cullThese ) override ;

  // assuming you only want to sample in one way
  // at a time (you can bind multiple SamplerState
  // objects in hlsl but we only use slot 0).
  void setTextureSamplingMode( TextureSamplingMode texSampMode ) override ;
  
  // A general undo( statename ) would be helpful
  void revertPointSize() override ;
  void setPointSize( float newPtSize ) override ;

  bool createFont( int fontId, int height, int boldness, char* fontName ) override ;
  bool loadTexture( int textureId, char* filename, ByteColor backgroundColor ) override ;
  
  void drawText( int fontId,
    const char *str, ByteColor color,
    float x, float y, float boxWidth, float boxHeight,
    DWORD formatOptions ) override ;

  bool beginText() override ;
  
  bool endText() override ;

  bool beginDrawing() override ; // Call once per frame before drawing.
  // Returns TRUE when beginDrawing succeeded.
  // Don't try and draw if this function fails (it means the D3D device
  // is temporarily "lost" (i.e. some other app has control of it))

  bool endDrawing() override ;   // "flips the buffer" over so the user can see
  // what we just drew this frame

  bool present() override ;

} ;

#endif
