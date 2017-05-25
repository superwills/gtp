#include "Vector.h"

WaveParam::WaveParam( real iFreq, real iAmp, const Vector& iLongAxis, const Vector& iTransverseAxis )
{
  freq = iFreq ;
  amp = iAmp ;
  longAxis = iLongAxis ;
  transverseAxis = iTransverseAxis ;
}

// Gets you the phase of a position in space by
// projecting onto the longitudinal axis
real WaveParam::getPhase( const Vector& pos ) const
{
  return pos % longAxis ;
}

Vector Vector::randomPointInsideTri( const Vector& a, const Vector& b, const Vector& c )
{
  Vector bary = randBary() ;
  return a*bary.x + b*bary.y + c*bary.z ; 
}


bool Vector::isCloser( const Vector& amICloser, const Vector& thanMe ) const {
  return (*this - amICloser).len2()   <   (*this - thanMe).len2() ;
}

int Vector::printInto( char *buf ) const
{
  return sprintf( buf, "(%.2f %.2f %.2f)", x,y,z ) ;
}

void Vector::printPlain() const
{
  plain( "( %3.2f, %3.2f, %3.2f )\n", x,y,z ) ;
}

void Vector::print() const
{
  printf( "( %3.2f, %3.2f, %3.2f )\n", x,y,z ) ;
}

Vector& Vector::jitter( real amnt )
{
  this->normalize() ;

  // jitter by amnt about a random axis
  Vector axis = (*this) << SVector::random(1).toCartesian() ; //Vector::random(-1,1) ;
  axis.normalize() ;

  *this = (*this) * Matrix::Rotation( axis, sqrt(randFloat(0,amnt*amnt)) ) ;
  return *this ;
}

Vector Vector::jitteredCopy( real amnt ) const
{
  Vector copy = *this ;
  return copy.jitter( amnt ) ;
}

// you could also use the householder matrix
Vector Vector::reflectedCopy( const Vector& N ) const
{
  // make a copy and reflect it
  return Vector(*this).reflect( N ) ;
}

Vector Vector::refractedCopy( const Vector& N, real n1, real n2 ) const
{
  return Vector(*this).refract( N, n1, n2 ) ;
}

Vector Vector::halfWay( const Vector& other ) const
{
  // from torrance/ "a reflectance model for cg"
  Vector VL = (*this) + other ;
  return VL.normalize() ;
}

Vector Vector::getRandomPerpendicular() const
{
  if( len2() == 0 )
  {
    error( "normal was 0" ) ;
    return Vector(0,1,0) ; // you get Y.
  }

  // Get a RANDOM up vector by crossing THIS
  // with random.
  Vector perp ;
  do perp = *this << Vector::random(-1,1) ;
  while( perp.len2() == 0 ) ; // if the random was EXACTLY the normal
  // (really small chance) (FWD), then
  // the up vector will still be 0.

  return perp.normalize() ;
}

Vector& Vector::flip( const Vector& N )
{
  // really negative of reflect, but with how it works easier to type code
  return *this = 2*( *this % N ) * N - *this ;  // opposite to reflection
}

Vector& Vector::reflect( const Vector& N )
{
  //Vector L = this->normalizedCopy() ;
  //return (L - 2 * ( (L % N) * N )).normalize() ; // no one said normalize it.// reflected ray dir
  //*this = (*this - 2 * ( (*this % N) * N )) ; // reflected ray dir
  //return *this ;
  
  return *this = *this - 2*( *this % N ) * N ;  // this is right
}

// n1=FROM, n2 is the density of the material
// we are travelling TO.
// You are responsible for normalizing N before calling
Vector& Vector::refract( const Vector& N, real n1, real n2 )
{
  Vector norm = N ;

  // check that "this" is at an angle > 180 degrees to normal (so they actually "hit")
  real NdotL = N % *this ;

  // ok, if NdotL is bigger than 0, then
  // this and N face the same general direction
  // (like they are almost collinear and not even
  // greater than 90 degrees different in direction).
  // This means the incident vector didn't really
  // strike the plane, OR it means WE ARE
  // INSIDE THE OBJECT AND TRYING TO GET OUT.
  if( NdotL > 0 )
  {
    //////puts( "incident vector doesn't strike plane" ) ;
    //////return L ;
    // you are hitting the plane from BEHIND. THis is ok, it just means
    // you are intersecting outer space from INSIDE the shape,
    // which, AS THE WAY IT IN PROGRAMMED, (always call n1 with
    // coeffient empty space, n2 with coefficient of material)
    // means n1 and n2 swap,
    // and N gets reversed (because we are inside the shape, going out.)
    ////////////swap( n1, n2 ) ; // DON'T do this.  assume the user of this function will pay attention
    // to what side of the surface he is entering/exiting.

    // physically, if you hit "the back side" of a surface,
    // you can just turn the normal around.
    norm = -N ;
    NdotL = norm % *this ; // recalculate using new normal
  }

  real ti = PI - acos( NdotL ) ;        // ti=theta incident
  /////printf( "Incident angle=%f\n", DEGREES(ti) ) ;

  real angle = n1/n2 * sin( ti ) ; // n1 > n2 means FROM MORE DENSE TO LESS DENSE,
  // the denser the material we are in, the LESS LIKELY we are able to get out
  // (like wanting to stay in a "warm house". Light wants to stay inside the diamond).

  // When n1/n2 ≈ 0, angle=0 and so tr=0, which means the
  // refracted ray lines up EXACTLY with the normal. So
  // in the extreme case (entering a VERY high index of
  // refraction), the light ray will tend to go down towards
  // the NORMAL of the surface, no matter the entry angle.
  if( angle > 1 )
  {
    // TOTAL INTERNAL REFLECTION
    // when travelling from DENSE medium to LESS DENSE medium (e.g. water to air,
    // OR DIAMOND TO AIR.).  Diamond to air has a LOT of total internal reflection,
    // which is why the light bounces around inside it a lot and it sparkles.

    // return a REFLECTED vector instead
    /////puts( "TOTAL INTERNAL REFLECTION!!" ) ;
    return reflect( norm ) ;
  }

  real tr = asin( angle ) ; // tr=theta transmitted
  /////printf( "The transmitted angle=%f\n", DEGREES(tr) ) ;

  // get axis perpendicular to incident plane
  Vector axis = ( *this << norm ).normalize() ;
  /////printf( "axis: " ) ;
  /////axis.print() ;

  Matrix rot = Matrix::Rotation( axis, tr )  ;

  /////printf( "The |rot|=%f\n", rot.det() ) ;
  /////rot.print() ;

  // rotate -N tr radians to get the
  // refracted vector
  *this = -norm * rot ;
  return *this ;
}

ByteColor Vector::toSignedByteColor() const
{
  // x,y,z,w should ALREADY be between -1 and +1 each (normalized)
  return (((*this) + 1)/2).toByteColor() ; // Doesn't need to include w in the normalization
  
  // (-1,-1,-1,-1),(1,1,1,1) + 1-> (0,0,0,0),(2,2,2,2) /2-> (0,0,0,0),(1,1,1,1)
  //Vector signedColor( (x+1)/2, (y+1)/2, (z+1)/2, (w+1)/2 ) ;
  //return signedColor.toByteColor() ;
}

SVector Vector::toSpherical() const
{
  SVector v ;
  v.r = len() ;

  if( !v.r ) // tElevation, pAzimuth=0 here.
    v.pAzimuth=v.tElevation=0;
  else
  {
    // pAzimuth x-axis angle
    if( x == 0 )
      v.pAzimuth = 0 ;
    else
    {
      // z==opp, x==adj
      v.pAzimuth = atan2( z, x ) ;
    }

    v.tElevation = acos( y / v.r ) ;
  }

  return v ;
}

//Vector& Vector::operator=( const ByteColor& byteColor )
//{
//  *this = byteColor.toRGBAVector() ;
//  return *this;
//}

Vector& Vector::operator=( int v )
{
  x=y=z=w=v;
  return *this ;
}
Vector& Vector::operator=( real v )
{
  x=y=z=w=v;
  return *this ;
}

// Static
void Vector::wave( real time, real phase, real freq, real amp,
  const Vector& longAxis, const Vector& transverseAxis, Vector& pos, Vector& norm )
{
  // the displacement to the pos variable is function of amp and angle
  // in the direction of the transverseAxis
  pos += amp*sin( time*freq + phase ) * transverseAxis ;

  // the change to the normal components
  float slope = freq*amp*cos( time*freq + phase ) ;
  float t = atan( slope ) ;
  
  Vector zAxis = longAxis << transverseAxis ;

  // rotation about "z" axis 
  norm = norm * Matrix::Rotation( zAxis, t )  ;
  norm.normalize() ;
}

void Vector::wave( real time, const WaveParam& wp, Vector& pos, Vector& norm )
{
  wave( time, wp.getPhase(pos), wp.freq, wp.amp, wp.longAxis, wp.transverseAxis, pos, norm ) ;
}

#pragma region outside operators
Vector operator+( const Vector & a, const Vector & b )
{
  return Vector( a.x + b.x, a.y + b.y, a.z + b.z ) ;
}
Vector operator+( const real & a, const Vector & b )
{
  return Vector( a + b.x, a + b.y, a + b.z ) ;
}
Vector operator+( const Vector & a, const real & b )
{
  return Vector( a.x + b, a.y + b, a.z + b ) ;
}

Vector operator-( const Vector & a, const Vector & b )
{
  return Vector( a.x - b.x, a.y - b.y, a.z - b.z ) ;
}
Vector operator-( const real & a, const Vector & b )
{
  return Vector( a - b.x, a - b.y, a - b.z ) ;
}
Vector operator-( const Vector & a, const real & b )
{
  return Vector( a.x - b, a.y - b, a.z - b ) ;
}

Vector operator-( const Vector & a )
{
  return Vector( -a.x, -a.y, -a.z ) ;
}

real operator%( const Vector & a, const Vector & b )
{
  return a.Dot( b ) ;
}

bool operator,( const Vector & a, const Vector & b )
{
  return a.Near( b ) ;
}


Vector operator*( const Vector & a, const Vector & b )
{
  return a.componentWiseMultiply( b ) ; //inline, so no loss
}

// CROSS PRODUCT
Vector operator<<( const Vector & a, const Vector & b )
{
  return Vector(
    a.y*b.z - b.y*a.z,
    b.x*a.z - a.x*b.z,
    a.x*b.y - b.x*a.y
  ) ;
}

// LOSES the w component, replaces it with a 1.
// You can't really change this because it is expected behavior
// by the code already, but.. it might bite you sometimes.
Vector operator*( const Vector & a, const real b )
{
  return Vector( a.x*b, a.y*b, a.z*b ) ;
}

// LOSES the w component, replaces it with a 1.
Vector operator*( const real b, const Vector & a )
{
  return Vector( a.x*b, a.y*b, a.z*b ) ;
}

/// ROW MAJOR
Vector operator*( const Vector& v, const Matrix & a )
{
  Vector result(

    v.x*a._11 + v.y*a._21 + v.z*a._31 + a._41, //*v.w
    v.x*a._12 + v.y*a._22 + v.z*a._32 + a._42, //*v.w
    v.x*a._13 + v.y*a._23 + v.z*a._33 + a._43  //*v.w

  ) ;

  real w = v.x*a._14 + v.y*a._24 + v.z*a._34 + a._44 ; //*v.w//!! if you're NOT going multiply by w here,
  // then the Vector class doesn't need a 'w' at all (but it comes in handy
  // for specifying the alpha channel in colors!)
  
  // Here I am working in the perspective divide automatically
  if( w != 0 )  result /= w ;

  return result ;
}

Vector transformNormal( const Vector& n, const Matrix & a )
{
  return Vector(
    n.x*a._11 + n.y*a._21 + n.z*a._31,
    n.x*a._12 + n.y*a._22 + n.z*a._32,
    n.x*a._13 + n.y*a._23 + n.z*a._33
  ) ;
}

Vector operator/( const Vector & a, const real b )
{
  return Vector( a.x/b, a.y/b, a.z/b ) ;
}

Vector operator/( const Vector & a, const Vector& b )
{
  return a.componentWiseDivide( b ) ; // inline, so no loss
}
#pragma endregion

#pragma region svector
Vector SVector::toCartesian() const
{
  return Vector(
    r*sin(tElevation)* //proj xz plane
      cos(pAzimuth),
    r*cos(tElevation), // independent of pAzimuth, pAzimuth goes around the y-axis
    r*sin(tElevation)*sin(pAzimuth)
  ) ;
}

SVector SVector::random( real radius )
{
  //SVector sv( 1, randFloat(0,PI), randFloat(0,2*PI) ) ; // actually simplistic: this will tend to favor the poles and
                                                          // have a lack of samples at the equator.
  //return SVector( radius,
  //    2*acos( sqrt( randFloat() ) ),  // distribute the elevation component with GREATER emphasis on the
  //                                    // equator (LOTS of samples with tElevation=PI/2, less at poles)
  //    randFloat(0,2*PI)               // equal spacing on the AZIMUTH
  //  ) ;

  // another formula, from the "Total Compedium", avoids the square root.
  return SVector( radius,
    acos( 1 - 2*randFloat() ),
    randFloat( 0, 2*PI )
  ) ;
}

Vector SVector::randomHemi( const Vector& normal )
{
  if( normal.len2() == 0 ) {
    error( "Bad normal" ) ;
    return 0 ;
  }
  Vector cart = SVector::random( 1 ).toCartesian() ;
  
  do cart = SVector::random( 1 ).toCartesian() ;
  while( cart % normal < 0 ) ;

  return cart ;
  
  
  //////error( "This doesn't work" ) ;
  //////// use a tilted sphere with pole @ normal.
  //////SVector pole = normal.toSpherical() ;
  //////
  //////// Gets values on a hemisphere about y-axis
  //////
  ///////// You have to rotate phi/theta
  //////SVector sv( 1,
  //////    acos( sqrt( randFloat() ) ),  // distribute the elevation component with GREATER emphasis on the
  //////                                    // equator (LOTS of samples with tElevation=PI/2, less at poles)
  //////    randFloat(0,2*PI)               // equal spacing on the AZIMUTH
  //////  ) ;
  //////
  //////// rotate so centers around normal given.
  //////// the azimuth doesn't matter since its completely
  //////// symmetric "around"
  //////sv.tElevation += pole.tElevation ;
  //////sv.pAzimuth += pole.pAzimuth ;
  //////
  //////return sv ;
}

SVector SVector::randomHemiCosine( real radius )
{
  return SVector( radius,
    acos( sqrt( randFloat() ) ),
    2*PI*randFloat()
  ) ;
}

Vector SVector::randomHemiCosine( const Vector& normal )
{
  SVector sv = randomHemiCosine( 1.0 ) ;

  Vector y(0,1,0);
  Vector axis = y << normal ;
  real rads = y.angleWith( normal ) ;
  Matrix mat = Matrix::Rotation( axis, rads ) ;

  return sv.toCartesian() * mat ;
}
#pragma endregion