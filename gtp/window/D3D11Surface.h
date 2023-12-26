#ifndef D3D11SURFACE_H
#define D3D11SURFACE_H

#include <D3D11.h>
#include "../util/StdWilUtil.h"
#include "ISurface.h"
#include "D3DDrawingVertex.h"
#include "D3D11VB.h"

class D3D11Window ;


struct Texcoording
{
  int n, row, col, startingRow, startingCol ;
  int accum ;
  int totalAccum ; // total accumulated since start, never resets

  Texcoording( int texNSize ) : n(texNSize),row(0),col(0),startingRow(0),startingCol(0),accum(0),totalAccum(0){}
  
  void next()
  {
    col++ ;
    accum++ ;
    totalAccum++ ;
    if( col >= n )  // when col gets to 1024..
    {
      row++ ;   // move to the next row
      col = 0 ; // reset column index
    }
    if( row >= n )  // if ROW gets to 1024..
    {
      error( "Out of texcoords, overwriting beginning (as a safety)" ) ;
      row = 0 ; // col will already have been set to 0
    }
  }

  void start()
  {
    accum = 0 ;
    startingRow = row ;
    startingCol = col ;
  }
} ;

/// These are used for each
/// of the 5 sides of the
/// hemicube
struct D3D11Surface : public ISurface
{
private:
  D3D11Window *win ; // this should be static
  ID3D11ShaderResourceView* shaderResView ;
  int bpp ; // BYTES per pixel, not bits.

public:
  ID3D11Texture2D *tex, *texStage ; // to patch over d3d11's texture difficulties,
  // you actually have to stage a DYNAMIC texture to read it,
  // and you have to stage a DEFAULT texture to EITHER write or read it.

  ID3D11Texture2D *depthTex, *depthTexStage ; 

  /// The surface texture used
  /// to render to this surface
  ID3D11RenderTargetView *rtv ;
  ID3D11DepthStencilView *dsv ;

  const static float clearColor[4] ;

  D3D11_VIEWPORT viewport ;

  // we need to remember what type of texture it was
  D3D11_TEXTURE2D_DESC texDesc ;

  // used only for non-rendertarget surfaces  
  D3D11_MAPPED_SUBRESOURCE subres ;
  D3D11_MAPPED_SUBRESOURCE subresDepth ;
  
  TVertexBuffer<VertexTC> *vb ; // I know I need TC verts

  SurfaceType surfaceType ;

  // the preferred/default slot for this texture.
  // as the programmer you need to keep control/track
  // of what slot you expect a certain texture to be
  // bound to.  the shader program needs to know
  // what id to sample.
  int slot ;

public:
  // requires a GPU since it needs to make a d3d texture
  D3D11Surface( D3D11Window* iWin,
    int ix, int iy,
    int iWidth, int iHeight,
    real zNear, real zFar,
    real zLevelForDrawing,
    SurfaceType iSurfaceType,
    int slotId, DXGI_FORMAT texFormat, int ibpp ) ;

  ~D3D11Surface() ;

  // to read pixel values we must "lock" the surface.
  // what we'll do is require an "open" and "close"
  // in the interface
  // unfortunately for d3d11, this is flawed.
  // d3d11 has many texture types, and MAP only
  // works for DYNAMIC and STAGING types.
  // IE you should NOT use MAP 
  BYTE* open() override ;
  float* openDepthTex() ;

  void close() override ;
  void closeDepthTex() ;

  bool isOpened() override ;
  
  // If the surface IS dynamic, then you
  // get the colorptr back (else returns null)
  ByteColor* clear( ByteColor toColor ) ;
  void draw() override ;

  int getWidth() const override ;
  int getHeight() const override ;
  int getNumPixels() const override ;

  int getPixelByteIndex( int row, int col ) const ;

  // function to get a pointer to the color buffer
  // of this surface.
  ByteColor* getColorAt( int pxNum ) override ;
  ByteColor* getColorAt( int row, int col ) override ;

  float* getDepthAt( int row, int col ) ;

  void setColorAt( int row, int col, ByteColor val ) override ;

  void setColorAt( int row, int col, USHORT h1, USHORT h2 ) override ;

  void setColorAt( int row, int col, const Vec3& color ) override ;

  void setColorAt( Texcoording& ctex, const Vec3& color ) ;

  void setColorAt( Texcoording& ctex, const Vector& color ) ;

  // byte alignment "feature" of d3d11 is that
  // there are RowPitch - bpp*cols wasted bytes
  // at the end of each row
  int getWastedBytesAtEndOfEachRow() const ;

  int getWastedPxAtEndOfEachRow() const ;

  int getPxPerRow() ;

  void copyFromByteArray( BYTE * arrSrc, int bytesPerPixel, int numPixels ) override ;

  // make this the active texture
  void activate() override ;

  // unset this as the active texture
  void deactivate() override ;


  void rtBind() override ;
  void rtUnbind() override ;
  void rtBindMainscreen() override ;

  bool save( char* filename ) override ;

  // saves to a DDS
  void saveDDS( char* filename ) override ;

  // saves jpg
  void saveJPG( char* filename ) override ;

  void setAlpha( float alpha ) ;


} ;

#endif

