#include "D3D11Window.h"
#include "../scene/scene.h"
#include "../util/StdWilUtil.h"
#include "../geometry/Shape.h"
#include "../geometry/Mesh.h"
#include "../Globals.h"

char* TextureSamplingModeName[] = {
  "Point",
  "Linear",
  "Anisotropic"
} ;

// Fixes error calling DXGetErrorDescriptionA() below:
// https://stackoverflow.com/questions/31053670/unresolved-external-symbol-vsnprintf-in-dxerr-lib
int ( WINAPIV * __vsnprintf )( char *, size_t, const char*, va_list ) = _vsnprintf;

bool DX_CHECK( HRESULT hr, char * msg )
{
  if( FAILED( hr ) )
  {
    // Weird error:
    // 1>dxerr.lib( dxerra.obj ) : error LNK2001: unresolved external symbol _vsnprintf
    // 1>..gtp.exe : fatal error LNK1120: 1 unresolved externals
    error( "%s. %s:  %s", msg, DXGetErrorStringA( hr ), DXGetErrorDescriptionA( hr ) ) ;
    return false ;
  }

  else
    return true ;
}

D3D11Window::D3D11Window( HINSTANCE hInst, TCHAR* windowTitleBar,
                     int windowXPos, int windowYPos,
                     int windowWidth, int windowHeight ) :
  IGraphicsWindow( hInst, windowTitleBar,
          windowXPos, windowYPos,
          windowWidth, windowHeight )
{
  d3d=0,gpu=0,
  
  swapChain=0,backbufferSCTex=0,renderTargetView=0,depthStencilView=0;

  shaderVcDebug = 0 ;
  shaderVtcSurfaces = 0 ;
  shaderSHLitCPU = 0 ;
  shaderSHLitGPU = 0 ;
  shaderPhongLit = 0 ;
  shaderVncPlain = 0 ;
  activeShader = 0 ;

  blendState=0;
  
  rasterizerState=0;
  
  dxgiAdapter=0,ssPoint=0,ssLinear=0,ssAnisotropic=0,dxgiKeyedMutex11=0,directWrite=0,d2dQuad=0;

  //sharedTexSurface=0;
  NO_RESOURCE[0]=0;

  

  info( "Starting up Direct3D..." ) ;
  if( !init( windowWidth, windowHeight ) )
  {
    info( "D3D11 failed to initialize." ) ;
  }

  // init directwrite
  directWrite = new DirectWrite( dxgiAdapter, sharedTex ) ;
  static const VertexTC quadVerts[4] = 
  {
    VertexTC( Vector(-1,-1),  Vector( 0,1 ),  Vector(1,1,1,1) ), // z-values selected put it right up in the users face
    VertexTC( Vector( 1,-1),  Vector( 1,1 ),  Vector(1,1,1,1) ),
    VertexTC( Vector(-1, 1),  Vector( 0,0 ),  Vector(1,1,1,1) ),
    VertexTC( Vector( 1, 1),  Vector( 1,0 ),  Vector(1,1,1,1) )
  };
  d2dQuad = new TVertexBuffer<VertexTC>( this, quadVerts, 4 ) ;

  deviceDirty = false ;
}

D3D11Window::~D3D11Window()
{
  info( "D3D11Window destroy" ) ;

  SAFE_RELEASE( swapChain ) ;
  SAFE_RELEASE( backbufferSCTex ) ;
  SAFE_RELEASE( renderTargetView ) ;
  SAFE_RELEASE( depthStencilView ) ;

  DESTROY( shaderVcDebug ) ;
  DESTROY( shaderVtcSurfaces ) ;
  DESTROY( shaderSHLitCPU ) ;
  DESTROY( shaderSHLitGPU ) ;
  DESTROY( shaderPhongLit ) ;
  DESTROY( shaderVncPlain ) ;
  
  SAFE_RELEASE( blendState ) ;
  SAFE_RELEASE( rasterizerState ) ;

  SAFE_RELEASE( dxgiAdapter ) ;
  
  SAFE_RELEASE( sharedTex );
  SAFE_RELEASE( ssPoint );
  SAFE_RELEASE( ssLinear );
  SAFE_RELEASE( ssAnisotropic );
  SAFE_RELEASE( dxgiKeyedMutex11 );

  DESTROY( directWrite ) ;
  DESTROY( d2dQuad ) ;

  if( gpu )  gpu->ClearState();  // you have to do this or it hangs
  SAFE_RELEASE( gpu );
  SAFE_RELEASE( d3d );
  
}

void D3D11Window::activate()
{
  //info( "d3d11 activate" ) ;
  Window::activate() ;
}
void D3D11Window::deactivate()
{
  //info( "d3d11 deactivate" ) ;
  Window::deactivate() ;
}

bool D3D11Window::setSize( int width, int height, bool fullScreen )
{
  error( "Not implemented" ) ;
  return 0 ;
}
bool D3D11Window::fullScreenInMaxResolution()
{
  error( "Not implemented" ) ;
  // go fullscreen, in max resolution
  return 0 ;
}
int D3D11Window::getWidth() 
{
  return viewport.Width ;
}
int D3D11Window::getHeight()
{
  return viewport.Height ;
}

void D3D11Window::setModelviewProjection( const Matrix& mvp )
{
  mvp.writeFloat16RowMajor( gpuCData1.modelViewProj ) ;
  updateGPUCData( 1 ) ;
}

void D3D11Window::setWorld( const Matrix& mat )
{
  mat.writeFloat16RowMajor( gpuCData2.world ) ;
  updateGPUCData( 2 ) ;
}

void D3D11Window::render( Scene * scene )
{
  MutexLock LOCK( scene->mutexShapeList, INFINITE );
  
  //info( "%d shapes", scene->shapes.size() ) ;
  for( int i = 0 ; i < scene->shapes.size() ; i++ )
    scene->shapes[ i ]->draw() ;
}

void D3D11Window::bindScreenRenderTarget()
{
  gpu->OMSetRenderTargets( 1, &renderTargetView, depthStencilView ) ;
}

void D3D11Window::step()
{
}

bool D3D11Window::init( int width, int height )
{
  // Now we must create and describe the swapchain.
  // Swapchains are explained here: http://msdn.microsoft.com/en-us/library/bb206356(VS.85).aspx

  // FOLLOWS: How To: Create a device and immediate context: http://msdn.microsoft.com/en-us/library/ff476879(VS.85).aspx
  // There are 4 steps to this process:

  // 1.  MSDN:  "Fill out the DXGI_SWAP_CHAIN_DESC structure
  //             with information about buffer formats and dimensions."
  DXGI_SWAP_CHAIN_DESC swapChainDesc ;  // DXGI_SWAP_CHAIN_DESC: http://msdn.microsoft.com/en-us/library/bb173075(VS.85).aspx
  ZeroMemory( &swapChainDesc, sizeof( swapChainDesc ) );

  // So now we create the SwapChainDesc, then
  // we'll instantiate a SwapChain based on it. In a previous life
  // (Direct3D9), this was D3DPRESENT_PARAMETERS.
  swapChainDesc.BufferCount        = 1;
  swapChainDesc.BufferDesc.Width   = width;
  swapChainDesc.BufferDesc.Height  = height;
  swapChainDesc.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM; // lucky we chose R8G8B8A8. You cannot save a screenshot using D3DX11 lib
  // if you use bgra.
  
  // These are ignored for a backbuffer:
  //swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
  //swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

  swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT ;
  swapChainDesc.OutputWindow       = hwnd;
  swapChainDesc.SampleDesc.Count   = 1;
  swapChainDesc.SampleDesc.Quality = 0 ;
  swapChainDesc.Windowed           = TRUE;
  

  // Get the adapter explicitly here, so we don't have to retrieve it out
  // of the d3d11 interface later when creating d3d10.1/d2d (must be sure to use
  // the same dxgi adapter)
  IDXGIFactory1 *dxgiFactory ;
  DX_CHECK( CreateDXGIFactory1( __uuidof( IDXGIFactory1 ), (void**)&dxgiFactory ), "Creating dxgifactory" ) ;

  // So you probably only have 1 "DISPLAY ADAPTER"/gpu.
  // If you have more than one, you can choose which to use,
  // but the default choice is to use the first one enumerated
  
  // Get the first available adapter
  if( dxgiFactory->EnumAdapters( 0, (IDXGIAdapter**)&dxgiAdapter ) == DXGI_ERROR_NOT_FOUND )
  {
    FatalAppExitA( 0, "You don't have a display adapter" ) ;
  }
  SAFE_RELEASE( dxgiFactory ) ;

  // Get some information about the gpu.
  DXGI_ADAPTER_DESC1 desc = {0};
  dxgiAdapter->GetDesc1( &desc ) ;
  


  D3D_FEATURE_LEVEL featureLevelYouGot ;

  // 2. MSDN: "Using the DXGI_SWAP_CHAIN_DESC structure from step one,
  //           call D3D11CreateDeviceAndSwapChain to initialize the device
  //           and swap chain at the same time."
  // Try and make a hardware device & swapchain
  bool ok = DX_CHECK( D3D11CreateDeviceAndSwapChain(
    dxgiAdapter,
    D3D_DRIVER_TYPE_UNKNOWN,
    NULL,
    D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_BGRA_SUPPORT  // bgra is SUPPORTED, doesn't mean i have to USE IT for everything
    | D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG
    ,
    NULL,
    0, D3D11_SDK_VERSION,
    &swapChainDesc, &swapChain,
    &d3d, &featureLevelYouGot, &gpu ), "Creating device and swapchain" ) ;

  // DID IT WORK?!!?
  if( !ok )
    FatalAppExitA( 0, "Could not create device and swapchain!" ) ;
  
  ///////
  // ok, now we've created a DEVICE and SWAPCHAIN.
  // WHOOPEE!  Pretty soon we'll have triangles
  // on the screen.
  
  // 3.  MSDN:  "Create a render-target view by calling ID3D11Device::CreateRenderTargetView
  //             and bind the back-buffer as a render target by calling
  //             ID3D11DeviceContext::OMSetRenderTargets."
  
  // Get a pointer to the backbuffer (which is really
  // going to be the "texture" used by the swapchain)
  ok = DX_CHECK( swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&backbufferSCTex ), "Get swapchain backbuffer" );
  if( !ok )  FatalAppExitA( 0, "Could not get swapchain backbuffer!" ) ;

  // Now create the render target view. A render target view
  // is basically a "way to access" the data behind it
  // (in this case, its a "lens" thru which we look to
  // access the raw texture data saved in the backbuffer).
  
  ok = DX_CHECK( d3d->CreateRenderTargetView( backbufferSCTex, NULL, &renderTargetView ), "Create Render Target view" ) ;
  if( !ok )  FatalAppExitA( 0, "Could not create render target view!" ) ;
  // We're soon going to link up the OUTPUT MERGER stage
  // of the GPU to this render target (so the gpu ends up
  // drawing into the backbuffer we specified here)




  #pragma region set up the depth stencil

  // Now, we're also going to make a depth stencil
  // while we're at it, so we can have depth testing.
  // This is all from http://msdn.microsoft.com/en-us/library/bb205074(VS.85).aspx
  D3D11_TEXTURE2D_DESC depthTexDesc;
  depthTexDesc.ArraySize          = 1;
  depthTexDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
  depthTexDesc.CPUAccessFlags     = 0;
  depthTexDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depthTexDesc.Width              = width ;
  depthTexDesc.Height             = height ;
  depthTexDesc.MipLevels          = 1;
  depthTexDesc.MiscFlags          = 0;
  depthTexDesc.SampleDesc.Count   = 1;
  depthTexDesc.SampleDesc.Quality = 0;
  depthTexDesc.Usage              = D3D11_USAGE_DEFAULT;

  // Make a texture for the stencil buffer
  ID3D11Texture2D * depthTex;
  DX_CHECK( d3d->CreateTexture2D( &depthTexDesc, NULL, &depthTex ), "creating depth stencil tex" ) ;

  // now create a "view" ("lens" to look at thru) of that depth stencil tex
  DX_CHECK( d3d->CreateDepthStencilView( depthTex, NULL, &depthStencilView ), "Create depth stencil view" ) ;

  // Now link up OM: output-merger
  // http://msdn.microsoft.com/en-us/library/bb205120(VS.85).aspx
  gpu->OMSetRenderTargets( 1, &renderTargetView, depthStencilView ) ;
  
  depthWrite( true ) ;
  #pragma endregion





  // Setup the viewport
  viewport.Width    = width;
  viewport.Height   = height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  gpu->RSSetViewports( 1, &viewport );

  // Alpha blending:
  alphaBlend( true ) ;

  D3D11_RASTERIZER_DESC rasterDesc;
  rasterDesc.FillMode              = D3D11_FILL_SOLID;
  rasterDesc.CullMode              = D3D11_CULL_NONE; //D3D11_CULL_BACK ;
  rasterDesc.FrontCounterClockwise = TRUE; // CW IS FRONT FACING
  rasterDesc.DepthBias             = 0;
  rasterDesc.DepthBiasClamp        = 0;
  rasterDesc.SlopeScaledDepthBias  = 0;
  rasterDesc.ScissorEnable         = FALSE; //All pixels outside an active scissor rectangle are culled.
  rasterDesc.MultisampleEnable     = FALSE; //
  rasterDesc.AntialiasedLineEnable = FALSE;
  
  DX_CHECK( d3d->CreateRasterizerState( &rasterDesc, &rasterizerState ), "create rasterizer state" ) ;
  gpu->RSSetState( rasterizerState );  
  fillmode = FillMode::Solid ;
  cullmode = CullMode::CullNone ;


  #pragma region ready a texture for d2d, and acquire a mutex lock object for it
  //sharedTexSurface = new D3D11Surface( this, 0, 0, width, height, 0, 1, true ) ; //need to pass bgra to it.
  D3D11_TEXTURE2D_DESC sharedTexDesc = { 0 } ;
  sharedTexDesc.ArraySize          = 1 ;
  sharedTexDesc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE ;
  sharedTexDesc.CPUAccessFlags     = 0;
  sharedTexDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;  // the d3d10.1 tex needs to be bgra.
  sharedTexDesc.Width              = width;
  sharedTexDesc.Height             = height;
  sharedTexDesc.MipLevels          = 1;
  sharedTexDesc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX ; // because it will be shared between d3d10.1/d2d1 and d3d11
  sharedTexDesc.SampleDesc.Count   = 1;
  sharedTexDesc.SampleDesc.Quality = 0;
  sharedTexDesc.Usage              = D3D11_USAGE_DEFAULT;
  
  DX_CHECK( d3d->CreateTexture2D( &sharedTexDesc, NULL, &sharedTex ), "Create offscreen d2d tex" ) ;

  // get the shader resource view
  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc ;
  viewDesc.Format = sharedTexDesc.Format ;
  viewDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D ;

  D3D11_TEX2D_SRV texSrv ;
  texSrv.MipLevels = sharedTexDesc.MipLevels ;
  texSrv.MostDetailedMip = 0 ;
  viewDesc.Texture2D = texSrv ;
  DX_CHECK( d3d->CreateShaderResourceView( sharedTex, &viewDesc, &sharedTexResView ), "Create shader resource view" ) ;

  // D3D11 ALSO needs a handle, but it will take it on the D3D11 texture.
  DX_CHECK( sharedTex->QueryInterface<IDXGIKeyedMutex>( &dxgiKeyedMutex11 ), "Get d3d11 keyed mutex from D3D11 Texture" ) ;
  
  // bind a sampler to shut the warning up that occurs when you don't explicitly bind a sampler
  // D3D11_SAMPLER_DESC:  http://msdn.microsoft.com/en-us/library/ff476207(VS.85).aspx
  // D3D11_FILTER:  http://msdn.microsoft.com/en-us/library/ff476132(VS.85).aspx
  D3D11_SAMPLER_DESC ssDesc = { } ;
  ssDesc.Filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_POINT ;
  ssDesc.AddressU = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP ;
  ssDesc.AddressV = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP ;
  ssDesc.AddressW = D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP ;
  ssDesc.MinLOD = -FLT_MAX ;
  ssDesc.MaxLOD =  FLT_MAX ;
  ssDesc.MipLODBias = 0 ;
  ssDesc.MaxAnisotropy = 16 ;
  ssDesc.ComparisonFunc = D3D11_COMPARISON_NEVER ;
  
  DX_CHECK( d3d->CreateSamplerState( &ssDesc, &ssPoint ), "Create ss point" ) ;
  ssDesc.Filter = D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR ;
  DX_CHECK( d3d->CreateSamplerState( &ssDesc, &ssLinear ), "Create ss linear" ) ;
  ssDesc.Filter = D3D11_FILTER::D3D11_FILTER_ANISOTROPIC ;
  DX_CHECK( d3d->CreateSamplerState( &ssDesc, &ssAnisotropic ), "Create ss anisotropic" ) ;

  // you can set these now
  gpu->VSSetSamplers( 0, 1, &ssPoint ) ;
  gpu->PSSetSamplers( 0, 1, &ssPoint ) ;

  gpu->VSSetSamplers( 1, 1, &ssLinear ) ;
  gpu->PSSetSamplers( 1, 1, &ssLinear ) ;
      
  gpu->VSSetSamplers( 2, 1, &ssAnisotropic ) ;
  gpu->PSSetSamplers( 2, 1, &ssAnisotropic ) ;
  
  #pragma endregion

  #pragma region init gpucdata
  
  // init the gpucdatabuffers
  const static int NUM_CBUFFERS = 6 ;

  gpuCDataBuffer.resize( NUM_CBUFFERS, NULL ) ; // put NUM_CBUFFERS nulls in it.
  // if you add another cbuffer then you need to increase the size of the array.
  
  D3D11_BUFFER_DESC gpuDataBufferDesc ;
  gpuDataBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  gpuDataBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  gpuDataBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  gpuDataBufferDesc.MiscFlags = 0;
  gpuDataBufferDesc.StructureByteStride = 0 ;

  gpuDataBufferDesc.ByteWidth = sizeof( GPUCDATA0_Lights ) ;
  DX_CHECK( d3d->CreateBuffer( &gpuDataBufferDesc, 0, &gpuCDataBuffer[0] ), "create buffer gpu-cdata buffer0" ) ;

  gpuDataBufferDesc.ByteWidth = sizeof( GPUCDATA1_Modelview ) ;
  DX_CHECK( d3d->CreateBuffer( &gpuDataBufferDesc, 0, &gpuCDataBuffer[1] ), "create buffer gpu-cdata buffer1" ) ;

  gpuDataBufferDesc.ByteWidth = sizeof( GPUCDATA2_World ) ;
  DX_CHECK( d3d->CreateBuffer( &gpuDataBufferDesc, 0, &gpuCDataBuffer[2] ), "create buffer gpu-cdata buffer2" ) ;

  gpuDataBufferDesc.ByteWidth = sizeof( GPUCDATA3_TextureRotation ) ;
  DX_CHECK( d3d->CreateBuffer( &gpuDataBufferDesc, 0, &gpuCDataBuffer[3] ), "create buffer gpu-cdata buffer3" ) ;

  gpuDataBufferDesc.ByteWidth = sizeof( GPUCDATA4_SHCoeffs ) ;
  DX_CHECK( d3d->CreateBuffer( &gpuDataBufferDesc, 0, &gpuCDataBuffer[4] ), "create buffer gpu-cdata buffer4" ) ;

  gpuDataBufferDesc.ByteWidth = sizeof( GPUCDATA5_Waves ) ;
  DX_CHECK( d3d->CreateBuffer( &gpuDataBufferDesc, 0, &gpuCDataBuffer[5] ), "create buffer gpu-cdata buffer5" ) ;

  // set up the relationship between cpu mems and gpu mems
  for( int i = 0 ; i < gpuCDataBuffer.size() ; i++ )
    updateGPUCData( i ) ;
  #pragma endregion
  
  return true ;
}

void D3D11Window::updateGPUCData( int bufferNumber )
{
  int byteSize = 0 ;
  void* gpuCDataPtr = 0 ;

  switch( bufferNumber )
  {
    case 0:
      byteSize = sizeof( GPUCDATA0_Lights ) ;
      gpuCDataPtr = (void*)&gpuCData0 ;
      break ;
  
    case 1:
      byteSize = sizeof( GPUCDATA1_Modelview ) ;
      gpuCDataPtr = (void*)&gpuCData1 ;
      break ;

    case 2:
      byteSize = sizeof( GPUCDATA2_World ) ;
      gpuCDataPtr = (void*)&gpuCData2 ;
      break ;

    case 3:
      byteSize = sizeof( GPUCDATA3_TextureRotation ) ;
      gpuCDataPtr = (void*)&gpuCData3 ;
      break ;

    case 4:
      byteSize = sizeof( GPUCDATA4_SHCoeffs ) ;
      gpuCDataPtr = (void*)&gpuCData4 ;
      break ;

    case 5:
      byteSize = sizeof( GPUCDATA5_Waves ) ;
      gpuCDataPtr = (void*)&gpuCData5 ;
      break ;

    default:
      error( "updateGPUCData bad index %d", bufferNumber ) ;
      break ;
  }

  D3D11_MAPPED_SUBRESOURCE mappedRes ;
  DX_CHECK( gpu->Map( gpuCDataBuffer[ bufferNumber ], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes ), "Map gpucdatabuffer" );
  memcpy( mappedRes.pData, gpuCDataPtr, byteSize ) ; // write to the gpu mems.
  
  gpu->VSSetConstantBuffers( bufferNumber, 1, &gpuCDataBuffer[ bufferNumber ] );
  gpu->PSSetConstantBuffers( bufferNumber, 1, &gpuCDataBuffer[ bufferNumber ] );
  gpu->Unmap( gpuCDataBuffer[ bufferNumber ], 0 ) ;
      
  /// This doesn't seem to work.
  //gpu->UpdateSubresource( gpuCDataBuffer[0], 0, 0, gpuCDataPtr, byteSize, 0 ) ;
}

void D3D11Window::screenshot()
{
  char buf[ 300 ] ;

  // rather than cd, include the full path
  // sprintf returns to you the length of the string,
  // which is also the array position of the NULL character
  int len = sprintf( buf, DIR_BASE("/code/gtp snapshots/images/") ) ;
  len += sprintNow( buf + len ) ;
  sprintf( buf + len, ".png" ) ;
  
  if( DX_CHECK( D3DX11SaveTextureToFileA( gpu, backbufferSCTex, D3DX11_IMAGE_FILE_FORMAT::D3DX11_IFF_PNG, buf ), "snapshot" ) )
    info( Cyan, "Snapped a screenshot %s", buf ) ;
  else
    error( "Snapshot failed" ) ;
}

void D3D11Window::screenshot( char* name )
{
  char buf[ 300 ] ;

  // rather than cd, include the full path
  // sprintf returns to you the length of the string,
  // which is also the array position of the NULL character
  int len = sprintf( buf, DIR_BASE("/code/gtp snapshots/images/") ) ;
  //len += sprintNow( buf + len ) ;
  sprintf( buf + len, "%s.png", name ) ;
  
  if( DX_CHECK( D3DX11SaveTextureToFileA( gpu, backbufferSCTex, D3DX11_IMAGE_FILE_FORMAT::D3DX11_IFF_PNG, buf ), "snapshot" ) )
    info( Cyan, "Snapped a screenshot %s", buf ) ;
  else
    error( "Snapshot failed" ) ;
}

void D3D11Window::clear( ByteColor toColor )
{
  // Clear the back buffer
  float clearColor[4] ; // RGBA
  toColor.toRGBAVector().writeFloat4( clearColor ) ;
  gpu->ClearRenderTargetView( renderTargetView, clearColor );

  // Clear the depth buffer to all 1's (every pixel is as far away as it could be)
  // and the stencil buffer to all 0's
  gpu->ClearDepthStencilView( depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0 );

}

void D3D11Window::alphaBlend( bool on )
{
  // can't get this to work without creating from scratch.
  ////SAFE_RELEASE( blendState ) ;

  D3D11_RENDER_TARGET_BLEND_DESC rtBlendDesc ;
  memset( &rtBlendDesc, 0, sizeof( D3D11_RENDER_TARGET_BLEND_DESC ) ) ;
  
  rtBlendDesc.BlendEnable = on ; // ONLY OPTION THAT GETS SENT TO THIS FUNCTION
  
  // The SOURCE blend pixel is the NEW FRAGMENT that was generated.
  // The DESTINATION blend pixel is the ONE THAT WAS ALREADY THERE.
  //   OutputPixel = ( SourceColor.rgba * SrcBlend )   __BlendOp__   ( DestColor.rgba * DestBlend )
  rtBlendDesc.SrcBlend  = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA ;   // new pixel contributes Alpha% to final pixel color
  rtBlendDesc.DestBlend = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA ;  // existing pixel contributes (1-Alpha)% to final pixel color.
  rtBlendDesc.BlendOp   = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD ;   // defines how to combine the RGB data sources
  rtBlendDesc.SrcBlendAlpha  = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA ;
  rtBlendDesc.DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA ;
  rtBlendDesc.BlendOpAlpha   = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD ;

  rtBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL ;
  
  D3D11_BLEND_DESC blendStateDesc;
  ZeroMemory( &blendStateDesc, sizeof( D3D11_BLEND_DESC ) );
  blendStateDesc.RenderTarget[0] = rtBlendDesc ;
  DX_CHECK( d3d->CreateBlendState( &blendStateDesc, &blendState ), "Create blendstate" ) ;
  gpu->OMSetBlendState( blendState, 0, 0xffffffff );
}

void D3D11Window::depthWrite( bool on )
{
  // Create Depth-Stencil State
  // --
  // The depth-stencil state tells the output-merger stage
  // how to perform the depth-stencil test. The depth-stencil
  // test determines whether or not a given pixel should be drawn.
  D3D11_DEPTH_STENCIL_DESC depthAndStencil ;
  depthAndStencil.DepthEnable = true ;

  // Turn ON or OFF WRITES to the depth stencil
  // http://msdn.microsoft.com/en-us/library/ff476113(VS.85).aspx
  if( on )
    depthAndStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL ;
  else
    depthAndStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO ;
  
  // What makes a PASS?  http://msdn.microsoft.com/en-us/library/ff476101(VS.85).aspx
  depthAndStencil.DepthFunc = D3D11_COMPARISON_LESS ;  //LESS:If the source data is less than the destination data, the comparison passes.
  // This means:  If the new fragment has LESS DEPTH (ie is closer to the eye) than
  // the existing fragment, then the NEW fragment "passes" (ie is kept), and the
  // old fragment fails (ie is thrown away).
  
  depthAndStencil.StencilEnable = false;  // NOT stenciling.
  
  // Create depth stencil state
  ID3D11DepthStencilState *depthStencilState ;

  // Interestingly, if you try to create a state that already exists, 
  // "If an application attempts to create a depth-stencil-state interface with the same state as an existing interface, the same interface will be returned and the total number of unique depth-stencil state objects will stay the same."
  DX_CHECK( d3d->CreateDepthStencilState( &depthAndStencil, &depthStencilState ), "CreateDepthStencilState" ) ;

  gpu->OMSetDepthStencilState( depthStencilState, 0 ) ;

  depthStencilState->Release() ; // I don't have this reference anymore, after here.
}

void D3D11Window::setBackgroundColor( ByteColor color )
{
  error( "Invalid" ) ;
  ///clearColor = color ;
}

void D3D11Window::toggleFillMode()
{
  if( fillmode == FillMode::Solid )
    setFillMode( FillMode::Wireframe ) ;
  else
    setFillMode( FillMode::Solid ) ;
}

void D3D11Window::cullMode( CullMode cullThese )
{
  cullmode = cullThese ;
  gpu->RSGetState( &rasterizerState ) ;

  D3D11_RASTERIZER_DESC rasterDesc ;
  rasterizerState->GetDesc( &rasterDesc ) ;
  
  rasterizerState->Release() ; // drop old one

  // always consider the FRONT as counter clockwise wound faces
  rasterDesc.FrontCounterClockwise = TRUE ;

  switch( cullmode ) // change it
  {
    case CullMode::CullClockwise:
      rasterDesc.CullMode = D3D11_CULL_BACK ;
      break ;

    case CullMode::CullCounterClockwise:
      rasterDesc.CullMode = D3D11_CULL_FRONT ;
      break ;

    default:
      warning( "Invalid cullmode, culling nothing"  ) ;

    case CullMode::CullNone:
      rasterDesc.CullMode = D3D11_CULL_NONE ;
      break ;
  }

  //create/set new one
  DX_CHECK( d3d->CreateRasterizerState( &rasterDesc, &rasterizerState ), "Create rasterizer state" ) ;

  gpu->RSSetState( rasterizerState ) ;
}

void D3D11Window::setFillMode( FillMode iFillMode )
{
  fillmode = iFillMode ;
  gpu->RSGetState( &rasterizerState ) ;

  D3D11_RASTERIZER_DESC rasterDesc ;
  rasterizerState->GetDesc( &rasterDesc ) ;
  
  rasterizerState->Release() ; // drop old one

  switch( fillmode ) // change it
  {
    case FillMode::Wireframe:
      rasterDesc.FillMode = D3D11_FILL_WIREFRAME ;
      break ;

    default:
      warning( "Invalid fillmode %d, you get solidfill", iFillMode ) ;

    case FillMode::Solid:
      rasterDesc.FillMode = D3D11_FILL_SOLID ;
      break ;
  }

  //create/set new one
  DX_CHECK( d3d->CreateRasterizerState( &rasterDesc, &rasterizerState ), "Create rasterizer state" ) ;

  gpu->RSSetState( rasterizerState ) ;
}

void D3D11Window::setTextureSamplingMode( TextureSamplingMode texSampMode )
{
  switch( texSampMode )
  {
    case TextureSamplingMode::Point:
      gpu->VSSetSamplers( 0, 1, &ssPoint ) ;
      gpu->PSSetSamplers( 0, 1, &ssPoint ) ;
      break ;

    case TextureSamplingMode::Linear:
      gpu->VSSetSamplers( 0, 1, &ssLinear ) ;
      gpu->PSSetSamplers( 0, 1, &ssLinear ) ;
      break ;

    default:
      warning( "Invalid tex sample mode %d, you get anisotropic", texSampMode ) ;

    case TextureSamplingMode::AnisotropicSampling:
      gpu->VSSetSamplers( 0, 1, &ssAnisotropic ) ;
      gpu->PSSetSamplers( 0, 1, &ssAnisotropic ) ;
      break ;
  }
  
}

// A general undo( statename ) would be helpful
void D3D11Window::revertPointSize()
{
  // You can't actually change point size in d3d11
  error( "revertPoint Not implemented" ) ;
}
void D3D11Window::setPointSize( float newPtSize )
{
  error( "setPointSize Not implemented" ) ;
}

bool D3D11Window::createFont( int fontId, int height, int boldness, char* fontName )
{
  return directWrite->createFont( fontId, height, boldness, fontName ) ;
}
bool D3D11Window::loadTexture( int textureId, char* filename, ByteColor backgroundColor )
{
  error( "loadTex Not implemented" ) ;
  return 0 ;
}

void D3D11Window::drawText( int fontId,
  const char *str, ByteColor color,
  float x, float y, float boxWidth, float boxHeight,
  DWORD formatOptions )
{
  directWrite->draw( fontId, str, color, x, y, boxWidth, boxHeight, formatOptions ) ;
}

bool D3D11Window::beginText()
{
  // d3d11 only releases the mutex
  // long enough for directwrite to finish drawing to it.
  // Let d3d11 release its lock on the sharedTex
  static bool first = true ;
  if( first ){
    //info( "D3D11Window skipping mutex release.." ) ; 
    first =false;
  }
  else
    DX_CHECK( dxgiKeyedMutex11->ReleaseSync( 0 ), "d3d11 release mutex" ) ;

  // Now LET D2D DRAW ON THE sharedTex
  directWrite->begin() ;

  return true ; // nothing to change in d3d11
}

// draws the directwrite output TO the sharedTex
bool D3D11Window::endText()
{
  directWrite->end() ;

  // Now D3D11 takes back its lock on
  // the sharedTex for the rest of the time
  // (rendering time)
  return DX_CHECK( dxgiKeyedMutex11->AcquireSync( 1, INFINITE ), "d3d acquire mutex" ) ;
}

bool D3D11Window::beginDrawing()
{
  // CHECK DEVICE STATUS
  bool deviceAcq = DX_CHECK(swapChain->Present( 0, DXGI_PRESENT_TEST ), "dxgi present test" ) ;

  if( !deviceAcq )
  {
    info( "You lost the device" ) ;

    // DON'T TRY AND RENEW DEVICE IF NOT HAVE FOCUS
    if( focus )
    {
      info( "Renewing the device.." ) ;
    }
  }
  
  return deviceAcq ;  // if you DON'T have the device, don't
  // try to draw.
}

bool D3D11Window::endDrawing()
{
  // no end
  return true ;
}

bool D3D11Window::present()
{
  return DX_CHECK( swapChain->Present( 0, 0 ), "Present" ) ;
}




