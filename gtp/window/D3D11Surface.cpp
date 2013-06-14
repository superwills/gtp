#include "../window/D3D11Window.h"
#include "D3D11Surface.h"

const float D3D11Surface::clearColor[4] = {1.0f,0,0,1.0f};

D3D11Surface::D3D11Surface( D3D11Window* iWin, int ix, int iy, int iWidth, int iHeight, real zNear, real zFar,
  real zLevelForDrawing, SurfaceType iSurfaceType, int slotId, DXGI_FORMAT texFormat, int ibpp )
{
  memset( &subres, 0, sizeof( D3D11_MAPPED_SUBRESOURCE ) );
  bpp = ibpp;
  surfaceType = iSurfaceType ;
  slot = slotId ;

  tex=texStage=depthTex=depthTexStage=0;
  rtv=0;
  dsv=0;
  win = iWin ;

  viewport.TopLeftX = ix ;
  viewport.TopLeftY = iy ;
  viewport.Width = iWidth ;
  viewport.Height = iHeight ;
  viewport.MinDepth = zNear ;
  viewport.MaxDepth = zFar ;

  memset( &texDesc, 0, sizeof( D3D11_TEXTURE2D_DESC ) ) ;
  
  texDesc.Width = iWidth ;
  texDesc.Height = iHeight ;
  texDesc.MipLevels = 1;
  texDesc.ArraySize = 1;
  texDesc.Format = texFormat ; // RGBA!! This is the only format you can save to PNG.
  // otherwise you get the fail comment, "can't find matching WIC format, please save this file to a dds"
  //texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM ; // BGRA!! do not use.
  texDesc.SampleDesc.Count = 1 ;  // no multisampling
  texDesc.SampleDesc.Quality = 0 ;// no multisampling

  real z = zLevelForDrawing, alpha = 1.0 ;

  // Create 4 verts for the corners
  // of the quad we'll use to draw
  // the rt texture to the screen
  VertexTC v[4] ;

  if( surfaceType == SurfaceType::RenderTarget ) // GPU WRITE
  {
    #pragma region make raster/render target
    texDesc.Usage = D3D11_USAGE_DEFAULT ; // the only one that can allow GPU write
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // BIND AS A RENDER TARGET!!
    texDesc.CPUAccessFlags = 0 ; // IF you use D3D11_CPU_ACCESS_READ you must create a staging tex,
    // and a staging tex cannot be set up as outpu or input to the GPU
    texDesc.MiscFlags = 0 ;//D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GENERATE_MIPS http://msdn.microsoft.com/en-us/library/ff476203(VS.85).aspx

    DX_CHECK( win->d3d->CreateTexture2D( &texDesc, NULL, &tex ), "Create render target texture" ) ;


    // create the staging copy now.
    // you need this if you want to read
    // the pixels of this render target, 
    D3D11_TEXTURE2D_DESC texDescStaging = texDesc ;
    texDescStaging.Usage = D3D11_USAGE_STAGING ;
    texDescStaging.BindFlags = 0 ;
    texDescStaging.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE ;
    DX_CHECK( win->d3d->CreateTexture2D( &texDescStaging, NULL, &texStage ), "Create rt staging texture" ) ;


    // set up the render target view
    DX_CHECK( win->d3d->CreateRenderTargetView( tex, NULL, &rtv ), "Create render target view for surface" ) ;

    // need a depth stencil for this one
    D3D11_TEXTURE2D_DESC depthTexDesc;
    depthTexDesc.ArraySize          = 1;
    depthTexDesc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
    depthTexDesc.CPUAccessFlags     = 0;
    //depthTexDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT; // "A 32-bit z-buffer format that supports 24 bits for depth and 8 bits for stencil."
    depthTexDesc.Format             = DXGI_FORMAT_D32_FLOAT ;
    depthTexDesc.Width              = texDesc.Width ;
    depthTexDesc.Height             = texDesc.Height ;
    depthTexDesc.MipLevels          = 1;
    depthTexDesc.MiscFlags          = 0;
    depthTexDesc.SampleDesc.Count   = 1;
    depthTexDesc.SampleDesc.Quality = 0;
    depthTexDesc.Usage              = D3D11_USAGE_DEFAULT;

    // Make a texture for the depth buffer
    DX_CHECK( win->d3d->CreateTexture2D( &depthTexDesc, NULL, &depthTex ), "creating depth stencil tex" ) ;

    // now create a "view" ("lens" to look at thru) of that depth tex
    DX_CHECK( win->d3d->CreateDepthStencilView( depthTex, NULL, &dsv ), "Create depth stencil view" ) ;


    //STAGING COPY of the depth tex
    D3D11_TEXTURE2D_DESC depthTexDescStaging = depthTexDesc ;
    depthTexDescStaging.Usage = D3D11_USAGE_STAGING;
    depthTexDescStaging.BindFlags = 0 ;
    depthTexDescStaging.CPUAccessFlags = D3D11_CPU_ACCESS_READ ;
    DX_CHECK( win->d3d->CreateTexture2D( &depthTexDescStaging, NULL, &depthTexStage ), "Create rt depth tex staging texture" ) ;



    // draws it the same way (without flipping)
    // 0    2
    // |   /|
    // v /  V
    // 1    3
    // For a triangle list though, we go
    // 0 1 2 3
    real s = 1.0 ;
    // V INVERTED in this one
    // U,V bottom left corner = (0,1)
    v[0] = VertexTC( Vector(-s, +s, z),  Vector( 0, 0 ),  Vector(1,1,1,alpha) ) ;
    v[1] = VertexTC( Vector(-s, -s, z),  Vector( 0, 1 ),  Vector(1,1,1,alpha) ) ;
    v[2] = VertexTC( Vector(+s, +s, z),  Vector( 1, 0 ),  Vector(1,1,1,alpha) ) ; 
    v[3] = VertexTC( Vector(+s, -s, z),  Vector( 1, 1 ),  Vector(1,1,1,alpha) ) ;
    #pragma endregion
  }
  else // SurfaceType::ColorBuffer: CPU WRITE: BLOB OF PIXEL __I__ CAN WRITE TO
  {
    #pragma region make raytracing surface
    texDesc.Usage = D3D11_USAGE_DYNAMIC ; // CPU WRITE / GPU READ
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // its a resource for the shader that will render it.
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE ; // IF you use D3D11_CPU_ACCESS_READ you must create a staging tex,
    // and a staging tex cannot be set up as outpu or input to the GPU
    texDesc.MiscFlags = 0 ;

    DX_CHECK( win->d3d->CreateTexture2D( &texDesc, NULL, &tex ), "Create dynamic surface texture" ) ;

    BYTE* data = open() ;
    for( int row = 0; row < texDesc.Height ; row++ )
    {
      int rowStart = row * subres.RowPitch ;  // rowpitch is bytes per row
      for( int col = 0; col < texDesc.Width; col++ )
      {
        int colStart = col * 4;
        int index = rowStart + colStart ;
        data[index + 0] = 255 ; // r
        data[index + 1] = 0 ;   // g
        data[index + 2] = ((real)col/texDesc.Width)*255 ; // b
        data[index + 3] = 255 ; // a
      }
    }
    close() ;

    // flips the texture vertically

    // 2 -> 3
    //  \ 
    //    \ 
    // 0 -> 1
    // For a triangle list though, we go
    // 0 1 2 3

    // b-a
    // c-d

    // TEXCOORDS STRAIGHT
    real s = 1.0 ;
    v[0] = VertexTC( Vector(-s, -s, z),  Vector(0, 1),  Vector(1,1,1,alpha) ) ;  // c
    v[1] = VertexTC( Vector( s, -s, z),  Vector(1, 1),  Vector(1,1,1,alpha) ) ;  // d
    v[2] = VertexTC( Vector(-s, +s, z),  Vector(0, 0),  Vector(1,1,1,alpha) ) ;  // b
    v[3] = VertexTC( Vector(+s, +s, z),  Vector(1, 0),  Vector(1,1,1,alpha) ) ;  // a
    #pragma endregion
  }
  

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc ;
  viewDesc.Format = texDesc.Format ;
  viewDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D ;

  D3D11_TEX2D_SRV texSrv ;
  texSrv.MipLevels = texDesc.MipLevels ;
  texSrv.MostDetailedMip = 0 ;
  viewDesc.Texture2D = texSrv ;
  DX_CHECK( win->d3d->CreateShaderResourceView( tex, &viewDesc, &shaderResView ), "Create shader resource view" ) ;

  // create a vertex buffer
  vb = new TVertexBuffer<VertexTC>( win, v, 4 ) ;
}

D3D11Surface::~D3D11Surface()
{
  SAFE_RELEASE( tex ) ;
  SAFE_RELEASE( texStage ) ;
  SAFE_RELEASE( depthTex ) ;
  SAFE_RELEASE( depthTexStage ) ;
  SAFE_RELEASE( rtv ) ;
  SAFE_RELEASE( dsv ) ;
  DESTROY( vb ) ;
}

// to read pixel values we must "lock" the surface.
// what we'll do is require an "open" and "close"
// in the interface
BYTE* D3D11Surface::open()
{
  if( isOpened() )
  {
    warning( "Texture was already opened and you tried to open it again" ) ;
    return (BYTE*)subres.pData ;
  }

  // CopyStructureCount:  Copies data from a buffer holding variable length data.
  // http://msdn.microsoft.com/en-us/library/ff476393(VS.85).aspx
  // CopyResource:  Copy the entire contents of the source resource to the destination resource using the GPU.
  // http://msdn.microsoft.com/en-us/library/ff476392(VS.85).aspx
  // UpdateSubresource:  The CPU copies data from memory to a subresource created in non-mappable memory.
  // http://msdn.microsoft.com/en-us/library/ff476486(VS.85).aspx

  if( texDesc.Usage == D3D11_USAGE::D3D11_USAGE_DYNAMIC ) // COLORBUFFER
  {
    // GPU READ, CPU WRITE
    // http://msdn.microsoft.com/en-us/library/ff476457(VS.85).aspx
    // ::Map: Get a pointer to the data contained in a subresource, and deny the GPU access to that subresource.
    //info( "Opening COLORBUFFER texture for WRITING. "
    //  "You better write the whole texture because now "
    //  "anything that WAS in the texture has been invalidated." ) ;

    DX_CHECK( win->gpu->Map(
      tex,
      D3D11CalcSubresource( 0,0,0 ),
      D3D11_MAP_WRITE_DISCARD, // You must use D3D11_MAP_WRITE_DISCARD with Dynamic textures
      // "the previous contents of the resource will be undefined." .. ie you are obliged to write the WHOLE THING!
      0,
      &subres ), "gpu map subresource" ) ;

    return (BYTE*)subres.pData ; // RGBA!!
  }
  else if( texDesc.Usage == D3D11_USAGE::D3D11_USAGE_DEFAULT ) // RENDERTARGETS
  {
    // GPU READ, GPU WRITE (so it's a render target)

    // needs to be staged first,
    // COPY TO STAGING TEX
    win->gpu->CopyResource( texStage, tex ) ;

    // map the staging resource as READ/WRITE..
    DX_CHECK( win->gpu->Map(
      texStage,
      D3D11CalcSubresource( 0,0,0 ),
      D3D11_MAP_READ_WRITE, // the staging tex can be both read from and written to.
      0,
      &subres ), "gpu map subresource" ) ;

    // Now you can safely read it
    return (BYTE*)subres.pData ; // RGBA!!
  }
  else
  {
    error( "Not DYNAMIC or DEFAULT texture" ) ;
    return NULL ;
  }
}

float* D3D11Surface::openDepthTex()
{
  if( texDesc.Usage == D3D11_USAGE::D3D11_USAGE_DEFAULT ) // RENDERTARGETS
  {
    // GPU READ, GPU WRITE (so it's a render target)

    // needs to be staged first,
    // COPY TO STAGING TEX
    win->gpu->CopyResource( depthTexStage, depthTex ) ;

    // map the staging resource as READ/WRITE..
    DX_CHECK( win->gpu->Map(
      depthTexStage,
      D3D11CalcSubresource( 0,0,0 ),
      D3D11_MAP_READ, // the staging tex can be both read from and written to.
      0,
      &subresDepth ), "gpu map depth subresource" ) ;

    // Now you can safely read it
    return (float*)subresDepth.pData ; // RGBA!!
  }
  else
  {
    error( "Colorbuffer but why do you want to write the depth tex? You can't see it." ) ;
    return 0 ;
  }
}

void D3D11Surface::close()
{
  if( texDesc.Usage == D3D11_USAGE_DYNAMIC ) //COLORBUFFER
    win->gpu->Unmap( tex, D3D11CalcSubresource(0,0,0) ) ; // allow the gpu to use it again
  else if( texDesc.Usage == D3D11_USAGE_DEFAULT ) //RENDERTARGETS
  {
    // copy the staging tex to the default tex
    // PRACTICALLY you don't expect to need to do this.
    // usually you will open() a rendertarget for READING,
    // after the gpu has written to it.
    //win->gpu->UpdateSubresource( tex, D3D11CalcSubresource( 0,0,0 ),
    //  NULL, texStage, subres.RowPitch, subres.DepthPitch ) ;
    win->gpu->Unmap( texStage, D3D11CalcSubresource(0,0,0) ) ; // allow the gpu to use it again
  }
  else
    error( "Not DYNAMIC or DEFAULT texture" ) ;

  subres.pData = 0 ;          // invalidate the pointer
}

void D3D11Surface::closeDepthTex()
{
  win->gpu->Unmap( depthTexStage, D3D11CalcSubresource(0,0,0) ) ; // allow the gpu to use it again
}

bool D3D11Surface::isOpened()
{
  return (bool)subres.pData ;
}

ByteColor* D3D11Surface::clear( ByteColor toColor )
{
  float clearColor[4];
  toColor.toRGBAVector().writeFloat4( clearColor ) ;

  // clear it first
  // you can only use this method if it's a RenderTarget
  if( rtv )
  {
    win->gpu->ClearRenderTargetView( rtv, clearColor );
    
    // Clear the depth buffer to all 1's (every pixel is as far away as it could be)
    // and the stencil buffer to all 0's
    if( dsv )
      win->gpu->ClearDepthStencilView( dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0 );

    return 0 ;
  }
  else
  {
    // manually set the contents to all 0.
    int pxPerRow = getPxPerRow() ; // include the "wasted bytes"
    ByteColor* p = (ByteColor*)open() ;

    for( int i = 0 ; i < getHeight()*pxPerRow ; i++ )
      p[i] = toColor ;
    
    return p ;  // if you close it you lose yoru "work" in clearing this.
    //close();  ////WELL LEAVE IT OPEN!!! If you close it
    // and open it again then actually your "clear" work is lost!
    // (each time you open a dynamic texture you are obliged to write the
    // whole thing!)
  }

  
}

void D3D11Surface::draw()
{
  activate() ;
  vb->drawTriangleStrip() ;
  deactivate() ;
}

int D3D11Surface::getWidth() const
{
  return texDesc.Width ;
}

int D3D11Surface::getHeight() const
{
  return texDesc.Height ;
}

int D3D11Surface::getNumPixels() const
{
  return getWidth()*getHeight() ;
}

int D3D11Surface::getPixelByteIndex( int row, int col ) const
{
  int skipBytes = getWastedBytesAtEndOfEachRow() ;
  return row*subres.RowPitch + col * bpp ; // row*bytesPerRow + col*bytesPerPixel
}

// function to get a pointer to the color buffer
// of this surface.
ByteColor* D3D11Surface::getColorAt( int pxNum )
{
  // Well, there are getWastedBytesAtEndOfEachRow()
  // bytes at the end of each row, so,
  // you have to know the row to know how
  // many bytes to skip
  int row = pxNum / getWidth() ;
  int col = pxNum % getWidth() ;
  return getColorAt( row, col ) ;
}

ByteColor* D3D11Surface::getColorAt( int row, int col )
{
  int i = getPixelByteIndex( row, col ) ;
  return (ByteColor*)( (BYTE*)subres.pData + i ) ;
}

float* D3D11Surface::getDepthAt( int row, int col )
{
  return (float*)subresDepth.pData ;
}

void D3D11Surface::setColorAt( int row, int col, ByteColor val )
{
  int i = getPixelByteIndex( row, col ) ;
  *(ByteColor*)( (BYTE*)subres.pData + i ) = val ;
}

void D3D11Surface::setColorAt( int row, int col, USHORT h1, USHORT h2 )
{
  int i = getPixelByteIndex( row, col ) ;
  BYTE* ptr = (BYTE*)subres.pData + i ; // advance the pointer i bytes

  // treating the pointer as a USHORT pointer, write h1
  *(USHORT*)( ptr ) = h1 ;

  // advance 2 bytes and write h2
  *(USHORT*)( ptr+2 ) = h2 ;
}

void D3D11Surface::setColorAt( int row, int col, const Vec3& color )
{
  int i = getPixelByteIndex( row, col ) ;
  BYTE* ptr = (BYTE*)subres.pData + i ; // advance the pointer i bytes

  // Now treat it as a Vec3 pointer, write the 96 bits of color information.
  *(Vec3*)ptr = color ;
}

void D3D11Surface::setColorAt( Texcoording& ctex, const Vec3& color )
{
  int i = getPixelByteIndex( ctex.row, ctex.col ) ;
  ctex.next() ;//advance here
  BYTE* ptr = (BYTE*)subres.pData + i ; // advance the pointer i bytes

  // Now treat it as a Vec3 pointer, write the 96 bits of color information.
  *(Vec3*)ptr = color ;
}

void D3D11Surface::setColorAt( Texcoording& ctex, const Vector& color )
{
  int i = getPixelByteIndex( ctex.row, ctex.col ) ;
  ctex.next() ;//advance here
  BYTE* ptr = (BYTE*)subres.pData + i ; // advance the pointer i bytes

  *(Vec4*)ptr = color ;
}

int D3D11Surface::getWastedBytesAtEndOfEachRow() const
{
  // (RowPitch-Width*bpp) bytes at the end of each row is wasted.
  return subres.RowPitch - bpp*getWidth() ; // WASTED SPACE at the end of each row.
  // RowPitch = bytes of image per row, including wasted spaced, padding
  // bpp*WIDTH  = bpp bytes per pixel, * width of image is # ACTUAL used pixels per row.
}

int D3D11Surface::getWastedPxAtEndOfEachRow() const
{
  return getWastedBytesAtEndOfEachRow() / bpp ;
}

int D3D11Surface::getPxPerRow()
{
  return getWidth() + getWastedPxAtEndOfEachRow() ;
}

void D3D11Surface::copyFromByteArray( BYTE * arrSrc, int bytesPerPixel, int numPixels )
{
  BYTE* data = open() ;
  
  int rows = getHeight() ;
  int cols = getWidth() ;

  // you can copy in groups of subres.RowPitch, then skip skipBytes
  // at the end of each row
  //int skipBytes = getWastedBytesAtEndOfEachRow() ;

  for( int row = 0; row < rows ; row++ )
  {
    int dstRowStart = row * subres.RowPitch ;  // RowPitch is the number of __BYTES__ in ROW INCLUDING WASTED SPACE.
    // A row doesn't start @ (row*COLS_PER_ROW) for this reason..
    // there may be wasted bytes at the end of each row
    // xxxx..
    // xxxx..
    // ROWS=2, COLS=4, ROWPITCH=6 (2 wasted bytes per row)!
    int srcRowStart = row * ( bytesPerPixel * cols ); // however _my_ array is always tightly packed.
    memcpy( data + dstRowStart, arrSrc + srcRowStart, bytesPerPixel*cols ) ;
  }

  close() ;
}

// make this the active texture
void D3D11Surface::activate() //override
{
  win->activeShader->setTexture( slot, shaderResView ) ;
}

// unset this as the active texture
void D3D11Surface::deactivate() //override
{
  // this is an alternate way to deact the tex,
  // you can also call the shader's unsetTexture method
  // directly
  win->activeShader->unsetTexture( slot ) ;
}

void D3D11Surface::rtBind()
{
  if( surfaceType != SurfaceType::RenderTarget ) {
    error( "Surface not a render target, can't bound as one" ) ;
    return ;
  }
  win->gpu->OMSetRenderTargets( 1, &rtv, dsv ) ; // set the depth texture that comes with this surface
  win->gpu->RSSetViewports( 1, &viewport );
}

void D3D11Surface::rtUnbind()
{
  if( surfaceType != SurfaceType::RenderTarget ) {
    error( "Not a render target" ) ;
    return ;
  }
  win->gpu->OMSetRenderTargets( 1, &win->renderTargetView, win->depthStencilView ) ;
  win->gpu->RSSetViewports( 1, &win->viewport );
}

void D3D11Surface::rtBindMainscreen()
{
  if( surfaceType != SurfaceType::RenderTarget ) {
    error( "Not a render target" ) ;
    return ;
  }
  win->gpu->OMSetRenderTargets( 1, &win->renderTargetView, win->depthStencilView ) ;
  win->gpu->RSSetViewports( 1, &win->viewport );
}

bool D3D11Surface::save( char* filename )
{
  return DX_CHECK( D3DX11SaveTextureToFileA(
    win->gpu,
    tex,
    D3DX11_IMAGE_FILE_FORMAT::D3DX11_IFF_PNG,
    filename ), "Saving texture as .png image file" ) ;
}

// saves to a DDS
void D3D11Surface::saveDDS( char* filename )
{
  DX_CHECK( D3DX11SaveTextureToFileA(
    win->gpu,
    tex,
    D3DX11_IMAGE_FILE_FORMAT::D3DX11_IFF_DDS,
    filename ), "Saving texture as DDS" ) ;
}

// saves jpg
void D3D11Surface::saveJPG( char* filename ) 
{
  DX_CHECK( D3DX11SaveTextureToFileA(
    win->gpu,
    tex,
    D3DX11_IMAGE_FILE_FORMAT::D3DX11_IFF_JPG,
    filename ), "Saving texture as jpg" ) ;
}

void D3D11Surface::setAlpha( float alpha )
{
  VertexTC* dp = (VertexTC*)vb->lockVB() ;
  for( int i = 0 ; i < 4 ; i++ ) // 4 VERTS
    dp[i].color.w = alpha ;
  vb->unlockVB();
}
