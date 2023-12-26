#include "DirectWrite.h"
#include "D3D11Window.h"


DirectWrite::DirectWrite( IDXGIAdapter1* idxgiAdapter, ID3D11Texture2D *iSharedTex )
{
  dxgiAdapter = idxgiAdapter ;
  sharedTex = iSharedTex ;

  #pragma region d3d10.1 init
  // d3d10.1
  ID3D10Device1 * d3d101 ;

  bool ok = DX_CHECK( D3D10CreateDevice1(
    dxgiAdapter, // Use the same adapter as the d3d11 device
    D3D10_DRIVER_TYPE_HARDWARE, NULL,
    D3D10_CREATE_DEVICE_BGRA_SUPPORT | D3D10_CREATE_DEVICE_DEBUG, D3D10_FEATURE_LEVEL_10_1, D3D10_1_SDK_VERSION,
    &d3d101 ), "Creating d3d10.1 device" ) ;
  if( !ok )
    FatalAppExitA( 0, "Could not create d3d10.1 device!" ) ;
  #pragma endregion
  
  #pragma region get the dxgisurface and the dxgimutex handle for it
  // Now what we need is the IDXGISurface to the D3D11 texture.
  // D3D11 sharedTex ->(QUERYINTERFACE)-> IDXGIResource ->(D3D10 GETSHAREDHANDLE)-> HANDLE ->(OPENSHAREDRESOURCE)-> IDXGISurface
  
  // Get IDXGIRESOURCE from d3d11 sharedTex
  IDXGIResource *dxgiResource ;
  DX_CHECK( sharedTex->QueryInterface( &dxgiResource ), "Get dxgi resource interface to d3d11 tex" );

  // Get SHAREDHANDLE from IDXGIRESOURCE
  HANDLE dxgiSharedHandle ;
  DX_CHECK( dxgiResource->GetSharedHandle( &dxgiSharedHandle ), "Get shared handle on d3d11tex thru dxgi interface" ) ;
  SAFE_RELEASE( dxgiResource ) ;

  // Get IDXGISURFACE from SHAREDHANDLE
  IDXGISurface *dxgiSurface ;
  DX_CHECK( d3d101->OpenSharedResource( dxgiSharedHandle, __uuidof( IDXGISurface1 ), (void**)&dxgiSurface ), "Get dxgisurface to d3d11tex" ) ;
  d3d101->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP ) ;
  SAFE_RELEASE( d3d101 ) ; // I don't need the d3d10 interface anymore after this.
  // Its still alive, (thru the shared handle) but I don't need explicit access to it.

  /////////////////// GET THE KEYED MUTEX ////////////////////
  // Dieter: "Now, you need the IDXGIKeyedMutex handles for both the D3D11 and D3D10 versions of your shared resource.
  //          You will use these to lock the resource to which ever device is doing any read/write operations with the resource."
  // The KEYED MUTEX is from the dxgi surface
  DX_CHECK( dxgiSurface->QueryInterface( &dxgiKeyedMutex10 ), "Get keyed mutex 10" ) ;
  #pragma endregion

  #pragma region d2d init
  // Connect the IDXGISURFACE to the output of D2D
  // remember that the dxgiSurface goes THRU d3d10 to
  // the d3d11 tex underneath it all.
  D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
    D2D1_RENDER_TARGET_TYPE_DEFAULT, 
    D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
    96,96);
  
  DX_CHECK( D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory ), "Create d2d1 factory" );
  DX_CHECK( d2dFactory->CreateDxgiSurfaceRenderTarget(
    dxgiSurface, &props,
    &d2dRenderTarget ),
    "Create dxgi surface rt" ) ;
  SAFE_RELEASE( dxgiSurface ) ;  // dxgiSurface not used explicitly by our code after this line
  
  // Now create a brush on the render target.
  DX_CHECK( d2dRenderTarget->CreateSolidColorBrush( D2D1::ColorF(0,0,1), &d2dFontBrush ), "create solid brush" );

  DX_CHECK( d2dRenderTarget->CreateSolidColorBrush( D2D1::ColorF(1,1,1), &d2dDrawBrush ), "create solid brush" );

  // Create DirectWrite interface+fonts
  DX_CHECK( DWriteCreateFactory( DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwWriteFactory ), "Create dwrite factory" ) ;

  // Create a base font and change its properties
  createFont( 0, 18, 700, "Arial" ) ;
  #pragma endregion

  //vshader = new VSurface( "shader/vtc_dw_surface.cg", "vShader" ) ;
  //pxshader = new PxSurface( "shader/vtc_dw_surface.cg", "pxShader" ) ;
}

DirectWrite::~DirectWrite()
{
  SAFE_RELEASE( sharedTex ) ;
  SAFE_RELEASE( dxgiAdapter ) ;
  SAFE_RELEASE( dxgiKeyedMutex10 ) ;
  SAFE_RELEASE( d2dRenderTarget ) ;
  SAFE_RELEASE( d2dFontBrush ) ;
  SAFE_RELEASE( d2dDrawBrush ) ;

  // release all created fonts
  for( FontMapIter iter = fonts.begin() ; iter != fonts.end() ; ++iter )
    SAFE_RELEASE( iter->second ) ;
  fonts.clear() ;

  SAFE_RELEASE( d2dFactory ) ;      // release only when not making anymore geometry stuff or surfaces/render targets
  SAFE_RELEASE( dwWriteFactory );   // not making anymore fonts

  //DESTROY( vshader ) ;
  //DESTROY( pxshader ) ;

  

}

bool DirectWrite::createFont( int fontId, int height, int boldness, char* fontName )
{
  // see if a font by this id already exists
  if( fonts.find( fontId ) != fonts.end() )
  {
    error( "A font by id=%d already exists, nothin' doin'", fontId ) ;
    return false ;
  }

  IDWriteTextFormat* font ;
  wchar_t* wFontName = getUnicode(fontName) ;
  
  DX_CHECK( dwWriteFactory->CreateTextFormat(
    wFontName,
    NULL,
    (DWRITE_FONT_WEIGHT)boldness,
    DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STRETCH_NORMAL,
    height,
    TEXT("en-us"),
    &font
  ), "Create font" ) ;

  DX_CHECK( font->SetTextAlignment(DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING), "Set text alignment" ) ;
  DX_CHECK( font->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR), "set para align" ); // align to top of box
  DX_CHECK( font->SetLineSpacing( DWRITE_LINE_SPACING_METHOD::DWRITE_LINE_SPACING_METHOD_DEFAULT, height, 0.8*height ), "Set line spacing" ) ;

  fonts.insert( make_pair( fontId, font ) ) ;

  delete[] wFontName ;

  return true ;
}

IDWriteTextFormat* DirectWrite::getFont( int fontId )
{
  FontMapIter fontIter = fonts.find( fontId ) ;
  if( fontIter == fonts.end() )
  {
    error( "No font %d, using default font", fontId ) ;
    return fonts.at( 0 ) ;
  }
  else
    return fontIter->second ;
  
}

void DirectWrite::begin()
{
  // lock the sharedTex so d2d can draw on it
  DX_CHECK( dxgiKeyedMutex10->AcquireSync( 0, INFINITE ), "d2d acquire mutex on shared tex" ) ;
  
  d2dRenderTarget->BeginDraw();
  
  d2dRenderTarget->SetTransform(D2D1::IdentityMatrix());
  
  d2dRenderTarget->Clear(D2D1::ColorF(0,0,0,0));

}

void DirectWrite::end()
{
  DX_CHECK( d2dRenderTarget->EndDraw(), "d2d end draw" ) ;
  
  DX_CHECK( dxgiKeyedMutex10->ReleaseSync( 1 ), "d2d release mutex on shared tex" ) ; // because we released on 1,
  // the only bit of code that can pick up the mutex next
  // must pick up on 1
}

void DirectWrite::drawProgressBar( const ProgressBar & bar )
{
  // fill
  d2dDrawBrush->SetColor( D2D1::ColorF( D2D1::ColorF::Black, 0.5f ) ) ;
  d2dRenderTarget->FillRoundedRectangle(
    D2D1::RoundedRect( D2D1::RectF( bar.x, bar.y, bar.x + bar.w, bar.b() ), 4, 3 ),
    d2dDrawBrush
  ) ;

  // outside
  d2dDrawBrush->SetColor( D2D1::ColorF( bar.color.intValue, bar.color.a/255.0f ) ) ;
  d2dRenderTarget->DrawRoundedRectangle(
    D2D1::RoundedRect( D2D1::RectF( bar.x, bar.y, bar.r(), bar.b() ), 4, 3 ),
    d2dDrawBrush, 2.0f
  ) ;
  d2dRenderTarget->FillRoundedRectangle(
    D2D1::RoundedRect( D2D1::RectF( bar.x, bar.y, bar.x + bar.w*bar.percDone, bar.b() ), 4, 3 ),
    d2dDrawBrush
  ) ;

  draw( bar.fontId, bar.txt, ByteColor(255,255,255,255), bar.x+2, bar.y+2, bar.w, bar.h+2, 0 ) ;
}

// Need to add a BEGIN/END.
void DirectWrite::draw( int fontId,
  const char *str, ByteColor color,
  float x, float y, float boxWidth, float boxHeight,
  DWORD formatOptions )
{
  d2dFontBrush->SetColor( D2D1::ColorF( color.intValue, color.a/255.0f ) ) ;

  // Create a D2D rect that is the same size as the window.
  D2D1_RECT_F layoutRect = D2D1::RectF( x,y,x+boxWidth,y+boxHeight );

  // Use the DrawText method of the D2D render target interface to draw.
  wchar_t* txt = getUnicode( str ) ;
  UINT len = wcslen( txt ) ;

  IDWriteTextFormat* font = getFont( fontId ) ;
  
  DWRITE_TEXT_ALIGNMENT op ;
  if( formatOptions & DT_CENTER )  op = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_CENTER ;
  else if ( formatOptions & DT_RIGHT )  op = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_TRAILING ;
  else //if ( formatOptions & DT_LEFT )
    op = DWRITE_TEXT_ALIGNMENT::DWRITE_TEXT_ALIGNMENT_LEADING ;
  DX_CHECK( font->SetTextAlignment( op ), "Set text alignment" ) ;

  if( formatOptions & DT_VCENTER )
    DX_CHECK( font->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_CENTER), "set para align" );
  else
    DX_CHECK( font->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT::DWRITE_PARAGRAPH_ALIGNMENT_NEAR), "set para align" );

  d2dRenderTarget->DrawText(
    txt,         // The string to render.
    len,         // The string's length.
    font,        // The text format/font
    layoutRect,  // The region of the window where the text will be rendered.
    d2dFontBrush // The brush used to draw the text.
    );

  delete txt ;
}

