#ifndef MOUSE_H
#define MOUSE_H

#include "../util/StdWilUtil.h"

// Facilitate use of mouse through
// Win32.
//
// Mouse input messages from windows are stateless.
//
// When you get a mouse input message from
// windows, its about an EVENT THAT HAPPENED
// AT THE MOUSE.
//
// Because input messages only arrive as EVENTS,
// we can't POLL the mouse through RAW INPUT.
//
// So instead we must remember
// when the mouse left button was pushed down for example,
// that the left mouse button IS DOWN _UNTIL_
// we get a left mouse button UP message.
//
// This class provides service
// for a few select buttons. You
// can expand the class to handle
// RI_MOUSE_BUTTON_4 and 5 if you wish.
class Mouse
{
private:
  int x, y ;   // x and y position of the mouse in the window.
  int dx, dy ; // difference between mouse pos last frame
  // and this frame

  // Keeps track of how many frames its been
  // since dx and dy last were updated.
  // WHEN this value exceeds 1, then
  // dx and dy are reset to 0.
  // This value will keep on getting pushed
  // back to 0 if the mouse is changing a lot
  // rapidly (every frame)
  int framesSinceLastChange ;

  RECT clipZone ;  // the bounding rectangle thta the mouse
  // isn't allowed to leave, even when you move it.

  bool leftHeldDown, rightHeldDown, middleHeldDown,
       leftJustPressed, rightJustPressed, middleJustPressed,
       leftJustReleased, rightJustReleased, middleJustReleased ;

public:
  Mouse()
  {
    // assume 640x480 clipzone
    clipZone.left = 0 ;
    clipZone.right = 640 ;
    clipZone.top = 0 ;
    clipZone.bottom = 480 ;

    // start mouse in middle
    x = 320 ;
    y = 240 ;

    dx = dy = 0 ;
    framesSinceLastChange = 0 ;

    leftHeldDown = rightHeldDown = middleHeldDown =
       leftJustPressed = rightJustPressed = middleJustPressed =
       leftJustReleased = rightJustReleased = middleJustReleased = false ;
  }

  #pragma region get move and set mouse pos
  int getX()
  {
    return x;
  }
  int getDx()
  {
    return dx;
  }
  int getY()
  {
    return y;
  }
  int getDy()
  {
    return dy;
  }

  void moveX( int dx )
  {
    x += dx ;
  
    // x-clamp to screen bounds
    if( x > clipZone.right ) // if its too far right
      x = clipZone.right ;   // slam it to the right edge of the clipZone
    else if( x < clipZone.left )     // if its too far left
      x = clipZone.left ;    // slam it to the left edge of the clipZone

  }

  void moveY( int dy )
  {
    y += dy ;

    // y clamp
    if( y > clipZone.bottom )
      y = clipZone.bottom ;
    else if( y < clipZone.top )
      y = clipZone.top ;
  }

  void setX( int ix )
  {
    x = ix ;
    // these don't check clip purposefully
    // so you CAN move the mouse offscreen
    // if you like
  }

  void setY( int iy )
  {
    y = iy ;
  }
  #pragma endregion

  void setClipZone( RECT newClipZone )
  {
    clipZone = newClipZone ;
  }

  enum Button
  {
    Left = RI_MOUSE_LEFT_BUTTON_DOWN,
    Right = RI_MOUSE_RIGHT_BUTTON_DOWN,
    Middle = RI_MOUSE_MIDDLE_BUTTON_DOWN
  } ;

  bool justPressed( Button mouseButton )
  {
    switch( mouseButton )
    {
    case Left:
      return leftJustPressed ;
    case Right:
      return rightJustPressed ;
    case Middle:
      return middleJustPressed ;

    default:
      warning("No such mouse button") ;
      return false ;
    }
  }

  bool justReleased( Button mouseButton )
  {
    switch( mouseButton )
    {
    case Left:
      return leftJustReleased ;
    case Right:
      return rightJustReleased ;
    case Middle:
      return middleJustReleased ;

    default:
      warning("No such mouse button") ;
      return false ;
    }
  }

  bool isPressed( Button mouseButton )
  {
    switch( mouseButton )
    {
    case Left:
      return leftHeldDown ;
    case Right:
      return rightHeldDown ;
    case Middle:
      return middleHeldDown ;

    default:
      warning("No such mouse button") ;
      return false ;
    }
  }

  // This function is only called
  // whenever there is a mouse message,
  // __not necessarily every frame__
  void updateInput( RAWINPUT *raw )
  {
    framesSinceLastChange = 0 ; // say that mouse just changed.
    
    // These are EVENTS, so they only occur
    // ONCE.

    leftJustPressed = raw->data.mouse.ulButtons & RI_MOUSE_LEFT_BUTTON_DOWN ;
    rightJustPressed = raw->data.mouse.ulButtons & RI_MOUSE_RIGHT_BUTTON_DOWN ;
    middleJustPressed = raw->data.mouse.ulButtons & RI_MOUSE_MIDDLE_BUTTON_DOWN ;

    leftJustReleased = raw->data.mouse.ulButtons & RI_MOUSE_LEFT_BUTTON_UP ;
    rightJustReleased = raw->data.mouse.ulButtons & RI_MOUSE_RIGHT_BUTTON_UP ;
    middleJustReleased = raw->data.mouse.ulButtons & RI_MOUSE_MIDDLE_BUTTON_UP ;

    // Now the "held down" states
    // If the mouse button IS NOT ALREADY
    // held down, then its current "held down"
    // state is the same as the JUST PRESSED
    // state.
    if( !leftHeldDown )
      leftHeldDown = leftJustPressed ;
    else
    {
      // If a mouse button IS BEING HELD DOWN,
      // then we have a chance to escape that
      // state is when the justReleased event
      // occurs for that mouse button

      //if( leftJustReleased )
      //  leftHeldDown = false ;
      
      // ((Logically same as above
      //   very light optimization))
      leftHeldDown = !leftJustReleased ;
    }


    if( !rightHeldDown )
      rightHeldDown = rightJustPressed ;
    else
      rightHeldDown = !rightJustReleased ;
    
    if( !middleHeldDown )
      middleHeldDown = middleJustPressed ;
    else
      middleHeldDown = !middleJustReleased ;    




    
    // usFlags is ALWAYS ABSOLUTE, see 
    if( raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE )
    {
      // its a 0 flag so
      // this is am real mouse
      dx = raw->data.mouse.lLastX ;
      dy = raw->data.mouse.lLastY ;
    }
    else if( containsFlag( raw->data.mouse.usFlags, MOUSE_MOVE_ABSOLUTE ) )
    {
      // its a tablet or positioning device
      // 1 .. 65537, ACROSS THE DESKTOP
      static int lastX = raw->data.mouse.lLastX ;
      static int lastY = raw->data.mouse.lLastY ;
      if( leftJustPressed )
      {
        // reset
        lastX = raw->data.mouse.lLastX ;
        lastY = raw->data.mouse.lLastY ;
      }


      if( leftHeldDown )
      {
        // get the relative change in position
        // to turn it into a pixel count,

        dx = (raw->data.mouse.lLastX - lastX) ;
        dy = (raw->data.mouse.lLastY - lastY) ;

        static int scW = GetSystemMetrics( SM_CXSCREEN ) ;
        static int scH = GetSystemMetrics( SM_CYSCREEN ) ;

        dx = (dx/65536.f)*scW ;
        dy = (dy/65536.f)*scH ;

        lastX = raw->data.mouse.lLastX ;
        lastY = raw->data.mouse.lLastY ;
        //info( "abs mouse %d %d", dx, dy ) ;
      }
    }
    
    
    // Move the mouse now
    moveX( dx );
    moveY( dy );
    
    

  }

  // Call this every frame of the game
  // to UNSET the justPressed, justReleased vars
  void step()
  {
    framesSinceLastChange++;

    // If its been exactly 1 frame
    // since there was an UPDATE
    // message (of ANY kind!) then
    // we reset the mouse's state
    // variables to their default values.
    // If successive frames occur where
    // there is a mouse update then
    // all these variables would be updated
    // by the updateInput function anyway
    if( framesSinceLastChange == 1 )
    {
      leftJustPressed = 
      rightJustPressed = 
      middleJustPressed = 

      leftJustReleased = 
      rightJustReleased = 
      middleJustReleased = false ;

      // we have nothing to say about
      // the *HeldDown states.

      // We also most reset the
      // dx and dy vars
      dx = dy = 0 ;
    }
    
   
  }
} ;

#endif MOUSE_H