#ifndef DW_H
#define DW_H

#include <D3D11.h>

// DIRECTWRITE
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

#include <map>
using namespace std ;

#include "../threading/ProgressBar.h"

#pragma comment( lib, "d2d1.lib"  )
#pragma comment( lib, "dwrite.lib" )

//class VSurface ;
//class PxSurface ;

typedef map<int, IDWriteTextFormat*> /* as simply */ FontMap ;
typedef map<int, IDWriteTextFormat*>::iterator /* as simply */ FontMapIter ;

class DirectWrite
{
  ID3D11Texture2D *sharedTex ;

  // dxgi
  IDXGIAdapter1 *dxgiAdapter ;
  IDXGIKeyedMutex *dxgiKeyedMutex10 ;

  // d2d
  // I need these for the life of the program
  // to render text
  ID2D1RenderTarget* d2dRenderTarget;
  ID2D1SolidColorBrush *d2dFontBrush, *d2dDrawBrush;
  
  FontMap fonts ;

  ID2D1Factory* d2dFactory;  // geometry stuff + surfaces/render targets
  IDWriteFactory* dwWriteFactory; // used for creating fonts

public:
  //VSurface * vshader ;
  //PxSurface * pxshader ;

  // To create a DirectWrite object, you must
  // pass a DXGI adapter to use, and a d3d11 texture to attach to
  DirectWrite( IDXGIAdapter1* idxgiAdapter, ID3D11Texture2D *iSharedTex ) ;
  ~DirectWrite() ;

  bool createFont( int fontId, int height, int boldness, char* fontName ) ;

  void begin() ;
  void end() ;

  void drawProgressBar( const ProgressBar & bar ) ;

  void draw( int fontId,
    const char *str, ByteColor color,
    float x, float y, float boxWidth, float boxHeight,
    DWORD formatOptions ) ;


private:
  IDWriteTextFormat* getFont( int fontId ) ;
} ;



#endif
