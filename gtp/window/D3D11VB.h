#ifndef D3D11VB_H
#define D3D11VB_H

#include <D3D11.h>

class D3D11Window ;

// All you can do with a VertexBuffer once
// its made is draw it.
// Later you may be able to modify it.
class VertexBuffer abstract
{
public:
  virtual void drawTris() PURE ;
  virtual void drawLines() PURE ;
  virtual void drawPoints() PURE ;
} ;

template <typename T>
class TVertexBuffer : public VertexBuffer
{
  int numVertices, numIndices ;
  ID3D11Buffer * vb ; // http://msdn.microsoft.com/en-us/library/ff476899(VS.85).aspx
  ID3D11Buffer * ib ;

  // every VB needs a suitable vertexLAYOUT to
  // be able to draw itself
  ////ID3D11InputLayout *layout ;  // this can be a static member for
  // all CLASSES of type <T> (this is not actually used and is controlled by the calling code

  // could be a static member
  D3D11Window *win ; // the d3d11 window instance

public:
  TVertexBuffer( D3D11Window* iWin, const T* iVBData, int iNumVertices )
  {
    win = iWin ;
    vb = 0 ;
    ib = 0 ;

    numVertices = iNumVertices ;
    numIndices  = 0 ;

    D3D11_BUFFER_DESC vbDesc = { 0 } ;
    vbDesc.ByteWidth      = numVertices * sizeof( T ) ;
    vbDesc.Usage          = D3D11_USAGE_DYNAMIC ;      // DEFAULT, IMMUTABLE, DYNAMIC, STAGING. http://msdn.microsoft.com/en-us/library/ff476259(VS.85).aspx
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE ;   // tied to the .Usage field. Can make the buffer cpu readable or writable if you want
    vbDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER ; // is it a vertex buffer? an index buffer? a render target?
    vbDesc.MiscFlags      = 0 ;                        // Things like D3D11_RESOURCE_MISC_GENERATE_MIPS and D3D11_RESOURCE_MISC_TEXTURECUBE
    
    // Wrap the vertex data in a D3D11_SUBRESOURCE_DATA structure.
    D3D11_SUBRESOURCE_DATA vbData = { 0 } ;
    vbData.pSysMem = iVBData ;

    // Now create the vertex buffer
    DX_CHECK( win->d3d->CreateBuffer( &vbDesc, &vbData, &vb ), "Create vb" ) ;
  }

  TVertexBuffer( D3D11Window* iWin, const T* iVBData, int iNumVertices, unsigned int* iIndexData, int iNumIndices )
  {
    win = iWin ;
    vb = 0 ;
    ib = 0 ;

    numVertices = iNumVertices ;
    numIndices = iNumIndices ;
    
    D3D11_BUFFER_DESC vbDesc = { 0 } ;
    vbDesc.ByteWidth      = numVertices * sizeof( T ) ;
    // Need the vb's to be updateable
    vbDesc.Usage          = D3D11_USAGE_DYNAMIC ;           // DEFAULT, IMMUTABLE, DYNAMIC, STAGING. http://msdn.microsoft.com/en-us/library/ff476259(VS.85).aspx
    // Usage:    Default  Dynamic  Immutable  Staging
    // GPU-Read      yes      yes        yes     yes1
    // GPU-Write     yes                         yes1
    // CPU-Read                                  yes1
    // CPU-Write              yes                yes1
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE ;        // tied to the .Usage field. Can make the buffer cpu readable or writable if you want
    vbDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER ; // is it a vertex buffer? an index buffer? a render target?
    vbDesc.MiscFlags      = 0 ;                        // Things like D3D11_RESOURCE_MISC_GENERATE_MIPS and D3D11_RESOURCE_MISC_TEXTURECUBE
    
    // Wrap the vertex data in a D3D11_SUBRESOURCE_DATA structure.
    D3D11_SUBRESOURCE_DATA vbData = { 0 };
    vbData.pSysMem = iVBData ;

    // Now create the vertex buffer
    DX_CHECK( win->d3d->CreateBuffer( &vbDesc, &vbData, &vb ), "Create vb" ) ;

    D3D11_BUFFER_DESC ibDesc = { 0 };
    ibDesc.ByteWidth = numIndices * sizeof( unsigned int ) ;
    ibDesc.Usage = D3D11_USAGE_DEFAULT ; // NOT updateable.
    ibDesc.CPUAccessFlags = 0 ;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER ;
    ibDesc.MiscFlags = 0 ;

    D3D11_SUBRESOURCE_DATA ibData = { 0 } ;
    ibData.pSysMem = iIndexData ;

    // Now create the vertex buffer
    DX_CHECK( win->d3d->CreateBuffer( &ibDesc, &ibData, &ib ), "Create ib" ) ;
  }

  ~TVertexBuffer()
  {
    SAFE_RELEASE( vb ) ;
    SAFE_RELEASE( ib ) ;
  }

  int getNumVerts() { return numVertices ; }
  int getNumIndices() { return numIndices ; }

  void drawTris() override
  {
    if( ib )
      drawIndexedTris() ;
    else
      drawNonindexedTris() ;
  }

  void drawLines() override
  {
    bind() ;
    win->gpu->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );
    win->gpu->Draw( numVertices, 0 ); // numVertices, startVertex
  }

  void drawPoints() override
  {
    bind() ;
    win->gpu->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
    win->gpu->Draw( numVertices, 0 ); // numVertices, startVertex
  }

  void drawTriangleStrip()
  {
    bind() ;
    win->gpu->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
    win->gpu->Draw( numVertices, 0 ); // numVertices, startVertex
  }

  void bind()
  {
    UINT strides[1] ;  UINT offsets[1] ;  ID3D11Buffer * buffers[1] ;
    
    strides[ 0 ] = sizeof( T ) ;
    offsets[ 0 ] = 0 ;
    buffers[ 0 ] = vb ;

    win->gpu->IASetVertexBuffers(
      0,        // StartSlot - first input slot for binding. max=32 for d3d11
      1,        // # vertex buffers in the array that you're going to draw now
      buffers,  // the array of vertex buffers
      strides,  // the stride value for each entry in pBuffers
      offsets   // array of offset values - bytes to SKIP before first element in pBuffers[i]
    );
  }

  void drawNonindexedTris()
  {
    bind() ;
    win->gpu->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    win->gpu->Draw( numVertices, 0 ); // numVertices, startVertex
  }

  void drawIndexedTris()
  {
    bind() ;
    win->gpu->IASetIndexBuffer( ib, DXGI_FORMAT_R32_UINT, 0 ) ;  // c'mon, how many bytes are we talking about here.
    // even in the upper limit (~65k faces), you still only saved 128k BYTES.
    // while the vertex data would take 2.6 MB.

    win->gpu->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    win->gpu->DrawIndexed( numIndices, 0, 0 ) ;
  }

  void* lockVB()
  {
    if( !vb )
    {
      error( "No vb" ) ;
      return NULL ;
    }
    D3D11_MAPPED_SUBRESOURCE mapped ;
    if( DX_CHECK( win->gpu->Map( vb, 0,
      ///D3D11_MAP_WRITE, //!! ILLEGAL WITH DYNAMIC TEXTURES:  "D3D11_USAGE_DYNAMIC Resources must use either MAP_WRITE_DISCARD or MAP_WRITE_NO_OVERWRITE with Map."
      D3D11_MAP::D3D11_MAP_WRITE_DISCARD, ///!! THROWS AWAY __ALL__ PREVIOUS DATA!
      // The resource must have been created with write access and dynamic usage (See D3D11_CPU_ACCESS_WRITE and D3D11_USAGE_DYNAMIC).
    0, &mapped ), "Lock vb" ) )
      return mapped.pData ;
    else
      return 0 ;
  }

  void unlockVB()
  {
    win->gpu->Unmap( vb, 0 ) ;
  }
} ;

//template <typename T> D3D11Window* VertexBuffer<T>::win = 0 ; // not static b/c its too easy
// to forget to init it for EVERY CLASS that uses it

#endif

