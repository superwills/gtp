#include "D3D11Texture.h"
#include "GTPWindow.h"


D3D11Texture::D3D11Texture( int iWidth, int iHeight )
{
  tex = 0 ;

  width = iWidth ;
  height = iHeight ;

  memset( &texDesc, 0, sizeof( D3D11_TEXTURE2D_DESC ) ) ;
  texDesc.Width = iWidth ;
  texDesc.Height = iHeight ;
  texDesc.MipLevels = 1;
  texDesc.ArraySize = 1;
  texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM ;
  texDesc.SampleDesc.Count = 1 ;  // no multisampling
  texDesc.SampleDesc.Quality = 0 ;// no multisampling
  texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE ;

  // create the tex!
  DX_CHECK( window->d3d->CreateTexture2D( &texDesc, NULL, &tex ), "Create texture" ) ;

  D3D11_SHADER_RESOURCE_VIEW_DESC texRsvDesc ;
	texRsvDesc.Format = texDesc.Format;
	texRsvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D ;
	texRsvDesc.TextureCube.MipLevels = texDesc.MipLevels;
	texRsvDesc.TextureCube.MostDetailedMip = 0; //always 0
  DX_CHECK( window->d3d->CreateShaderResourceView( tex, &texRsvDesc, &shaderResView ), "create shader res view texture" ) ;

  // initialize and write random values
  colorValues.resize( width*height ) ;
  
  //for( int i = 0 ; i < colorValues.size() ; i++ )
  //  colorValues[i] = Vector::random() ;
  generateNoise() ;

  writeTex() ;

}

D3D11Texture::D3D11Texture( char* filename )
{
  tex = 0 ;
  texStage = 0 ; // unused
  // stage it first to extract the colorValues
  #pragma region read px and save
  { // scope block destroys the temporaries
    D3DX11_IMAGE_LOAD_INFO loadInfo ;
	  loadInfo.MiscFlags = 0;
    loadInfo.Usage = D3D11_USAGE::D3D11_USAGE_STAGING;
    loadInfo.CpuAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ;
    loadInfo.FirstMipLevel = 0 ;
    loadInfo.MipLevels = 1 ;
    loadInfo.BindFlags = 0 ;
  
    ID3D11Texture2D *stageTex ;
    DX_CHECK( D3DX11CreateTextureFromFileA(
      window->d3d, filename, &loadInfo, 0, (ID3D11Resource**)&stageTex, 0 ), 
      "load texture" );

    // nab the texture values
    D3D11_TEXTURE2D_DESC stageTexDesc ;
	  stageTex->GetDesc(&stageTexDesc);
  
    width = stageTexDesc.Width ;
    height = stageTexDesc.Height ;
  
    info( "Tex '%s' loaded, width=%d, height=%d", filename, width, height ) ;

    colorValues.resize( width*height ) ;
  
    D3D11_MAPPED_SUBRESOURCE mappedSubRes ;

    DX_CHECK( window->gpu->Map( stageTex, 0, D3D11_MAP::D3D11_MAP_READ, 0, &mappedSubRes ),
      "Map the texture for cpu read" ) ;
  
    int bytesPerPixel ;
    
    switch( stageTexDesc.Format )
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      bytesPerPixel = 16 ;
      break ;

    case DXGI_FORMAT_R8G8B8A8_UNORM: // default 8-bit format
    default:
      bytesPerPixel = 4 ;
      break ;
    }

    int wastedBytes = mappedSubRes.RowPitch - bytesPerPixel*width ; // actual bytes in a row - SHOULD (computed) bytes in a row
    int wastedIndicesPerRow = wastedBytes/bytesPerPixel ;
  
    //info( "Skip bytes=%d, wastedIndicesPerRow=%d",wastedBytes, wastedIndicesPerRow ) ;
    for( int row = 0 ; row < height ; row++ )
    {
      for( int col = 0 ; col < width ; col++ )
      {
        // copy over all pixels, converting to standard vector(0,1) colors
        int texIndex = ( row * ( width + wastedIndicesPerRow ) ) + col ; 
        Vector& texel = colorValues[ index( row,col ) ] ;
      
        switch( stageTexDesc.Format )
        {
          case DXGI_FORMAT_R8G8B8A8_UNORM: // default 8-bit format
            {
              UINT pixel = ((UINT*)mappedSubRes.pData)[ texIndex ] ;
        
              // abgr
              int ialpha = (0xff000000 & pixel)>>24, iblue=(0x00ff0000 & pixel)>>16, igreen=(0x0000ff00 & pixel)>>8, ired=0x000000ff & pixel ;
        
              texel = Vector::fromByteColor( ired, igreen, iblue, ialpha ) ;
            }
            break ;
          case DXGI_FORMAT_R32G32B32A32_FLOAT:
            {
              float* pixel = ((float*)mappedSubRes.pData) + 4*texIndex ; 
              texel.x = pixel[0];
              texel.y = pixel[1];
              texel.z = pixel[2];
              texel.w = pixel[3];
            }
            break ;
          default:
            warning( "Unrecognized texture format, you get black" ) ;
            break ;
        }
      }
    }
    
    SAFE_RELEASE( stageTex ) ;
  }
  #pragma endregion

  // close it and reload it
  #pragma region load as srv
  D3DX11_IMAGE_LOAD_INFO loadInfo ;
  loadInfo.MiscFlags = 0;
  loadInfo.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
  loadInfo.CpuAccessFlags = 0 ;
  loadInfo.FirstMipLevel = 0 ;
  loadInfo.MipLevels = 1 ;
  loadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE ;

  DX_CHECK( D3DX11CreateTextureFromFileA( window->d3d, filename, &loadInfo, 0, (ID3D11Resource**)&tex, 0 ), "load texture" );
  
	D3D11_TEXTURE2D_DESC texDesc ;
	tex->GetDesc( &texDesc );
  
	D3D11_SHADER_RESOURCE_VIEW_DESC texRsvDesc ;
	texRsvDesc.Format = texDesc.Format;
	texRsvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D ;
	texRsvDesc.TextureCube.MipLevels = texDesc.MipLevels;
	texRsvDesc.TextureCube.MostDetailedMip = 0; //always 0
  
  //D3D11_TEX2D_SRV texSrv ;
  //texSrv.MipLevels = texDesc.MipLevels ;
  //texSrv.MostDetailedMip = 0 ;
  //texRsvDesc.Texture2D = texSrv ;
  
  DX_CHECK( window->d3d->CreateShaderResourceView( tex, &texRsvDesc, &shaderResView ), "create shader res view texture" ) ;
  #pragma endregion
}

D3D11Texture::~D3D11Texture()
{
  SAFE_RELEASE( texStage ) ;
  SAFE_RELEASE( tex ) ;
}

void D3D11Texture::generateNoise()
{
  // 3 offset values
  
  PerlinGenerator gen( PerlinNoiseType::Sky, 2, 1 ) ;
  for( int row = 0 ; row < height ; row++ )
  {
    for( int col = 0 ; col < width ; col++ )
    {
      Vector uv( (real)row/height, (real)col/width ) ;
      
      //Vector val = gen.marble( uv.x, uv.y, 0, uv.x ) ;
      //Vector val = gen.wood( uv.x, uv.y, 0 ) ;
      //printf( "x %f, y %f\n",x,y );
      Vector val = gen.at( uv ) ;

      int idx = index( row,col ) ;
      colorValues[ idx ] = val ;
      colorValues[ idx ].w = 1 ;
    }
  }

  // now put that out to the 2d tex
  //writeTex() ;
}

Vector D3D11Texture::px( int row, int col )
{
  int idx = index( row, col ) ;  

  if( idx > colorValues.size() )
  {
    error( "index %d out of range of CubeMap", idx ) ;
    return 0 ;
  }

  return colorValues[ idx ] ;
}

Vector D3D11Texture::px( real u, real v )
{
  // get the pixel CLOSEST to u and v
  clamp<real>( u, 0., (real)1. ) ;
  clamp<real>( v, 0., (real)1. ) ;

  // (this is the same code as in CubeMap, I copied it here).
  // need to mix neighbouring pixels.
  // fetch 
  // row=47.7
  // col=12.3
  
  //        11 | 12 | 13 |
  //    47     | Y  | y  |
  //       ----+----+----+
  //    48     | XX | Y  |
  //       ----+----+----+
  //    49     |    |    |
  //       ----+----+----+
  //
  
  ////////////// return px( face, (int)(row), (int)(col) ) ; //NON interpolated
  real col = u * width ;
  real row = v * height ;

  // CHECK THAT ROW, COL DON'T EXCEED MAX ALLOWABLE WIDTH, HEIGHT
  if( row >= height ) row = height-1 ; // this happens when sampling
  if( col >= width ) col = width-1 ; // this happens when sampling

  int frow = floor(row) ;
  int fcol = floor(col) ; // these will always be safe
  int crow = ceil( row ) ; // IF H < row, then use H-1, NOT the row. prevents OOB.
  int ccol = ceil( col ) ;

  if( crow >= height ) crow = height-1 ;
  if( ccol >= width ) ccol = width-1 ;
  // If you OOB, you're really on another FACE.
  // What I did here is make you sample the same pixel twice,

  // when you've reached the edge, ccol==col AND col==fcol,
  
  if( ccol == fcol && crow == frow ) // 
  {
    // single pixel (very corner)
    return px( fcol, frow ) ;
  }
  else if( ccol == fcol )
  {
    // so that means there's only T and B (no l & r)
    Vector T = px( frow, fcol ) ;
    Vector B = px( crow, fcol ) ;
    
    real wT = crow - row ;
    real wB = row - frow ;

    return wT*T + wB*B ;
  }
  else if( crow == frow )
  {
    Vector L = px( frow, fcol ) ;
    Vector R = px( frow, ccol ) ;
    
    real wL = ccol - col ;
    real wR = col - fcol ;

    return wL*L + wR*R ;
  }
  else
  {
    // general (centered) case
    Vector TL = px( frow, fcol ) ;
    Vector TR = px( frow, ccol ) ;
    Vector BL = px( crow, fcol ) ;
    Vector BR = px( crow, ccol ) ;

    real wTL = ( crow - row ) * ( ccol - col ) ;
    real wTR = ( crow - row ) * ( col - fcol ) ;
    real wBL = ( row - frow ) * ( ccol - col ) ;
    real wBR = ( row - frow ) * ( col - fcol ) ;

    return wTL*TL + wTR*TR + wBL*BL + wBR*BR ;
  }

}

void D3D11Texture::writeTex()
{
  // create the staging copy now
  D3D11_TEXTURE2D_DESC texDescStaging = texDesc ;
  texDescStaging.Usage = D3D11_USAGE_STAGING ;
  texDescStaging.BindFlags = 0 ;
  texDescStaging.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE ;
  DX_CHECK( window->d3d->CreateTexture2D( &texDescStaging, NULL, &texStage ), "Create staging texture" ) ;

  // use the staging tex to write to the texture
  // 0) CREATE the stage (which is deleted when this fcn is done,
  //    because we are NOT reading/writing every frame)
  // 1) map the stage,
  // 2) write to the stage
  // 3) copy the stage to the 'default' tex
  
  D3D11_MAPPED_SUBRESOURCE subres ;
  
  int bytesPerPixel = 4 ;

  // 1) map the staging resource as READ/WRITE..
  DX_CHECK( window->gpu->Map(
    texStage,
    D3D11CalcSubresource( 0,0,0 ),
    D3D11_MAP_READ_WRITE, // I only need to write to the stage,
    0,
    &subres ), "map stage tex" ) ;

  // 2) write to the stage, the color array, one __pixel__ at a time
  ByteColor* p = (ByteColor*)subres.pData ; // RGBA!!

  int skipBytes = subres.RowPitch - bytesPerPixel * width ;
  int skipPx = skipBytes / 4 ;

  for( int row = 0; row < height ; row++ )
  {
    for( int col = 0 ; col < width ; col++ )
    {
      int pStart = row * (width + skipPx) + col ;
      p[ pStart ] = colorValues[ index( row, col ) ].toByteColor() ;
    }
  }

  // unmap it!
  window->gpu->Unmap( texStage, D3D11CalcSubresource( 0,0,0 ) ) ;

  // 3) copy stage to default tex
  window->gpu->CopyResource( tex, texStage ) ;

  // delete this now
  SAFE_RELEASE( texStage ) ;
}

void D3D11Texture::save()
{
  // SAVE
  char fi[255];
  int len = sprintNow( fi ) ;
  sprintf( fi + len, "_TEXTURE.png" ) ;
  
  DX_CHECK( D3DX11SaveTextureToFileA(
    window->gpu,
    tex,
    D3DX11_IMAGE_FILE_FORMAT::D3DX11_IFF_PNG,
    fi ), "Saving texture as image file" ) ;


}

// bind the texture to the pipeline
void D3D11Texture::activate( int slotNo )
{
  if( !window->activeShader )
  {
    error( "No active shader" ) ;
    return ;
  }

  window->activeShader->setTexture( slotNo, shaderResView ) ;
}







