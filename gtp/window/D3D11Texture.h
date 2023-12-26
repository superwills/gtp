#ifndef D3D11TEXTURE_H
#define D3D11TEXTURE_H

#include <D3D11.h>
#include "../util/StdWilUtil.h"
#include "../math/perlin.h"

struct D3D11Texture
{
  ID3D11ShaderResourceView* shaderResView ;
  
  int width, height ;

  // must keep these.
  vector<Vector> colorValues ;
  
  D3D11_TEXTURE2D_DESC texDesc ;
  ID3D11Texture2D* tex ;
  ID3D11Texture2D* texStage ;
  
  D3D11Texture( int iWidth, int iHeight ) ;

  D3D11Texture( char* filename ) ;

  ~D3D11Texture() ;

  void generateNoise() ;

  inline int index( int row, int col ) {
    return row*width + col ;
  }

  Vector px( int row, int col ) ;
  Vector px( real u, real v ) ;

  // write colorValues to tex
  void writeTex() ;

  void save() ;

  // bind the texture to the pipeline
  void activate( int slotNo ) ;
} ;

#endif