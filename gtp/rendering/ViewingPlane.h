#ifndef VIEWINGPLANE_H
#define VIEWINGPLANE_H

#include "../math/Vector.h"
#include "../math/Matrix.h"
#include "../geometry/Ray.h"

struct ViewingPlane
{
  Vector a,b,c,d ;

  // last known eye pos
  Vector eye ;

  // rows and columns in the pixel grid
  int rows, cols ;

  // rays per pixel
  int rpp, sqrtrpp ;

  // distances to the near and far planes
  real nearPlane, // how far back the eye is from the near plane
    farPlane ; // distance rays travel before they are "too far" for the eye to see/resolve

  // b--a
  // c--d
  ViewingPlane( int iRows, int iCols, int iRpp )
  {
    // initialize the corners with default values
    resetCorners() ;

    rows = iRows ;  cols = iCols ;
    rpp = iRpp ;
    sqrtrpp = (int)sqrt( (float)rpp ) ;

    nearPlane = 1 ; // the distance of this doesn't really matter, nothing is clipped
    farPlane = 1000 ;
  }

  void resetCorners()
  {
    // Start with eye at origin, looking down -z.
    eye = Vector( 0,0,0 ) ;

    // That means the viewing plane's vertices are
    // at z=-nearPlane, THESE ARE z=0 INITIALLY
    a = Vector( sqrt2, sqrt2, nearPlane ) ;
    b = Vector(-sqrt2, sqrt2, nearPlane ) ;
    c = Vector(-sqrt2,-sqrt2, nearPlane ) ;
    d = Vector( sqrt2,-sqrt2, nearPlane ) ;
  }

public:

  // gets you the physical space vector location of a pixel
  // jitterW = -1 sets at left of pixel,
  // jitterW =  0 sets at center pixel
  // jitterW = +1 sets at right of pixel
  // ROWS AND COLS COUNT LEFT-TO-RIGHT, COLS TOP TO BOTTOM
  Vector getPixelLocation( int pRow, int pCol, real jitterRow, real jitterCol )
  {
    // find the vector representing pixel row/col
    // on the viewing plane
    // b--a
    // |  |
    // c--d

    Vector ba = a - b ; // right
    Vector bc = c - b ; // down

    //real phyW = ab.len() ;
    //real phyH = ad.len() ;

    // what % of the way from a to b are we asking for?
    // I allowed jitterRow to be between -1 and +1,
    // but really it should be between -.5 and +.5
    // (hence the divide by 2 below)
    real percRow = ( pRow + .5 + jitterRow/2 )/ rows ;
    real percCol = ( pCol + .5 + jitterCol/2 )/ cols ;

    //point on viewing plane ray is going thru
    Vector v = b + bc*percRow + ba*percCol ;

    //{ MutexLock me( window->scene->mutexDebugLines, INFINITE ) ;
    //  window->addDebugPoint( v, Vector(1,0,0) ) ; }

    return v ;
  }

public:
  // gets you one a jittered ray
  // where a pixel is divided into
  // subrows and subcols.
  // If you pass 0 and 0 for jitterRow,jitterCol
  // you get a ray through the CENTER of that pixel
  Ray getRay( int pRow, int pCol, real jitterRow, real jitterCol )
  {
    Vector pixelLoc = getPixelLocation( pRow, pCol, jitterRow, jitterCol ) ;
    Vector eyeToPixel = (pixelLoc - eye).normalize() ;
    return Ray( eye, eyeToPixel, farPlane ) ;
  }

  // Unlike rasterization, orienting the viewing plane is a 
  // FORWARD transformation.
  void orient( const Vector& iEye, const Vector& look, const Vector& up )
  {
    eye = iEye ;

    Matrix view = Matrix::LookAtFORWARD( eye, look, up ) ; // forward transformation
    
    // Move the viewing plane.
    a = a * view ;
    b = b * view ;
    c = c * view ;
    d = d * view ;
  }

  void orient( const Vector& iEye, const Vector& right, const Vector& up, const Vector& forward )
  {
    eye = iEye ;
    Matrix view = Matrix::TransformToFace( right,up,forward ) ; // forward transformation
    
    // Move the viewing plane.
    a = a * view ;
    b = b * view ;
    c = c * view ;
    d = d * view ;
  }

  // Sets the near plane distance and ray length
  // and also SIZE of near plane.
  // if the near plane is too small, you will get
  // a nearly orthographic-looking projection
  void persp( real fovyRadians, real aspect, real nearPlaneDist, real farPlaneDist )
  {
    nearPlane = nearPlaneDist ;
    farPlane  = farPlaneDist ;

    // compute the SIZE of the near plane
    // tan(fovy)=o/a, nearPlane is opp, HEIGHT is adj
    // a=o/tan(fovy)
    real h_nearPH = nearPlane * tan( fovyRadians/2 ) ; // half near plane width

    // aspect=w/h
    real h_nearPW = h_nearPH*aspect ;
    
    // Viewing plane:
    // b--a
    // |  |
    // c--d
    resetCorners() ;
    eye = Vector(0,0,0);
    a.z=b.z=c.z=d.z=nearPlane;
    a.x =d.x= h_nearPW ;
    b.x =c.x=-h_nearPW ;
    
    a.y =b.y= h_nearPH ;
    c.y =d.y=-h_nearPH ;

    info( Magenta, "Near plane width %.2f, h=%.2f", h_nearPW, h_nearPH ) ;
  }
} ;


#endif