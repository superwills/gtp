#include "Matrix.h"

#pragma region ctors
Matrix::Matrix():
  // IDENTITY
  _11(1),_12(0),_13(0),_14(0),
  _21(0),_22(1),_23(0),_24(0),
  _31(0),_32(0),_33(1),_34(0),
  _41(0),_42(0),_43(0),_44(1)
{ }

Matrix::Matrix(
  real m11, real m12, real m13, real m14,
  real m21, real m22, real m23, real m24,
  real m31, real m32, real m33, real m34,
  real m41, real m42, real m43, real m44 ):
_11(m11), _12(m12), _13(m13), _14(m14),
_21(m21), _22(m22), _23(m23), _24(m24),
_31(m31), _32(m32), _33(m33), _34(m34),
_41(m41), _42(m42), _43(m43), _44(m44)
{ }

Matrix::Matrix( real m11, real m12, real m13,
          real m21, real m22, real m23,
          real m31, real m32, real m33 ):
_11(m11), _12(m12), _13(m13), _14(0),
_21(m21), _22(m22), _23(m23), _24(0),
_31(m31), _32(m32), _33(m33), _34(0),
_41( 0 ), _42( 0 ), _43( 0 ), _44(1)
{
  
}


Matrix::Matrix( const Matrix & a ):
  _11(a._11),_12(a._12),_13(a._13),_14(a._14),
  _21(a._21),_22(a._22),_23(a._23),_24(a._24),
  _31(a._31),_32(a._32),_33(a._33),_34(a._34),
  _41(a._41),_42(a._42),_43(a._43),_44(a._44)
{ }


//Matrix::Matrix( MatrixType mType, Vector arg ) {}
#pragma endregion

#pragma region <CONST>
void Matrix::print() const
{
  plain( 
    "[ %9.2f %9.2f %9.2f %9.2f ]\n"
    "[ %9.2f %9.2f %9.2f %9.2f ]\n"
    "[ %9.2f %9.2f %9.2f %9.2f ]\n"
    "[ %9.2f %9.2f %9.2f %9.2f ]\n",
    _11, _12, _13, _14,
    _21, _22, _23, _24,
    _31, _32, _33, _34,
    _41, _42, _43, _44
  ) ;
}

bool Matrix::operator==( const Matrix& b ) const
{
  // unrolled
  return 
    _11 == b._11 &&
    _12 == b._12 &&
    _13 == b._13 &&
    _14 == b._14 &&

    _21 == b._21 &&
    _22 == b._22 &&
    _23 == b._23 &&
    _24 == b._24 &&

    _31 == b._31 &&
    _32 == b._32 &&
    _33 == b._33 &&
    _34 == b._34 &&

    _41 == b._41 &&
    _42 == b._42 &&
    _43 == b._43 &&
    _44 == b._44 ;
}
bool Matrix::operator!=( const Matrix& b ) const
{
  return 
    _11 != b._11 ||
    _12 != b._12 ||
    _13 != b._13 ||
    _14 != b._14 ||

    _21 != b._21 ||
    _22 != b._22 ||
    _23 != b._23 ||
    _24 != b._24 ||

    _31 != b._31 ||
    _32 != b._32 ||
    _33 != b._33 ||
    _34 != b._34 ||

    _41 != b._41 ||
    _42 != b._42 ||
    _43 != b._43 ||
    _44 != b._44 ;
}

bool Matrix::Near( const Matrix& b ) const
{
  return 
    fabs( _11 - b._11 ) < EPS_MIN &&
    fabs( _12 - b._12 ) < EPS_MIN &&
    fabs( _13 - b._13 ) < EPS_MIN &&
    fabs( _14 - b._14 ) < EPS_MIN &&

    fabs( _21 - b._21 ) < EPS_MIN &&
    fabs( _22 - b._22 ) < EPS_MIN &&
    fabs( _23 - b._23 ) < EPS_MIN &&
    fabs( _24 - b._24 ) < EPS_MIN &&

    fabs( _31 - b._31 ) < EPS_MIN &&
    fabs( _32 - b._32 ) < EPS_MIN &&
    fabs( _33 - b._33 ) < EPS_MIN &&
    fabs( _34 - b._34 ) < EPS_MIN &&

    fabs( _41 - b._41 ) < EPS_MIN &&
    fabs( _42 - b._42 ) < EPS_MIN &&
    fabs( _43 - b._43 ) < EPS_MIN &&
    fabs( _44 - b._44 ) < EPS_MIN ;
}

// You specify epsilon, instead of
// using the built in value.
bool Matrix::Near( const Matrix& b, real eps ) const
{
  return 
    fabs( _11 - b._11 ) < eps &&
    fabs( _12 - b._12 ) < eps &&
    fabs( _13 - b._13 ) < eps &&
    fabs( _14 - b._14 ) < eps &&

    fabs( _21 - b._21 ) < eps &&
    fabs( _22 - b._22 ) < eps &&
    fabs( _23 - b._23 ) < eps &&
    fabs( _24 - b._24 ) < eps &&

    fabs( _31 - b._31 ) < eps &&
    fabs( _32 - b._32 ) < eps &&
    fabs( _33 - b._33 ) < eps &&
    fabs( _34 - b._34 ) < eps &&

    fabs( _41 - b._41 ) < eps &&
    fabs( _42 - b._42 ) < eps &&
    fabs( _43 - b._43 ) < eps &&
    fabs( _44 - b._44 ) < eps ;
}

bool Matrix::Near3x3( const Matrix& b ) const
{
  return
    IsNear( _11, b._11, EPS_MIN ) &&
    fabs( _12 - b._12 ) < EPS_MIN &&
    fabs( _13 - b._13 ) < EPS_MIN &&

    fabs( _21 - b._21 ) < EPS_MIN &&
    fabs( _22 - b._22 ) < EPS_MIN &&
    fabs( _23 - b._23 ) < EPS_MIN &&

    fabs( _31 - b._31 ) < EPS_MIN &&
    fabs( _32 - b._32 ) < EPS_MIN &&
    fabs( _33 - b._33 ) < EPS_MIN ;
}

Matrix Matrix::transposedCopy() const
{
  Matrix m( *this ) ;
  return m.transpose() ;
}

Matrix Matrix::getNormalMatrix() const
{
  Matrix inv = !(*this) ;
  return inv.transpose() ;
}

real Matrix::cofactor( int row, int col ) const
{
  int sign = (row + col)%2==0 ? 1 : -1 ;

  int or1 = (row + 1) % 3 ;
  int or2 = (row + 2) % 3 ;
  if( or1 > or2 ) swap( or1, or2 ) ;

  int oc1 = (col + 1) % 3 ;
  int oc2 = (col + 2) % 3 ;
  if( oc1 > oc2 ) swap( oc1, oc2 ) ;

  return sign * ( m[or1][oc1] * m[or2][oc2] - m[or2][oc1]*m[or1][oc2] ) ;
}

real Matrix::det() const
{
  return _11*( _22*_33 - _32*_23 ) - //cofactor
    _12*( _21*_33 - _31*_23 ) +
    _13*( _21*_32 - _31*_22 ) ;
}

/// Invert
// Bang the matrix to invert it.
Matrix Matrix::operator!() const
{
  real de = det() ;
  if( !de ) // NOT INVERTIBLE, you get identity
    return Matrix() ;

  // if its special orthogonal the inverse is the transpose
  if( IsNear( de, 1, EPS_MIN ) )
  {
    // now check if the columns are orthogonal
    if( getRow(0)->isOrthogonalWith( *getRow(1) ) && 
        getRow(0)->isOrthogonalWith( *getRow(2) ) &&
        getRow(1)->isOrthogonalWith( *getRow(2) ) )
      return transposedCopy() ;
  }

  Matrix inverse(
    
    cofactor(0,0), cofactor(0,1), cofactor(0,2),
    cofactor(1,0), cofactor(1,1), cofactor(1,2), 
    cofactor(2,0), cofactor(2,1), cofactor(2,2)

  ) ;

  inverse *= (1.0 / de) ;

  return inverse ;
}
#pragma endregion </CONST>

#pragma region <NON-CONST>
Matrix& Matrix::triangularize()
{
  // get as triangular as can be
  // collect zero rows at bottom
  // [ 0   1   2 ]
  // | 1   2   3 |
  // [ 0   0   1 ]
  const static int n = 4 ;
  int parked = 0 ;
  
  // walk across cols
  for( int col = 0 ;
       col < n && parked < (n-1);
       ++col )
  {
    // walk down rows, looking for ANY non-zero entry.
    for( int row = parked ;
         row < n && parked < (n-1);
         ++row )
    {
      if( !IsNear( m[ row ][ col ], 0.0, 1e-6 ) )
      {
        // this is it!  take this row to (one below) the
        // last parked row.
        swapRows( row, parked ) ;

        parked++ ;

        // WE CONTINUE looking for non-zero elements
        // to stack on the top part of the matrix.
        // by the end, all the 0 rows will be at
        // the bottom.
      }
    }
  }

  return *this ;
}

Matrix& Matrix::swapRows( int row1, int row2 )
{
  if( row1 != row2 )  // no-op if it is
  {
    // at first i used 3 here,
    // assuming operation on a 3x3 submatrix.
    // but then i changed it to 4
    for( int col = 0 ; col < 4 ; col++ )
      swap( m[ row1 ][ col ], m[ row2 ][ col ] ) ;
  }
  return *this ;
}


// srcMatrix is always "this"
Matrix& Matrix::rowAdd( int srcRow, int dstRow, real multiplier )
{
  for( int col = 0 ; col < 4 ; col++ )
    m[ dstRow ][ col ] += m[ srcRow ][ col ] * multiplier ;

  return *this ;
}

Matrix& Matrix::rowMult( int row, real mult )
{
  // only do 3 cols
  for( int col = 0 ; col < 4 ; col++ )
    m[ row ][ col ] *= mult ;

  return *this ;
}

Matrix& Matrix::invert()
{
  *this = !(*this) ;

  return *this ;
}

Matrix& Matrix::roundOff()
{
  for( int row = 0 ; row < 4 ; row++ )
    for( int col = 0 ; col < 4 ; col++ )
      if( IsNear( m[ row ][ col ], 0.0, EPS_MIN ) )
        m[ row ][ col ] = 0.0 ;

  return *this ;
}


/// Transposes the matrix.
Matrix& Matrix::transpose()
{
  // _11 _12 _13 _14
  // _21 _22 _23 _24
  // _31 _32 _33 _34
  // _41 _42 _43 _44

  //6 swaps
  swap( _21, _12 ) ;
  swap( _31, _13 ) ;
  swap( _32, _23 ) ;
  
  swap( _41, _14 ) ;
  swap( _42, _24 ) ;
  swap( _43, _34 ) ;

  return *this ;
}

Matrix& Matrix::operator+=( const Matrix& b )
{
  _11 += b._11 ;   _12 += b._12 ;   _13 += b._13 ;   _14 += b._14 ;
  _21 += b._21 ;   _22 += b._22 ;   _23 += b._23 ;   _24 += b._24 ;
  _31 += b._31 ;   _32 += b._32 ;   _33 += b._33 ;   _34 += b._34 ;
  _41 += b._41 ;   _42 += b._42 ;   _43 += b._43 ;   _44 += b._44 ;

  return *this ;
}

Matrix& Matrix::operator+=( real scalar )
{
  _11 += scalar ;   _12 += scalar ;   _13 += scalar ;   _14 += scalar ;
  _21 += scalar ;   _22 += scalar ;   _23 += scalar ;   _24 += scalar ;
  _31 += scalar ;   _32 += scalar ;   _33 += scalar ;   _34 += scalar ;
  _41 += scalar ;   _42 += scalar ;   _43 += scalar ;   _44 += scalar ;

  return *this ;
}

Matrix& Matrix::operator-=( const Matrix& b )
{
  _11 -= b._11 ;   _12 -= b._12 ;   _13 -= b._13 ;   _14 -= b._14 ;
  _21 -= b._21 ;   _22 -= b._22 ;   _23 -= b._23 ;   _24 -= b._24 ;
  _31 -= b._31 ;   _32 -= b._32 ;   _33 -= b._33 ;   _34 -= b._34 ;
  _41 -= b._41 ;   _42 -= b._42 ;   _43 -= b._43 ;   _44 -= b._44 ;

  return *this ;
}

Matrix& Matrix::operator-=( real scalar )
{
  _11 -= scalar ;   _12 -= scalar ;   _13 -= scalar ;   _14 -= scalar ;
  _21 -= scalar ;   _22 -= scalar ;   _23 -= scalar ;   _24 -= scalar ;
  _31 -= scalar ;   _32 -= scalar ;   _33 -= scalar ;   _34 -= scalar ;
  _41 -= scalar ;   _42 -= scalar ;   _43 -= scalar ;   _44 -= scalar ;

  return *this ;
}

Matrix& Matrix::operator*=( const Matrix& b )
{
  // You cannot update 'this' in place 
  // as you compute the result because the
  // entries get used in more than one line..

  // Treating 'this' as the left hand matrix..
  *this = *this * b ;
  return *this ;
}

Matrix& Matrix::operator*=( real scalar )
{
  _11 *= scalar ;   _12 *= scalar ;   _13 *= scalar ;   _14 *= scalar ;
  _21 *= scalar ;   _22 *= scalar ;   _23 *= scalar ;   _24 *= scalar ;
  _31 *= scalar ;   _32 *= scalar ;   _33 *= scalar ;   _34 *= scalar ;
  _41 *= scalar ;   _42 *= scalar ;   _43 *= scalar ;   _44 *= scalar ;

  return *this ;
}

Matrix& Matrix::operator/=( real scalar )
{
  scalar = 1.0/scalar ;

  return *this *= scalar ;
}
#pragma endregion </NON-CONST>

#pragma region <STATIC CONSTRUCTION ROUTINES>
Matrix Matrix::RotationX( real radians )
{
  //ROW MAJOR
  return Matrix(
    1,            0,            0, 0,
    0, cos(radians), sin(radians), 0,
    0,-sin(radians), cos(radians), 0,
    0,            0,            0, 1
  ) ;
}

Matrix Matrix::RotationY( real radians )
{
  //ROW MAJOR
  return Matrix(
    cos(radians), 0,-sin(radians), 0,
               0, 1,            0, 0,
    sin(radians), 0, cos(radians), 0,
               0, 0,            0, 1
  );
}

Matrix Matrix::RotationZ( real radians )
{
  //ROW MAJOR
  return Matrix(
    cos(radians), sin(radians), 0, 0,
   -sin(radians), cos(radians), 0, 0,
               0,            0, 1, 0,
               0,            0, 0, 1
  ) ;
}

/// Arbitrary axis angle rotation
Matrix Matrix::Rotation( const Vector & u, real radians )
{
  real c = cos( radians ) ;
  real l_c = 1 - c ;

  real s = sin( radians ) ;
  
  //ROW MAJOR
  return Matrix(
    u.x*u.x + (1 - u.x*u.x)*c,      u.x*u.y*l_c + u.z*s,        u.x*u.z*l_c - u.y*s,  0,
          u.x*u.y*l_c - u.z*s,  u.y*u.y+(1 - u.y*u.y)*c,        u.y*u.z*l_c + u.x*s,  0,
          u.x*u.z*l_c + u.y*s,      u.y*u.z*l_c - u.x*s,  u.z*u.z + (1 - u.z*u.z)*c,  0,
                            0,                        0,                          0,  1
  ) ;


  //COLUMN MAJOR
  //return Matrix(
  //  u.x*u.x + (1 - u.x*u.x)*c,   u.x*u.y*l_c - u.z*s,   u.x*u.z*l_c + u.y*s, 0,
  //  u.x*u.y*l_c + u.z*s,   u.y*u.y+(1 - u.y*u.y)*c,   u.y*u.z*l_c - u.x*s,   0,
  //  u.x*u.z*l_c - u.y*s,   u.y*u.z*l_c + u.x*s,   u.z*u.z + (1 - u.z*u.z)*c, 0,
  //  0,0,0,1
  //) ;
}

Matrix Matrix::RotationYawPitchRoll( real yawRadians, real pitch, real roll )
{
  // Order: roll, pitch, yaw
  // z-axis, x-axis, y-axis,
  Matrix rollMat = RotationZ( roll ) ;

  Matrix pitchMat = RotationX( pitch ) ;

  Matrix yawMat = RotationY( yawRadians ) ;

  return yawMat*pitchMat*rollMat ;
}

Matrix Matrix::Translate( const Vector& v )
{
  return Matrix(

      1,   0,   0, 0,
      0,   1,   0, 0,
      0,   0,   1, 0,
    v.x, v.y, v.z, 1

  ) ;
}

Matrix Matrix::Translate( real x, real y, real z )
{
  return Matrix(

    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    x, y, z, 1

  ) ;
}

Matrix Matrix::Scale( real x, real y, real z )
{
  return Matrix(
    x, 0, 0, 0,
    0, y, 0, 0,
    0, 0, z, 0,
    0, 0, 0, 1
  ) ;
}

Matrix Matrix::Scale( const Vector& v )
{
  return Matrix(
    v.x,   0,   0, 0,
    0,   v.y,   0, 0,
    0,     0, v.z, 0,
    0,     0,   0, 1
  ) ;
}

// Up may not necessarily be orthogonal to vector from eye -to> look (look-eye)
Matrix Matrix::LookAt( const Vector & eye, const Vector & look, const Vector & up )
{
  Vector fwd = (look - eye).normalize() ; // FORWARD, or z-axis.
  Vector right = (fwd << up).normalize() ;     // RIGHT, or x-axis.
  Vector oUP = (right << fwd) ; // UP, or y-axis:  already normal since u and w are perp and normalized

  Matrix rot = ViewingFace( right,oUP,fwd ) ; // you have to take
  // the change basis matrix AND INVERT IT (by transposing), because
  // you are transforming vertices WITH ORIENTATION DESCRIBED to
  // line up with r=(1,0,0), u=(0,1,0), f=(0,0,-1)
  Matrix trans = Translate( -eye ) ;// negate the eye,
  // because what's actually happening is we don't
  // move the eye, we move the world instead.
  // To move the eye to (0,0,5), we
  // move the world by (0,0,-5).

  // inversed translate followed by inversed rotation, ROW MAJOR
  return trans * rot ;
}

Matrix Matrix::LookAtFORWARD( const Vector & eye, const Vector & look, const Vector & up )
{
  Vector fwd = (look - eye).normalize() ;
  Vector right = (fwd << up).normalize() ;
  Vector oUP = (right << fwd) ;

  Matrix rot = TransformToFace( right,oUP,fwd ) ;
  Matrix trans = Translate( eye ) ;

  // rotation first, then translate ROW MAJOR
  return rot * trans  ;
}

// Two projection matrices
Matrix Matrix::ProjOrtho( real l, real r, real b, real t, real n, real f )
{
  // The orthographic projection is
  // simply a translate followed by
  // a scale
  Matrix translate = Translate( -(l+r)/2, -(b+t)/2, -(n+f)/2 ) ;
  Matrix scale = Scale( 2 / (r - l), 2 / (t - b), 2 / (n - f) ) ;

  return scale*translate ;
}

Matrix Matrix::ProjPerspectiveRH( real fieldOfViewRadians, real aspectRatio, real n, real f )
{
  real yScale = 1.0/tan( fieldOfViewRadians/2.0 ) ;
  real xScale = yScale / aspectRatio ;

  //ROWMAJOR
  // From http://msdn.microsoft.com/en-us/library/bb205351(VS.85).aspx
  return Matrix(
    xScale,      0,          0,  0,
         0, yScale,          0,  0,
         0,      0,    f/(n-f), -1,
         0,      0,  n*f/(n-f),  0
  ) ;
}

//http://msdn.microsoft.com/en-us/library/bb205354(VS.85).aspx
Matrix Matrix::ProjPerspectiveFrustum( real l, real r, real t, real b, real zn, real zf )
{
  return Matrix(
    2*zn/(r-l) ,  0          ,  0            ,    0,
    0          ,  2*zn/(t-b) ,  0            ,    0,
    (l+r)/(r-l),  (t+b)/(t-b),  zf/(zn-zf)   ,   -1,
    0          ,  0          ,  zn*zf/(zn-zf),    0
  ) ;
}

// RH => LH
Matrix Matrix::ViewingFace( const Vector& right, const Vector& up, const Vector& forward )
{
  // We use -fwd as new z-axis because 
  // the standard is to LOOK DOWN -z.
  // this effectively flips from LH to RH, or vice versa

  //INVERTED
  return Matrix(
    right.x, up.x, -forward.x, 0,
    right.y, up.y, -forward.y, 0,
    right.z, up.z, -forward.z, 0,
          0,    0,          0, 1
  ) ;
}

// Keeps handedness
Matrix Matrix::TransformToFace( const Vector& right, const Vector& up, const Vector& forward )
{
  // ROW MAJOR: The new basis vectors in terms
  // of the original basis are the ROWS of M.
  return Matrix(
       right.x,    right.y,    right.z, 0,
          up.x,       up.y,       up.z, 0,
     forward.x,  forward.y,  forward.z, 0, ///!!! Can't remember why I left this unnegated? but it keeps handedness
             0,          0,          0, 1
  ) ;
}

#pragma endregion </STATIC CONSTRUCTION ROUTINES>


#pragma region <NON-MEMBER FUNCTIONS>
Matrix operator+( const Matrix & a, const Matrix & b )
{
  return Matrix(
    
    a._11 + b._11, a._12 + b._12, a._13 + b._13, a._14 + b._14,
    a._21 + b._21, a._22 + b._22, a._23 + b._23, a._24 + b._24,
    a._31 + b._31, a._32 + b._32, a._33 + b._33, a._34 + b._34,
    a._41 + b._41, a._42 + b._42, a._43 + b._43, a._44 + b._44

  ) ;
}

Matrix operator+( const Matrix & a, real scalar )
{
  return Matrix(
    
    a._11 + scalar, a._12 + scalar, a._13 + scalar, a._14 + scalar,
    a._21 + scalar, a._22 + scalar, a._23 + scalar, a._24 + scalar,
    a._31 + scalar, a._32 + scalar, a._33 + scalar, a._34 + scalar,
    a._41 + scalar, a._42 + scalar, a._43 + scalar, a._44 + scalar

  ) ;
}

Matrix operator+( real scalar, const Matrix & a )
{
  return Matrix(
    
    a._11 + scalar, a._12 + scalar, a._13 + scalar, a._14 + scalar,
    a._21 + scalar, a._22 + scalar, a._23 + scalar, a._24 + scalar,
    a._31 + scalar, a._32 + scalar, a._33 + scalar, a._34 + scalar,
    a._41 + scalar, a._42 + scalar, a._43 + scalar, a._44 + scalar

  ) ;
}

Matrix operator-( const Matrix & a, const Matrix & b )
{
  return Matrix(
    
    a._11 - b._11, a._12 - b._12, a._13 - b._13, a._14 - b._14,
    a._21 - b._21, a._22 - b._22, a._23 - b._23, a._24 - b._24,
    a._31 - b._31, a._32 - b._32, a._33 - b._33, a._34 - b._34,
    a._41 - b._41, a._42 - b._42, a._43 - b._43, a._44 - b._44

  ) ;
}

Matrix operator-( const Matrix & a, real scalar )
{
  return Matrix(
    
    a._11 - scalar, a._12 - scalar, a._13 - scalar, a._14 - scalar,
    a._21 - scalar, a._22 - scalar, a._23 - scalar, a._24 - scalar,
    a._31 - scalar, a._32 - scalar, a._33 - scalar, a._34 - scalar,
    a._41 - scalar, a._42 - scalar, a._43 - scalar, a._44 - scalar

  ) ;
}

Matrix operator-( real scalar, const Matrix & a )
{
  return Matrix(
    
    scalar - a._11, scalar - a._12, scalar - a._13, scalar - a._14,
    scalar - a._21, scalar - a._22, scalar - a._23, scalar - a._24,
    scalar - a._31, scalar - a._32, scalar - a._33, scalar - a._34,
    scalar - a._41, scalar - a._42, scalar - a._43, scalar - a._44

  ) ;
}

Matrix operator*( const Matrix & a, const Matrix & b )
{
  return Matrix(

    a._11*b._11 + a._12*b._21 + a._13*b._31 + a._14*b._41, // _11
    a._11*b._12 + a._12*b._22 + a._13*b._32 + a._14*b._42, // _12
    a._11*b._13 + a._12*b._23 + a._13*b._33 + a._14*b._43, // _13
    a._11*b._14 + a._12*b._24 + a._13*b._34 + a._14*b._44, // _14

    a._21*b._11 + a._22*b._21 + a._23*b._31 + a._24*b._41, // _21
    a._21*b._12 + a._22*b._22 + a._23*b._32 + a._24*b._42, // _22
    a._21*b._13 + a._22*b._23 + a._23*b._33 + a._24*b._43, // _23
    a._21*b._14 + a._22*b._24 + a._23*b._34 + a._24*b._44, // _24

    a._31*b._11 + a._32*b._21 + a._33*b._31 + a._34*b._41, // _31
    a._31*b._12 + a._32*b._22 + a._33*b._32 + a._34*b._42, // _32
    a._31*b._13 + a._32*b._23 + a._33*b._33 + a._34*b._43, // _33
    a._31*b._14 + a._32*b._24 + a._33*b._34 + a._34*b._44, // _34

    a._41*b._11 + a._42*b._21 + a._43*b._31 + a._44*b._41, // _41
    a._41*b._12 + a._42*b._22 + a._43*b._32 + a._44*b._42, // _42
    a._41*b._13 + a._42*b._23 + a._43*b._33 + a._44*b._43, // _43
    a._41*b._14 + a._42*b._24 + a._43*b._34 + a._44*b._44  // _44

  ) ;
}

Matrix operator*( const Matrix & a, real scalar )
{
  return Matrix(

    a._11 * scalar, a._12 * scalar, a._13 * scalar, a._14 * scalar,
    a._21 * scalar, a._22 * scalar, a._23 * scalar, a._24 * scalar,
    a._31 * scalar, a._32 * scalar, a._33 * scalar, a._34 * scalar,
    a._41 * scalar, a._42 * scalar, a._43 * scalar, a._44 * scalar

  ) ;
}

Matrix operator*( real scalar, const Matrix & a )
{
  return Matrix(

    a._11 * scalar, a._12 * scalar, a._13 * scalar, a._14 * scalar,
    a._21 * scalar, a._22 * scalar, a._23 * scalar, a._24 * scalar,
    a._31 * scalar, a._32 * scalar, a._33 * scalar, a._34 * scalar,
    a._41 * scalar, a._42 * scalar, a._43 * scalar, a._44 * scalar

  ) ;

}

#pragma endregion <NON-MEMBER FUNCTIONS>


Matrix Identity ;

