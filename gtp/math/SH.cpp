#include "SH.h"
#include "SHSample.h"
#include "../window/GTPWindow.h"
#include "../geometry/Model.h"
#include "../rendering/RayCollection.h"

void SHRotationMatrix::makeRotation( const Matrix& r )
{
  // Level 1: rotation matrix goes straight in.
  for( int l = 1 ; l < bands ; l++ )
  {
    for( int row = 0 ; row < width( l ) ; row++ )
    {
      for( int col = 0 ; col < width( l ) ; col++ )
      {
        int m = row - l ;  // when l=2, m=-2,-1,0,1,2
        int mm = col - l ;

        real val = SH::Rlmm( r, l, m, mm ) ;
        int idx = SHRotationMatrix::index( l,m,mm ) ;//getOffsetToLevel( l ) + row*width(l) + col ;
        
        //printf( "%d %d (%d)=%f\n", m,mm, index, val ) ;
        coeff[ idx ] = val ;
      }
    }
  }
}

// Start it out using Spherical harmonics, pointing to YSH
function<real (int l, int m, real theta, real phi)> SH::Y = SH::YSH ;

// defaults to legendre
SH::Kernel SH::kernel = SH::Legendre ;
char* SH::kernelName = "SH" ; 

real SH::KcCoeffs[] =
{
  // These come from MuPad by integrating the (l,m) cpherical harmonic multiplied by itself.
  // If the (l,m) harmonics are multiplied by these factors, then the inner product of
  // an (l,m) harmonic with itself is 1.  With any other band, it'd always be 0 anyway.
  2.820947918e-1, //L=0
  4.886025119e-1, 4.886025119e-1, //L=1
  4.129444918e-1, 2.731371076e-1, 1.365685538e-1, //L=2
  4.047665634e-1, 1.854328105e-1, 6.022107172e-2, 2.458514958e-2, //L=3
  4.021466875e-1, 1.399411247e-1, 3.414213846e-2, 9.219431093e-3, 3.259561122e-3, //L=4
  4.009725341e-1, 1.122723096e-1, 2.200197653e-2, 4.562170666e-3, 1.081074122e-3, 3.418656546e-4, //L=5
  4.003445423e-1, 9.370452984e-2, 1.535720259e-2, 2.607284268e-3, 4.802311784e-4, 1.027178456e-4, 2.965208790e-5, //L=6
  3.999691606e-1, 8.039264056e-2, 1.132511831e-2, 1.633723697e-3, 2.490256634e-4, 4.173679601e-5, 8.203003044e-6, 2.192344781e-6, //L=7
  3.997268284e-1, 7.038582157e-2, 8.695055776e-3, 1.092352436e-3, 1.427899302e-4, 1.994605450e-5, 3.089600665e-6, 5.649543187e-7, 1.412385797e-7, //L=8
  3.995612758e-1, 6.259091642e-2, 6.884843506e-3, 7.667497016e-4, 8.798356395e-5, 1.060573155e-5, 1.376251441e-6, 1.992021591e-7, 3.420248310e-8, 8.061602577e-9, //L=9
  3.994431498e-1, 5.634839052e-2, 5.586011607e-3, 5.589858279e-4, 5.725452568e-5, 6.091881619e-6, 6.852855807e-7, 8.342077800e-8, 1.137633131e-8, 1.847145202e-9,  4.130342235e-10, //L=10
  3.993559065e-1, 5.123694925e-2, 4.622648211e-3, 4.201140561e-4, 3.889712274e-5, 3.712222653e-6, 3.701090444e-7, 3.919400144e-8, 4.509096539e-9, 5.830959106e-10, 9.003829770e-11, 1.919622957e-11, //L=11
  3.992896399e-1, 4.697496858e-2, 3.888462059e-3, 3.237425149e-4, 2.736552222e-5, 2.371042625e-6, 2.128144456e-7, 2.003750505e-8, 2.011003346e-9, 2.199307652e-10, 2.710808555e-11, 3.999214493e-12, 8.163362401e-13, //L=12
  3.992381219e-1, 4.336708685e-2, 3.316150644e-3, 2.547591851e-4, 1.981907608e-5, 1.573796100e-6, 1.286657036e-7, 1.093755829e-8, 9.784679097e-10,9.356368862e-11, 9.773181415e-12, 1.153056687e-12, 1.631464435e-13, 3.199564996e-14 //L=13
} ;

real SH::KSH(int l, int m)
{
  // renormalisation constant for SH function
  real temp = ((2.0*l+1.0)*facs[l-m]) / (4.0*PI*facs[l+m]);
  return sqrt(temp);
}

// from Green[2003]
// these CAN BE static methods
real SH::P( int l, int m, real x )
{
  // evaluate an Associated Legendre Polynomial P(l,m,x) at x
  // this is from Green but he originally took it/modified from Numerical Recipes
  real pmm = 1.0;
  
  if( m > 0 )
  {
    real somx2 = sqrt((1.0-x)*(1.0+x));
    real fact = 1.0;
    for(int i=1; i<=m; i++) {
      //pmm *= (-fact) * somx2; // CORDON-SHORTLEY phase term embedded here
      pmm *= (fact) * somx2; // No CORDON-SHORTLEY
      fact += 2.0;
    }
  }
  if(l==m) return pmm;
  real pmmp1 = x * (2.0*m+1.0) * pmm;
  if(l==m+1) return pmmp1;
  real pll = 0.0;
  
  for(int ll=m+2; ll<=l; ++ll)
  {
    pll = ( (2.0*ll-1.0)*x*pmmp1-(ll+m-1.0)*pmm ) / (ll-m);
    pmm = pmmp1;
    pmmp1 = pll;
  }
  return pll;
}

// the SH function.
//real SH::operator()(int l, int m, real theta, real phi)  //  return sh( l,m,theta,phi ) ;
// theta is elevation, phi is azimuth,
// but elevation is measured from Z AXIS,
// and PHI sweeps around Z AXIS
real SH::YSH(int l, int m, real theta, real phi)  //  return sh( l,m,theta,phi ) ;
{
  // return a point sample of a Spherical Harmonic basis function
  // l is the band, range [0..N]
  // m in the range [-l..l]
  // theta in the range [0..Pi]
  // phi in the range [0..2*Pi]

  // GREEN VERSION:
  if(m==0) return KSH(l,0)*P(l,m,cos(theta));
  else if(m>0) return sqrt2*KSH(l,m)*cos(m*phi)*P(l,m,cos(theta));
  else return sqrt2*KSH(l,-m)*sin(-m*phi)*P(l,-m,cos(theta));
}

real SH::YCheby(int l, int m, real theta, real phi)
{
  if(m==0) return Kc(l,0)*T(l,cos(theta));
  else if(m>0) return Kc(l,m)*cos(m*phi)*Tlm(l,m,cos(theta));
  else return Kc(l,-m)*sin(-m*phi)*Tlm(l,-m,cos(theta));
}

// from Ruedenberg (1996)
// cast before division.
real SH::a( int l, int m, int mm )
{
  return sqrt( (l+m)*(l-m)/((real)(l+mm))*(l-mm) ) ;
}

real SH::b( int l, int m, int mm )
{
  return sqrt( (l+m)*(l+m-1)/((real)(l+mm))*(l-mm) ) ;
}

real SH::c( int l, int m, int mm )
{
  return sqrt( (l+m)*(l-m)/((real)(l+mm))*(l+mm-1) ) ;
}

real SH::d( int l, int m, int mm )
{
  return sqrt( (l+m)*(l+m-1)/((real)(l+mm))*(l+mm-1) ) ;
}

real SH::ulmm( int l, int m, int mm )
{
  if( abs(mm) > l )
  {
    error( "|mm|(%d) > l(%d)", mm, l );
    return 0 ;
  }

  if( abs(mm) < l )
  {
    return sqrt( (l+m)*(l-m) / ((real)(l+mm)*(l-mm)) ) ;
  }
  else // abs(mm) == l
  {
    return sqrt( (l+m)*(l-m) / ( (real)(2*l)*(2*l-1) ) ) ;
  }
}
real SH::vlmm( int l, int m, int mm )
{
  if( abs(mm) > l )
  {
    error( "|mm|(%d) > l(%d)", mm, l );
    return 0 ;
  }
  
  if( abs(mm) < l )
  {
    return 0.5 * ( m==0?-1:1 ) * 
      sqrt( (1+δ(m,0))*(l+abs(m)-1)*(l+abs(m)) / ( (real)(l+mm)*(l-mm) ) ) ;
  }
  else // abs(mm) == l
  {
    return 0.5 * ( m==0?-1:1 ) *
      sqrt( (1+δ(m,0))*(l+abs(m)-1)*(l+abs(m)) / ( (real)(2*l)*(2*l-1) ) ) ;
  }
}
real SH::wlmm( int l, int m, int mm )
{
  if( abs(mm) > l )
  {
    error( "|mm|(%d) > l(%d)", mm, l );
    return 0 ;
  }

  if( abs(mm) < l )
  {
    if( m==0 ) return 0 ; // actually delta function.
    else return -0.5 * sqrt( (l-abs(m)-1)*(l-abs(m))/( (real)(l+mm)*(l-mm) ) ) ;
  }
  else // abs(mm) == l
  {
    if( m==0 ) return 0 ;
    else return -0.5 * sqrt( (l-abs(m)-1)*(l-abs(m)) / ( (real)(2*l)*(2*l-1) ) ) ;
  }
}

real SH::Ulmm( const Matrix& r, int l, int m, int mm )
{
  return iPlumm( r,0,l,m,mm ) ; // (always the same)
}

real SH::Vlmm( const Matrix& r, int l, int m, int mm )
{
  if( m==0 )
  {
    return iPlumm(r,1,l,1,mm) + iPlumm(r,-1,l,-1,mm) ;
  }
  else if( m > 0 )
  {
    if( m==1 )
      return sqrt2 * iPlumm(r,1,l,m-1,mm) ;
    else
      return iPlumm(r,1,l,m-1,mm)   -   iPlumm(r,-1,l,-m+1,mm) ;
  }
  else // if m < 0
  {
    if( m==-1 )
      return sqrt2 * iPlumm(r,-1,l,-m-1,mm) ;
    else
      return iPlumm(r,1,l,m+1,mm)   +
             iPlumm(r,-1,l,-m-1,mm) ; 
  }
}

real SH::Wlmm( const Matrix& r, int l, int m, int mm )
{
  if( m==0 )
  {
    error( "Cannot call Wlmm with m==0" ) ;
    return 0 ;
  }
  else if( m > 0 )
  {
    return iPlumm(r,1,l,m+1,mm)   +
           iPlumm(r,-1,l,-m-1,mm) ;
  }
  else // m < 0
  {
    return iPlumm(r,1,l,m-1,mm)   -
           iPlumm(r,-1,l,-m+1,mm);
  }
}

real SH::iPlumm( const Matrix& r, int i, int l, int u, int mm )
{
  if( abs(mm) < l )
  {
    return R(r,i,0)*Rlmm(r,l-1,u,mm) ;
  }
  else if( mm == l )
  {
    return R(r,i,1)*Rlmm(r,l-1,u,l-1)   -  R(r,i,-1)*Rlmm(r,l-1,u,-l+1) ;
  }
  else // m==-l
  {
    return R(r,i,1)*Rlmm(r,l-1,u,-l+1)  +  R(r,i,-1)*Rlmm(r,l-1,u,l-1) ;
  }
}


real SH::Rlmm( const Matrix& r, int l, int m, int mm )
{
  if( l <= 1 ) // look up inside the actual rotation matrix
  {
    // base case:
    return R( r, m, mm ) ;
  }
  
  // spawn calls to Ulmm, Vlmm, Wlmm
  real u = ulmm( l,m,mm ) ;
  real v = vlmm( l,m,mm ) ;
  real w = wlmm( l,m,mm ) ;

  // ONLY CALL FOR NONZERO. When w is 0, Wlmm() is likely to be a bad call.
  if( u )  u *= Ulmm( r,l,m,mm ) ;
  if( v )  v *= Vlmm( r,l,m,mm ) ;
  if( w )  w *= Wlmm( r,l,m,mm ) ;

  return u + v + w ;
}

void SH::Rotation( const Vector& axis, real radians, vector<real>& rotation )
{
  Matrix r = Matrix::Rotation( axis, radians ) ;

  // densely packed -- there are 
  // get the rotation coefficients for 3
}

void SH::generateBasisFuncVisualizationPoints( int numPoints, int l, int m, const Vector& center )
{
  for( int i = 0 ; i < numPoints ; i++ )
  {
    SVector sv = SVector::random( 1 ) ;
    // get SH value for band.
    real value = Y( l, m, sv.tElevation, sv.pAzimuth ) ;
    window->addDebugPoint( sv.toCartesian() + center, Vector::toRGB( 180*(value+1), 1, 1 ) ) ; // coloration by mag of point
  }
}

// Generates a visualization for a band,m
Model* SH::generateBasisFuncVisualization( int l, int m, int slices, int stacks, const Vector& center ) 
{
  // A bit crufty, but I'm wrapping the (l,m) band
  // into a functor called shBand.
  function<real (real tElevation, real pAzimuth)> shBandF =
    [ l, m ](real tElevation, real pAzimuth) -> real
    {
      // Y will be set as either cheby or legendre based
      return Y( l, m, tElevation, pAzimuth ) ; // evaluate the l,m function
    } ;

  char buf[260];
  sprintf( buf, "sh l=%d m=%d visualization", l,m ) ;
  Model *model = Model::visualize( buf, slices, stacks, center, Vector(0,1,0), Vector(1,0,0), shBandF ) ;
  
  model->transform( Matrix::Translate( center ) ) ;

  return model ;
}
