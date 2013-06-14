#include "SHVector.h"
#include "SHSample.h"
#include "../window/GTPWindow.h"
#include "../geometry/Model.h"

// An "SH projection" is the collection of
// a function's similarity with the "samples" (in class SH)
// The default constructor uses the same number of bands in
// window->shSamps->bands, because that number is used everywhere
// program wide, and _should not_ be different anywhere in this code.
SHVector::SHVector()
{
  allocate() ;
}

#if 0
SHVector::SHVector( const SHVector& other )
{
  info( "Copying sh vector" ) ;
  bands = other.bands; 
  coeffsScale = other.coeffsScale;
}

SHVector::SHVector( function<Vector (real tElevation, real pAzimuth)> funcToProject )
{
  allocate() ;
  project( funcToProject ) ;
}

SHVector::SHVector( Shape* shapeToProject )
{
  allocate();
  project( shapeToProject ) ;
}
#endif

int SHVector::save( FILE* file )
{
  int written = 0 ;
  written += sizeof(real)*fwrite( &coeffsScale[0], sizeof(Vector), coeffsScale.size(), file ) ;
  return written ;
}

SHVector::SHVector( FILE*file, int numCoeffs )
{
  coeffsScale.resize( numCoeffs ) ;

  fread( &coeffsScale[0], sizeof(Vector), numCoeffs, file ) ;
}

void SHVector::allocate()
{
  bands = window->shSamps->bands ;
  coeffsScale.resize( bands*bands, 0 ) ;
}

//real SHVector::getFunctionValue( real tElevation, real pAzimuth )
Vector SHVector::operator()( real tElevation, real pAzimuth )
{
  Vector sum ;
  for( int l = 0 ; l < bands ; l++ )
    for( int m = -l; m <= l; m++ )
      sum += coeffsScale[ SH_INDEX(l,m) ] * SH::Y( l, m, tElevation, pAzimuth ) ;
  return sum ;
}

bool SHVector::isZero()
{
  for( int i = 0 ; i < coeffsScale.size() ; i++ )
    if( coeffsScale[i].nonzero() ) return false ; // something was nonzero

  return false ; // nope, all 0
}

void SHVector::scale( real value )
{
  for( int i = 0; i < coeffsScale.size() ; i++ )
    coeffsScale[i] *= value ;
}

void SHVector::scale( Vector value )
{
  for( int i = 0; i < coeffsScale.size() ; i++ )
    coeffsScale[i] *= value ;
}

SHVector* SHVector::scaledCopy( real value ) const
{
  SHVector *copy = new SHVector() ;
  
  for( int i= 0; i < coeffsScale.size() ; i++ )
    copy->coeffsScale[i] = coeffsScale[i] * value ;

  return copy ;
}

SHVector* SHVector::scaledCopy( const Vector& value ) const
{
  SHVector *copy = new SHVector() ;

  for( int i= 0; i < coeffsScale.size() ; i++ )
    copy->coeffsScale[i] = coeffsScale[i] * value ;

  return copy ;
}

SHVector* SHVector::add( SHVector *other )
{
  for( int i = 0 ; i < coeffsScale.size() ; i++ )
    coeffsScale[i]+=other->coeffsScale[i];

  return this ;
}

SHVector* SHVector::addedCopy( SHVector *other )
{
  SHVector *copy = new SHVector(*this) ;
  return copy->add(other);
}

SHVector* SHVector::sub( SHVector *other )
{
  for( int i = 0 ; i < coeffsScale.size() ; i++ )
    coeffsScale[i]-=other->coeffsScale[i];
  return this ;
}

SHVector* SHVector::subFloor( SHVector *other )
{
  for( int i = 0 ; i < coeffsScale.size() ; i++ )
  {
    // sub the other from this,
    Vector nc = coeffsScale[i] - other->coeffsScale[i] ;
    
    // for each component, clamp by the lower bound,
    for( int c = 0 ; c < 4 ; c++ )
    {
      // use the negative number, else use 0
      real lowerBound = coeffsScale[i].e[c] < 0 ? coeffsScale[i].e[c] : 0.0 ;
      clamp( nc.e[c], lowerBound, (real)1e6 ) ;
    }
    
    // save it.
    coeffsScale[i] = nc ;
  }

  return this ;
}

SHVector* SHVector::subbedCopy( SHVector *other )
{
  SHVector *copy = new SHVector(*this) ;
  return copy->sub(other);
}

SHVector* SHVector::subbedFloorCopy( SHVector *other )
{
  // don't let any component get MORE NEGATIVE than it already was.
  SHVector *copy = new SHVector(*this) ;
  
  for( int i = 0 ; i < coeffsScale.size() ; i++ )
  {
    Vector nc = coeffsScale[i] - other->coeffsScale[i] ;
    for( int c = 0 ; c < 4 ; c++ )
    {
      // use the negative number, else use 0
      real lowerBound = coeffsScale[i].e[c] < 0 ? coeffsScale[i].e[c] : 0.0 ;
      clamp( nc.e[c], lowerBound, (real)1e6 ) ;
    }
    
    copy->coeffsScale[i] = nc ;
  }

  return copy ;
}

SHVector* SHVector::mult( SHVector *proj )
{
  for( int i= 0; i < coeffsScale.size() ; i++ )
    coeffsScale[i] *= proj->coeffsScale[i] ;
  return this ;
}

SHVector* SHVector::multedCopy( SHVector *proj )
{
  SHVector *copy = new SHVector( *this ) ; // form a copy based on this
  return copy->mult( proj ) ; // call mult on the copy, return the copy
}

SHVector* SHVector::cut( SHVector* cutter ) const
{
  // Get the lobe which cuts the piece of the light function you want.
  SHVector *dough = new SHVector( *this ) ; // copy this.

  // You take the dough, which is the original SH function.
  // SUB OUT the "cutter" from that SH function.  This is the "scrap material".
  SHVector *scrap = dough->subbedFloorCopy( cutter ) ;

  // Take the original function again, and 
  // sub away the "scrap material" now.
  // The result is the intersection of "cutter" with "this".
  SHVector *cutout = dough->sub( scrap ) ;

  return cutout ;
}

SHVector* SHVector::conv( SHVector* other ) const
{
  // the formula for sh convolution is in the paper
  // 
  SHVector* con = new SHVector();

  for( int l = 0 ; l < bands ; l++ )
    for( int m = -l; m <= l; m++ )
      con->coeffsScale[ SH_INDEX(l,m) ] = sqrt( 4*PI / (2*l+1) ) *
        coeffsScale[ SH_INDEX( l,0 ) ] * // use band m=0 __all the time__ for the convolution from the zonal harmonic (this)
        other->at( l,m ) ;
  
  return con ;
}

void SHVector::makeZonal()
{
  for( int l = 0 ; l < bands ; l++ )
    for( int m = -l; m <= l; m++ )
      if( m != 0 ) coeffsScale[ SH_INDEX(l,m) ] = 0 ;
}

Vector SHVector::dot( SHVector* other )
{
  Vector t ;
  for( int i = 0 ; i < coeffsScale.size() ; i++ )
    t += coeffsScale[ i ] * other->coeffsScale[ i ] ;
  return t ;
}

// rotates this SH function by rotation matrix r.
SHVector* SHVector::rotate( const Matrix& r ) const
{
  SHVector* rotated = new SHVector() ;
  
  SHRotationMatrix shRot( bands, r ) ; // construct the rotation matrix

  // Copy over the band 0 because it doesn't rotate
  rotated->coeffsScale[ SH_INDEX(0,0) ] += coeffsScale[ 0 ] ;

  for( int l = 1 ; l < bands ; l++ )
    for( int m = -l ; m <= l ; m++ )
      for( int mm = -l ; mm <= l ; mm++ )
        rotated->coeffsScale[ SH_INDEX(l,m) ] += shRot.coeff[ shRot.index( l,m,mm ) ] * coeffsScale[ SH_INDEX( l,mm ) ] ;

  return rotated ;
}
  
// I'm not sure the name of this function is very good/accurate.
void SHVector::integrateSample( SHSample & sample, real proportion, const Vector& color )
{
  // a sample is a "PIECE" of an SH function.
  // sample encodes a direction, and the "size" (radial)
  // in each band.
  for( int l = 0 ; l < bands ; l++ )  // i'm going to walk thru (l,m) to go thru the bands
  {
    for( int m = -l; m <= l; m++ )
    {
      int index = SH_INDEX( l,m ) ;

      // How *MUCH* of 
      coeffsScale[ index ] += color * proportion * sample.bandWeights[ index ] ;
      // that's a scaled copy (scaled by angle of SAMPLE with vertex normal)
      // You could move the (l,m) loop outside
    }
  }
}

Model* SHVector::generateVisualization( int slices, int stacks, const Vector& center )
{
  // you can't pass an object that HAS a lambda function AS a lambda function,
  // so I had to wrap it
  function<Vector (real,real)> f = [this]( real tEl, real pAz ) -> Vector {
    return (*this)( tEl, pAz ) ;
  } ;

  Model * model = Model::visualize( "SHVector visualization", slices, stacks, center, f ) ;
    ///function<Vector (real,real)>( *this ) ) ; ////////ERROR
  
  // double the stacks and try again.
  if( !model->meshGroup->meshes[0]->verts.size() )
  {
    warning( "SH viz had 0 verts, try increasing slices(%d) and stacks(%d)", slices,stacks ) ;

    // 
    //DESTROY( model ) ;
    //model = Model::visualize( "SHVector visualization", slices*=2, stacks*=2, center, f ) ;
  }

  return model ;
}

void SHVector::generateVisualizationAutoAdd( int slices, int stacks, const Vector& center )
{
  Model * model = generateVisualization( slices,stacks,center ); 

  if( model )
  {
    MutexLock LOCK( window->scene->mutexVizList, INFINITE );
    window->scene->addVisualization( model ) ;
  }
}

void SHVector::generateVisualizationR( int slices, int stacks, const Vector& center )
{
  function<real (real,real)> mag = [this]( real tEl, real pAz ) -> real {
    return (*this)( tEl, pAz ).x ;
  } ;

  Model * model = Model::visualize( "SHVector visualization", slices, stacks, center,
    Vector(0,1,0), // COLORPLUS: Green
    Vector(1,0,0), // COLORMINUS: Red
    mag ) ;
  
  if( !model->meshGroup->meshes[0]->verts.size() )
  {
    warning( "SH viz had 0 verts" ) ;
    DESTROY( model ) ;
  }
  else
  {
    MutexLock LOCK( window->scene->mutexVizList, INFINITE );
    window->scene->addVisualization( model ) ;
  } // lock release
}

void SHVector::print()
{
  for( int l = 0 ; l < bands ; l++ )
  {
    for( int m = -l ; m <= l ; m++ )
    {
      Vector& v = coeffsScale[ SH_INDEX(l,m) ];
      plain( "[ %+.2f, %+.2f, %+.2f ] ", v.x, v.y, v.z ) ;
    }
    plain( "\n" ) ;
  }
}

void SHVector::printZonal()
{
  for( int l = 0 ; l < bands ; l++ )
  {
    Vector& v = coeffsScale[ SH_INDEX(l,0) ];
    plain( "[ %+.2f, %+.2f, %+.2f ]\n", v.x, v.y, v.z ) ;
  }
}

















SHMatrix::SHMatrix()
{
  // band: 0  1  2  3
  // elts: 1  3  5  7
  //celts: 1  4  9  16

  // celts is l^2, where l is #bands.

  // the matrix has l^4 elts then,
  bands = window->shSamps->bands ;
  rows=cols=bands*bands;
  coeff.resize( rows*cols, 0.0 ) ;
}

Vector& SHMatrix::operator()( int l1, int m1, int l2, int m2 )
{
  int row = SH_INDEX( l1, m1 ) ;
  int col = SH_INDEX( l2, m2 ) ;

  return coeff[ row*cols + col ] ;
}

Vector& SHMatrix::operator()( int row, int col )
{
  return coeff[ row*cols + col ] ;
}

SHMatrix* SHMatrix::add( SHMatrix* other )
{
  for( int i = 0 ; i < coeff.size() ; i++ )
    coeff[i] += other->coeff[i];

  return this ;
}

SHMatrix* SHMatrix::sub( SHMatrix* other )
{
  for( int i = 0 ; i < coeff.size() ; i++ )
    coeff[i] -= other->coeff[i];

  return this ;
}

SHMatrix* SHMatrix::scale( real val )
{
  for( int i = 0 ; i < coeff.size() ; i++ )
    coeff[i] *= val ;

  return this ;
}

SHMatrix* SHMatrix::scale( const Vector& val )
{
  for( int i = 0 ; i < coeff.size() ; i++ )
    coeff[i] *= val ;

  return this ;
}

SHMatrix* SHMatrix::scaledCopy( real val ) const
{
  SHMatrix *m = new SHMatrix( *this ) ; // copy this
  return m->scale( val ) ; // scaled version of it
}

void SHMatrix::integrateSample( SHSample& sample, const Vector& scale )
{
  // now the formula is,
  // M(i,j) = yj(sd)*yi(sd)
  for( int i = 0 ; i < rows ; i++ )
  {
    for( int j = 0 ; j < cols ; j++ )
    {
      (*this)( i,j ) += scale * sample.bandWeights[i] * sample.bandWeights[j];
    }
  }
}

SHVector* SHMatrix::mult( SHVector* vec )
{
  // [ x x x ][ y ] = [ res ]
  // [ x x x ][ y ]   [ res ]
  // [ x x x ][ y ]   [ res ]
  SHVector* result=new SHVector() ;
  for( int i = 0 ; i < rows ; i++ )
    for( int j = 0 ; j < cols ; j++ )
      result->coeffsScale[i] += (*this)( i, j ) * vec->coeffsScale[ j ] ;
  return result ;
}