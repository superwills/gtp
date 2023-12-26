#include "D3DDrawingVertex.h"
#include "../math/SHVector.h"
#include "../scene/Material.h"

char* ColorIndexName[] =
{
  "DiffuseMaterial",
  "Emissive",
  "RadiosityComp",
  "SHComputed",
  "WaveletComputed",
  "SpecularMaterial",
  "Transmissive"
} ;

char* TexcoordIndexName[] =
{
  "Decal",
  "RadiosityID",
  "VectorOccludersIndex",
  "SHTexIndex"
} ;

AllVertex::AllVertex():
  shDiffuseAmbient(0),shDiffuseInterreflected(0), shDiffuseSummed(0),
  shSpecularMatrix(0),shSpecularMatrixInterreflected(0),
  ambientOcclusion(1.0),voData(0)
{}

AllVertex::AllVertex( const AllVertex& other ):
  shDiffuseAmbient(0), shDiffuseInterreflected(0), shDiffuseSummed(0),
  shSpecularMatrix(0),shSpecularMatrixInterreflected(0),
  ambientOcclusion(1.0),voData(0)
{
  pos = other.pos;
  norm=other.norm;
  for( int i = 0 ; i < NumColors ; i++ )
    color[i] = other.color[i] ;
  for( int i = 0 ; i < NumTexcoords ; i++ )
    texcoord[i] = other.texcoord[i] ;
  if( other.shDiffuseAmbient )  shDiffuseAmbient = new SHVector( *other.shDiffuseAmbient ) ;
  if( other.shDiffuseInterreflected )  shDiffuseInterreflected = new SHVector( *other.shDiffuseInterreflected ) ;
  if( other.shDiffuseSummed ) shDiffuseSummed = new SHVector( *other.shDiffuseSummed ) ;
  if( other.shSpecularMatrix ) shSpecularMatrix = new SHMatrix( *other.shSpecularMatrix ) ;
  if( other.shSpecularMatrixInterreflected ) shSpecularMatrixInterreflected = new SHMatrix( *other.shSpecularMatrixInterreflected ) ;
}

AllVertex::AllVertex( const Vector& iPos, const Vector& iNorm, const Material& mat ) :
  pos(iPos),norm(iNorm),
  shDiffuseAmbient(0),shDiffuseInterreflected(0),shDiffuseSummed(0),
  shSpecularMatrix(0),shSpecularMatrixInterreflected(0),
  ambientOcclusion(1.0),voData(0)
{
  setMaterial( mat ) ;

  // give everything junk values
  //for( int i = 0 ; i < NumColors ; i++ )
  //  color[i] = Vector::random() ;
  //for( int i = 0 ; i < NumTexcoords ; i++ )
  //  texcoord[i] = Vector::random() ;
}

AllVertex::AllVertex( const Vector& iPos, const Vector& iNorm, const Vector& diffuse, const Vector& emissive, const Vector& specular ) :
  pos(iPos),norm(iNorm),
  shDiffuseAmbient(0),shDiffuseInterreflected(0),shDiffuseSummed(0),
  shSpecularMatrix(0),shSpecularMatrixInterreflected(0),
  ambientOcclusion(1.0),voData(0)
{
  color[ ColorIndex::DiffuseMaterial ] = diffuse ;
  color[ ColorIndex::Emissive ] = emissive ;
  color[ ColorIndex::SpecularMaterial ] = specular ;
}

AllVertex::~AllVertex()
{
  DESTROY( shDiffuseAmbient );
  DESTROY( shDiffuseInterreflected ) ;
  DESTROY( shDiffuseSummed ) ;
  DESTROY( shSpecularMatrix ) ;
  DESTROY( shSpecularMatrixInterreflected ) ;
}

AllVertex& AllVertex::operator+=( const AllVertex& other )
{
  pos += other.pos ;
  norm += other.norm ;

  for( int i = 0 ; i < NumColors ; i++ )
    color[i] += other.color[i] ;
  for( int i = 0 ; i < NumTexcoords ; i++ )
    texcoord[i] += other.texcoord[i] ;
    
  if( shDiffuseAmbient )  shDiffuseAmbient->add( other.shDiffuseAmbient ) ;
  if( shDiffuseInterreflected )  shDiffuseInterreflected->add( other.shDiffuseInterreflected ) ;
  if( shDiffuseSummed )  shDiffuseSummed->add( other.shDiffuseSummed ) ;
  if( shSpecularMatrix )  shSpecularMatrix->add( other.shSpecularMatrix ) ;
  if( shSpecularMatrixInterreflected )  shSpecularMatrixInterreflected->add( other.shSpecularMatrixInterreflected ) ;
  return (*this) ;
}

AllVertex& AllVertex::operator-=( const AllVertex& other )
{
  pos -= other.pos ;
  norm -= other.norm ;

  for( int i = 0 ; i < NumColors ; i++ )
    color[i] -= other.color[i] ;
  for( int i = 0 ; i < NumTexcoords ; i++ )
    texcoord[i] -= other.texcoord[i] ;
    
  if( shDiffuseAmbient )  shDiffuseAmbient->sub( other.shDiffuseAmbient ) ;
  if( shDiffuseInterreflected )  shDiffuseInterreflected->sub( other.shDiffuseInterreflected ) ;
  if( shDiffuseSummed ) shDiffuseSummed->sub( other.shDiffuseSummed ) ;
  if( shSpecularMatrix )  shSpecularMatrix->sub( other.shSpecularMatrix ) ;
  if( shSpecularMatrixInterreflected )  shSpecularMatrixInterreflected->sub( other.shSpecularMatrixInterreflected ) ;
  return (*this) ;
}

AllVertex& AllVertex::operator*=( real scale )
{
  pos *= scale ;
  norm *= scale ;

  for( int i = 0 ; i < NumColors ; i++ )
    color[i] *= scale ;
  for( int i = 0 ; i < NumTexcoords ; i++ )
    texcoord[i] *= scale ;
    
  if( shDiffuseAmbient )  shDiffuseAmbient->scale( scale ) ;
  if( shDiffuseInterreflected )  shDiffuseInterreflected->scale( scale ) ;
  if( shDiffuseSummed ) shDiffuseSummed->scale( scale ) ;
  if( shSpecularMatrix )  shSpecularMatrix->scale( scale ) ;
  if( shSpecularMatrixInterreflected )  shSpecularMatrixInterreflected->scale( scale ) ;
  return (*this) ;
}

int AllVertex::save( FILE* f )
{
  // write 6 vectors
  int written = 0 ;
  
  written += sizeof(Vector)*fwrite( this, sizeof(Vector), NumElementsTotal, f ) ;

  // write the shprojections
  if( shDiffuseAmbient )
  {
    written += shDiffuseAmbient->save( f ) ;
  }
  else
  {
    //warning( "AllVertex::write():  no shDiffuseAmbient to write" ) ;
  }
  // don't care about shDiffuseInterreflected

  return written ;
}

void AllVertex::load( FILE * f, int numCoeffs )
{
  fread( this, sizeof(Vector), NumElementsTotal, f );

  if( !numCoeffs )
    shDiffuseAmbient = 0 ; // there is no SHPROJ
  else
    shDiffuseAmbient = new SHVector( f, numCoeffs ) ;
}

void AllVertex::setMaterial( const Material& mat )
{
  // the stuff we want from the Material is:
  color[ ColorIndex::DiffuseMaterial ] = mat.kd ;
  color[ ColorIndex::Emissive ] = mat.ke ;
  color[ ColorIndex::SpecularMaterial ] = mat.ks ;
  color[ ColorIndex::SpecularMaterial ].w = mat.Ns ; // Save the NS term here.
  color[ ColorIndex::Transmissive ] = mat.kt ; // need this for sh tracing caustics
}

AllVertex operator*( AllVertex v, real scale )
{
  // v is a copy
  v *= scale ;
  return v ;
}

AllVertex operator*( real scale, AllVertex v )
{
  // v is a copy
  v *= scale ;
  return v ;
}

AllVertex operator+( const AllVertex& a, const AllVertex& b )
{
  AllVertex c = a;
  c+=b;
  return c;
}

AllVertex operator-( const AllVertex& a, const AllVertex& b )
{
  AllVertex c=a;
  c-=b;
  return c ;
}