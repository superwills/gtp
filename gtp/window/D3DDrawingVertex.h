#ifndef D3DDRAWINGVERTEX_H
#define D3DDRAWINGVERTEX_H

#include "../math/Vector.h"

struct Material ;

struct Vec3{
  float x,y,z;
  Vec3():x(0),y(0),z(0){}
  Vec3( const Vector& v ) : x(v.x), y(v.y), z(v.z){}
  Vec3( float ix, float iy, float iz ):x(ix),y(iy),z(iz) {}
  Vector getVector() const { return Vector(x,y,z); }
};
struct Vec4{
  float x,y,z,w;
  Vec4():x(0),y(0),z(0),w(0){}
  Vec4( const Vector& v ) : x(v.x), y(v.y), z(v.z), w(v.w){} // allows INITIALIZATION only
  Vec4( float ix, float iy, float iz, float iw ):x(ix),y(iy),z(iz),w(iw) {}
  Vector getVector() const { Vector v(x,y,z); v.w=w; return v; }
};


// I thought of having VertexC : public Vertex but then I realized this creates potential
// confusion for people to do with assumptions about it being "ok" to pass in a VertexC when
// a Vertex is called for... because in inheritance, you can do that.  Having separate,
// concrete, fully defined objects here makes the most sense.  These really are
// distinct, non-interchangeable objects, as far as the GPU is concerned.



#pragma region used for obj loading, not shaders
// v
struct Vertex
{
  Vec3 pos ;

  Vertex() { }

  Vertex( float x, float y, float z ):
    pos(x,y,z)
  {
  }

  Vertex( const Vector &iPos ):
    pos( iPos )
  {
  }
} ;

struct VertexC
{
  Vec3 pos ;
  Vec4 color ;

  VertexC():color( 1,1,1,1 )
  {
  }

  VertexC( const Vector& iPos, const Vector& iColor ) :
    pos( iPos ), color( iColor )
  {
  }
} ;

// v//n
struct VertexN
{
  Vec3 pos ;
  Vec3 norm ;

  VertexN() { }

  VertexN( float x, float y, float z, float nx, float ny, float nz ) :
    pos(x,y,z),
    norm(nx,ny,nz)
  {
  }

  VertexN( const Vector &iPos, const Vector &iNorm ) :
    pos( iPos ),
    norm( iNorm )
  {
  }
} ;

// v/t
struct VertexT
{
  Vec3 pos ;
  Vec4 tex ;

  VertexT() { }

  VertexT( const Vector &iPos, const Vector &iTex ):
  pos(iPos), tex(iTex)
  {
  }
} ;

// v/t/n
struct VertexTN
{
  Vec3 pos ;
  Vec4 tex ;
  Vec3 norm ;

  // Preferred this format over memsets, they are hard to read :
  VertexTN() { }

  VertexTN( const Vector &iPos, const Vector &iTex, const Vector &iNorm ) :
  pos(iPos), tex(iTex), norm(iNorm)
  {
  }
} ;
#pragma endregion



#pragma region shader formats
// v/n/c
struct VertexNC
{
  Vec3 pos ;
  Vec3 norm ;
  Vec4 color ;

  VertexNC() { }

  VertexNC( float x, float y, float z, float nx, float ny, float nz ) :
    pos(x,y,z),
    norm(nx,ny,nz),
    color( 1,1,1,1 )
  {
  }

  VertexNC( const Vector &iPos, const Vector &iNorm, const Vector& iColor ):
  pos(iPos), norm(iNorm), color(iColor)
  {
  }
} ;

struct VertexTC
{
  Vec3 pos ;
  Vec4 tex ;
  Vec4 color ;

  VertexTC()
  {
  }

  VertexTC( const Vector& iPos,
    const Vector& iTex, 
    const Vector& iColor ) :
    pos( iPos ),
    tex( iTex ),
    color( iColor )
  {
    
  }
} ;

struct VertexTTTTNC
{
  Vec3 pos ;
  Vec4 tex[ 4 ] ;
  Vec3 norm ;
  Vec4 color ;

  VertexTTTTNC(){}

  VertexTTTTNC( const Vector& iPos,
    const Vector& iTex0,const Vector& iTex1,const Vector& iTex2,const Vector& iTex3,
    const Vector& iNorm,
    const Vector& iColor ) :
      pos(iPos),
      norm( iNorm ),
      color( iColor )
  {
    tex[0]=(iTex0); tex[1]=(iTex1); tex[2]=(iTex2); tex[3]=(iTex3);
  }
} ;

// fully loaded v/tttt/n/cccc
struct VertexTTTTNCCCC
{
  Vec3 pos ;
  Vec4 tex[ 4 ] ;
  Vec3 norm ;
  Vec4 color[ 4 ] ;

  VertexTTTTNCCCC(){}

  VertexTTTTNCCCC( const Vector& iPos,
    const Vector& iTex0,const Vector& iTex1,const Vector& iTex2,const Vector& iTex3,
    const Vector& iNorm,
    const Vector& iColor0,const Vector& iColor1,const Vector& iColor2,const Vector& iColor3 ) :
      pos(iPos),
      norm( iNorm )
  {
    tex[0]=(iTex0); tex[1]=(iTex1); tex[2]=(iTex2); tex[3]=(iTex3);
    color[0]=( iColor0 );color[1]=( iColor1 );color[2]=( iColor2 );color[3]=( iColor3 );
  }
} ;

struct VertexT10NC10
{
  Vec3 pos ;
  Vec4 tex[ 10 ] ;
  Vec3 norm ;
  Vec4 color[ 10 ] ;

  VertexT10NC10() { }

  VertexT10NC10( const Vector& iPos,
    const Vector& iTex0,const Vector& iTex1,const Vector& iTex2,const Vector& iTex3,const Vector& iTex4,
    const Vector& iTex5,const Vector& iTex6,const Vector& iTex7,const Vector& iTex8,const Vector& iTex9,
    const Vector& iNorm,
    const Vector& iColor0,const Vector& iColor1,const Vector& iColor2,const Vector& iColor3,const Vector& iColor4,
    const Vector& iColor5,const Vector& iColor6,const Vector& iColor7,const Vector& iColor8,const Vector& iColor9 ) :
      pos(iPos),
      norm( iNorm )
  {
    tex[0]=iTex0; tex[1]=iTex1; tex[2]=iTex2; tex[3]=iTex3; tex[4]=iTex4;
    tex[5]=iTex5; tex[6]=iTex6; tex[7]=iTex7; tex[8]=iTex8; tex[9]=iTex9;
    color[0]=iColor0;color[1]=iColor1;color[2]=iColor2;color[3]=iColor3;color[4]=iColor4;
    color[5]=iColor5;color[6]=iColor6;color[7]=iColor7;color[8]=iColor8;color[9]=iColor9;
  }

  VertexT10NC10( const Vector& iPos, const Vector* iTexesArray, const Vector& iNorm, const Vector* iColorsArray ):
    pos( iPos ), norm( iNorm )
  {
    for( int i = 0 ; i < 10 ; i++ ){
      tex[i] = iTexesArray[i] ;
      color[i] = iColorsArray[i] ;
    }
  }
} ;

template <size_t n>
struct VertexTNC
{
  Vec3 pos ;
  Vec4 tex[ n ] ;
  Vec3 norm ;
  Vec4 color[ n ] ;
  VertexTNC(){}
  VertexTNC( const Vector& iPos, Vector iTexs[n], const Vector& iNorm, Vector iColors[n] )
  {
    pos = iPos ;
    norm = iNorm ;
    for( int i = 0 ; i < n ; i++ )
    {
      tex[i]=iTexs[i];
      color[i]=iColors[i];
    }
  }
} ;

#pragma endregion

class SHVector ;
struct SHMatrix ;
struct CubeMap ;
struct VizFunc ;
struct VectorOccludersData ;

extern char* ColorIndexName[] ;
extern char* TexcoordIndexName[] ;

// name the color indices
enum ColorIndex{
  
  DiffuseMaterial, // the base diffuse material color
  Emissive,        // base emissive color.
  
  SpecularMaterial,

  // the transmissive material color
  Transmissive,

  RadiosityComp,   // radiosity computation result (ss final exitance)
  //////VectorOccludersRemoteColor, // the weighted average diffuse color of remote patches.

  SHComputed,  // SH computed sum color, updated by CPU when light angle changes.
  WaveletComputed      // the wavelet computed color.
  
} ;

enum TexcoordIndex{
  Decal, //tex0
  RadiosityID,   // radiosity patch id.
  VectorOccludersIndex, //tex2: index into interrefln tex, which has further information
  SHTexIndex
} ;

struct AllVertex
{
  const static int NumColors=10, NumTexcoords=10;
  const static int NumElementsTotal = 1 + NumTexcoords + 1 + NumColors ; // P + 4T + N + 4C

  Vector
    pos,        // space position
    texcoord[NumTexcoords],
    norm,       // normal
    color[NumColors] ;

  
  SHVector *shDiffuseAmbient,  // diffuse ambient occlusion map, projected to sh.
           *shDiffuseInterreflected,  // adds in diffuse interreflections from neighbouring geometry.
           *shDiffuseSummed ; // sum of ambient + interreflected
  
  SHMatrix *shSpecularMatrix,
           *shSpecularMatrixInterreflected ;

  // Cubemap approach.  The contents of the 
  // cubeMap ptr is like, pixels.
  CubeMap *cubeMap ;

  VectorOccludersData *voData ;

  // The RAW visibility function, which
  // can then be changed into a cubemap or SH function
  VizFunc *vizFunc ;
  
  // the ambient occlusion is % "open" (between 0 and 1)
  // it is found by casting rays in a hemisphere about the vertex,
  real ambientOcclusion ; // mirrored in (texcoord[VectorOccludersIndex].w)

  AllVertex() ;

  AllVertex( const AllVertex& other ) ;
  
  AllVertex( const Vector& iPos, const Vector& iNorm, const Material& mat ) ;
  
  AllVertex( const Vector& iPos, const Vector& iNorm, const Vector& diffuse, const Vector& emissive, const Vector& specular ) ;

  ~AllVertex();

  Vertex v()
  {
    return Vertex( pos ) ;
  }
  VertexN vn()
  {
    return VertexN( pos, norm ) ;
  }
  VertexNC vnc()
  {
    return VertexNC( pos, norm, color[ ColorIndex::DiffuseMaterial ] ) ;
  }
  VertexTTTTNC vttttnc()
  {
    return VertexTTTTNC( pos, texcoord[0], texcoord[1], texcoord[2], texcoord[3], norm, color[ ColorIndex::DiffuseMaterial ] ) ;
  }
  VertexTTTTNCCCC vttttncccc()
  {
    return VertexTTTTNCCCC( pos, texcoord[0], texcoord[1], texcoord[2], texcoord[3], norm, color[0], color[1], color[2], color[3] ) ;
  }

  VertexT10NC10 vt10nc10()
  {
    return VertexT10NC10( pos, texcoord, norm, color ) ;
  }

  AllVertex& operator+=( const AllVertex& other ) ;
  AllVertex& operator-=( const AllVertex& other ) ;
  AllVertex& operator*=( real scale ) ;

  int save( FILE* f ) ;
  void load( FILE* f, int numCoeffs ) ;

  void setMaterial( const Material& mat ) ;

} ;

AllVertex operator*( AllVertex v, real scale ) ;
AllVertex operator*( real scale, AllVertex v ) ;

AllVertex operator+( const AllVertex& a, const AllVertex& b ) ;
AllVertex operator-( const AllVertex& a, const AllVertex& b ) ;







#endif