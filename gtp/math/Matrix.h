#ifndef MATRIX_H
#define MATRIX_H

#include <algorithm> // std::swap
using std::swap ;
#include "../util/StdWilUtil.h"
#include "Vector.h"

union Vector ;

// Modelled after d3dx package

// I made a very big mistake with this class.
// It's a mix between row major and column major.
// I liked post-multiplying vectors, so 
// I said everything was column major.
// BUT, I'm actually using a row-major ordering,
// because specifying a matrix in a ctor by row major
// makes a lot more sense visually in code.
// See http://stackoverflow.com/questions/9688509/how-is-glkits-glkmatrix-column-major/12457805#12457805

// The translation components are _14, _24, _34,
// which LOOKS visually like the COLUMN MAJOR ordering spot,
// but actually because the DATA goes ROW1COL1, ROW1COL2, ROW1COL3
// (spec data down ROWS), you are by definition using ROW MAJOR matrices
// and not column major matrices.
// 

union Matrix
{
  struct
  {
    // I should have 0-based these,
    real _11, _12, _13, _14, //ROW 1
         _21, _22, _23, _24, //ROW 2
         _31, _32, _33, _34, //ROW 3
         _41, _42, _43, _44 ;//ROW 4
  }; // ROW MAJOR
  real m[4][4] ; // m[ROW][COL]
  real a[16];
  #pragma region ctors
  Matrix() ;

  Matrix( real m11, real m12, real m13, real m14,
          real m21, real m22, real m23, real m24,
          real m31, real m32, real m33, real m34,
          real m41, real m42, real m43, real m44 ) ;

  Matrix( real m11, real m12, real m13,
          real m21, real m22, real m23,
          real m31, real m32, real m33 ) ;


  Matrix( const Matrix & a ) ;
  
  // Doesn't change anything.. but you might.
  inline Vector* getRow( int row ) const {
    return (Vector*)m[ row ] ;
  }
  
  #pragma endregion

  #pragma region <STATIC CONSTRUCTION ROUTINES>
  // You can't declare members of a union as static.
  // Well!  Who woulda guessed.  You can however 
  // declare the class as a struct with an anonymous
  // union in it.
  // static Matrix identity ;
  // static Matrix GetIdentity() ; // NO POINT -- just
  // call Matrix() base ctor to get an identity matrix.

  static Matrix RotationX( real radians ) ;

  static Matrix RotationY( real radians ) ;

  static Matrix RotationZ( real radians ) ;

  /// Arbitrary axis angle rotation
  static Matrix Rotation( const Vector & u, real radians ) ;
  
  // Order: roll, pitch, yaw
  // z-axis, x-axis, y-axis,
  static Matrix RotationYawPitchRoll( real yawRadians, real pitch, real roll ) ;

  // TRANS * ROT= spinning in place (because ROTATE about axis (we are still @ origin), then 
  // ROT * TRANS= orbit (because trans FIRST, then rotate about the axis (which is now far away) )
  static Matrix Translate( const Vector & v ) ;

  static Matrix Translate( real x, real y, real z ) ;

  static Matrix Scale( real x, real y, real z ) ;

  static Matrix Scale( const Vector& v ) ;

  // The backward (rasterization) version of the LookAt matrix transformation.
  // This matrix will be applied to EVERY VERTEX, not the viewing point itself,
  // so it's the inverse of LookAtFORWARD
  static Matrix LookAt( const Vector & eye, const Vector & look, const Vector & up ) ;

  // The forward (raytracing) version of the look at matrix.
  // Used to move the 4 points of the viewing plane and the EYE into
  // their right position in space for ray tracing.
  static Matrix LookAtFORWARD( const Vector & eye, const Vector & look, const Vector & up ) ;

  // Two projection matrices
  static Matrix ProjOrtho( real l, real r, real b, real t, real n, real f ) ;

  static Matrix ProjPerspectiveRH( real fieldOfViewRadians, real aspectRatio, real n, real f ) ;

  static Matrix ProjPerspectiveFrustum( real l, real r, real t, real b, real zn, real zf ) ;

  // INVERSE of the TransformToFace
  static Matrix ViewingFace( const Vector& right, const Vector& up, const Vector& forward ) ;

  static Matrix TransformToFace( const Vector& right, const Vector& up, const Vector& forward ) ;

  // Define d3d stuff
  ///static Matrix FromXMMatrix( XMMATRIX *mat ) ;
  #pragma endregion </STATIC CONSTRUCTION ROUTINES>

  #pragma region <CONST>
  void print() const;

  /// Checks for EXACT equality.  Use with
  /// caution, most of the time you should
  /// be using Near, and not this operator.
  bool operator==( const Matrix& b ) const ;

    /// Checks for ANY difference between corresponding
  /// elements of this and b.  Usually you should
  /// use !Near(b) instead.
  bool operator!=( const Matrix& b ) const ;

    /// Checks that all entries in this are
  /// Near (within 1e-6) of the corresponding
  /// entries in b.
  bool Near( const Matrix& b ) const ;

    /// You specify epsilon, instead of
  /// using the built in value.
  bool Near( const Matrix& b, real eps ) const ;

    /// Checks if the 3x3 submatrix this
  /// is Near the 3x3 submatrix in b
  bool Near3x3( const Matrix& b ) const ;

  Matrix transposedCopy() const ;

  // returns transpose of inverse of matrix.
  // Leaves rotation matrix unchanged, while inverting the scale.
  // http://explodedbrain.livejournal.com/112906.html
  Matrix getNormalMatrix() const ;

  real cofactor( int row, int col ) const ;

  /// Gets you the determinant of
  /// this matrix, defined on the
  /// _11 .. _33 3x3 submatrix
  real det() const ;
  
  // Inverts the matrix
  Matrix operator!() const ;

  void writeFloat16RowMajor( float * arr ) const
  {
    /*
    int i = 0 ;
    for( int row = 0 ; row < 4 ; row++ )
    {
      for( int col = 0 ; col < 4 ; col++ )
      {
        //int i = (row*4)+col ;  //ROWMAJOR WALK ACROSS ROWS (IE STEP COLUMN IN MINOR INCREMENT)
        // The matrices here are stored 
        // COLUMN MAJOR WOULD BE: (col*4)+row;
        //arr[i++] = (float)m[ col ][ row ] ; // TRANSPOSES IT
        arr[i++] = (float)m[ row ][ col ] ; // true order
      }
    }
    */

    for( int i = 0 ; i < 16 ; i++ )
      arr[i] = a[i];
  }

  #pragma endregion </CONST>

  #pragma region <NON-CONST>
  /// Makes the matrix as UPPER TRIANGULAR
  /// as it can, by only doing row swap
  /// operations and nothing else
  Matrix& triangularize() ;

  Matrix& swapRows( int row1, int row2 ) ;
  
  Matrix& rowAdd( int srcRow, int dstRow, real multiplier ) ;

  Matrix& rowMult( int row, real mult ) ;
  
  Matrix& invert() ;

  /// Rounds the entries of this matrix
  /// down to exactly 0.0 where they are
  /// close enough to 0.0
  Matrix& roundOff() ;
  
    /// Transposes the matrix.
  Matrix& transpose() ;

  Matrix& operator+=( const Matrix& b ) ;

  Matrix& operator+=( real scalar ) ;
  
  Matrix& operator-=( const Matrix& b ) ;

  Matrix& operator-=( real scalar ) ;
  
  Matrix& operator*=( const Matrix& b ) ;

  Matrix& operator*=( real scalar ) ;

  Matrix& operator/=( real scalar ) ;
  #pragma endregion </NON-CONST>
} ;

#pragma region outside operators

// unions cannot have static members,
// so this is the next best thing.
extern Matrix Identity ;

Matrix operator+( const Matrix & a, const Matrix & b ) ;

Matrix operator+( const Matrix & a, real scalar ) ;

Matrix operator+( real scalar, const Matrix & a ) ;

Matrix operator-( const Matrix & a, const Matrix & b ) ;
Matrix operator-( const Matrix & a, real scalar ) ;
Matrix operator-( real scalar, const Matrix & a ) ;

Matrix operator*( const Matrix & a, const Matrix & b ) ;
Matrix operator*( const Matrix & a, real scalar ) ;
Matrix operator*( real scalar, const Matrix & a ) ;



#pragma endregion

#endif
