#ifndef CAMERA3D_H
#define CAMERA3D_H

#include "../util/StdWilUtil.h"
#include "../math/Vector.h"

// Really just bundles together EYE LOOK AND UP
// vectors, and provides some convenience methods
struct Camera
{
  /// For a sense of where
  /// it is in space
  Vector eye ; // look ; // look vector not necessary
  // since it is simply (eye + forward)
  // get it using the getter function

  int prepos ;

  // The x,y,z coordinate axis vectors
  Vector right, up, forward ;
  
  Camera()
  {
    prepos = 0 ;
    reset() ;
  }

  void reset()
  {
    // ASSUMES RIGHT-HANDED COORDINATE SYSTEM
    right = Vector( 1, 0, 0 ) ;
    up = Vector( 0, 1, 0 ) ;
    forward = Vector( 0, 0, -1 ) ;

    eye = Vector( 0, 0, 4 ) ;
  }

  // THIS IS THE POINT IN SPACE WHERE YOU ARE LOOKING.
  Vector getLook() const {
    return eye + forward ; // That is the POINT 'you looking at'
  }

  void setLook( Vector iLook )
  {
    // setting the look point means
    // you're setting where (eye + forward) is,
    // but (forward) is a unit vector, so
    // we'll just normalize it
    forward = (iLook - eye).normalize() ;

    // now you have to re-find up and right
    right = forward << up ;
    up = right << forward ;

    renormalizeVectors() ;
  }
  // convenience
  void setLook( real x, real y, real z ){
    setLook( Vector(x,y,z) ) ;
  }

  Matrix getViewMatrix() {
    return Matrix::LookAt( eye, getLook(), up ) ;
  }
  
  // Right, up, forward vectors
  // should always be normalized
  void renormalizeVectors() {
    right.normalize() ;  up.normalize() ;  forward.normalize() ;
  }

  void stepForward( real increment )
  {
    eye += increment*forward ;
  }

  void stepSide( real increment )
  {
    // Step towards the right
    eye += increment*right ;
  }

  /// Step horizontal yaw
  /// + is left, - is right.
  void stepYaw( real increment )
  {
    Matrix rotMat = Matrix::Rotation( up, increment ) ;

    forward = forward * rotMat ;
    //right = rotMat << right ; // you could just recompute this
    right = forward << up ;

    // renormalize these vectors
    // to make sure they stay unit vectors
    renormalizeVectors() ;
  }

  /// Step vertical pitch
  /// by an increment.  + is up, - is down.
  void stepPitch( real increment )
  {
    Matrix rotMat = Matrix::Rotation( right, increment ) ;

    forward = forward * rotMat ;
    up = right << forward ;
    renormalizeVectors() ;
  }

  void roll( real rads )
  {
    // rotate up and recalc right
    up = up * Matrix::Rotation( forward, rads ) ;
    right = forward << up ;
    renormalizeVectors() ;
  }

  void follow( const Vector& objectPos, const Vector& heading, real back, real height )
  {
    // to follow something @ objectPos, going heading
    // we want, offset from objectPos "back" units, and "height" units in the air.

    // Set the eye of the camera to being
    // @ (pos+offset)
    
    
    eye = objectPos - (back*heading) ;
    eye.z += height ;

    // Set the fwd of the camera as just
    // being equal to the passed in `heading' vector
    forward = heading ;

    // The right vector should be
    // maintained as well
    right = forward << up ;

    renormalizeVectors() ;
    
  }


  void setPrepos( int id )
  {
    reset() ;
    switch( id )
    {
      default:
        warning( "Invalid prepos %d", id ) ;
      case 0:
        // position 1
        eye = Vector( -7.8, 5.2, 7.8 ) ; // start point
        setLook( Vector(0,2,0) ) ;
        break ;
      case 1:
        // position 2
        eye = Vector( -13.4, 10.2, 13.4 ) ; // start point
        setLook( Vector(0,0,0) ) ;
        break ;
      case 2:
        eye = Vector( -19.56, 10.03, 19.56 ) ;
        setLook( Vector(0,2,0) ) ;
        break ;
      case 3:
        eye = Vector( 0, 14.46, 0 ) ;
        up = Vector( 0,0,-1 ) ;
        right = Vector( 1,0,0 ) ;
        forward = Vector( 0,-1,0 ) ;
        break ;
      case 4:
        eye = Vector( 0, 10.03, 17 ) ;
        setLook( Vector(0,2,0) ) ;
        break ;
      case 5:
        //eye = Vector( -12.3,7.05,12.3 ) ;
        //setLook( Vector(0,2,0) ) ;
        // lucy 2
        eye = Vector( -6.31, 20.11, 21.05) ;
        setLook( Vector(-5.98, 19.91, 20.13) ) ;
        break ;
      case 6:
        // teapot angle
        //eye = Vector( -3.63,5.36,4.23 ) ;
        //setLook( Vector(-1,1,0) ) ;
        // Lucy 1
        eye = Vector (-30.78, 26.47, 30.85) ;
        setLook( Vector(-30.13, 26.05, 30.22) ) ;
        break ;
      case 7:
        // lucy angle
        //eye = Vector( -11.55, 10.55, 17.66 ) ;
        //setLook( Vector(.5, 7, 1) ) ;
        break ;
      case 8:
        // above angle
        eye = Vector( 0, 18.5, 0 ) ;
        up = Vector( 0,0,-1 ) ;
        right = Vector( 1,0,0 ) ;
        forward = Vector( 0,-1,0 ) ;
        break ;
      case 9:
        // teapot spout for VO debug data
        eye = Vector(2.93, 2.33, 5.90), 
        setLook(Vector(2.82, 2.02, 4.96));
        break ;

      case 10:
        // dragon
        eye = Vector(-10.92, 13.78, 13.70) ;
        setLook( Vector(-10.35, 13.27, 13.05) ) ;
        break ;

      case 11:
        // shapes1 scene with torus looking like an ellipse
        eye=Vector(20.344615,22.251808,24.619772);
        forward=Vector(-0.615130,-0.334928,-0.713749);
        right=Vector(0.756596,0.003865,-0.653871);
        up=Vector(-0.221758,0.942236,-0.251028);
        break ;

      case 12:
        // in front of dragon
        eye=Vector(0.000000,10.622458,17.780692);
        forward=Vector(0.000000,-0.334238,-0.942489);
        right=Vector(1.000000,0.000000,0.000000);
        up=Vector(0.000000,0.942489,-0.334238);
        break ;

      case 13:
        eye=Vector(11.557138,20.681092,13.214835);
        forward=Vector(-0.629688,-0.156988,-0.760820);
        right=Vector(0.776615,-0.103235,-0.621459);
        up=Vector(-0.019018,0.982190,-0.186925);
        break ;

    }
  }
} ;

#endif // CAMERA3D_H
