#include "InputMan.h"

/////////////// PRIVATE ///////////////////
void InputMan::initInputMan( HWND hwnd, int windowWidth, int windowHeight )
{
  info( "Starting up rawinput devices..." ) ;

  // After the window has been created, 
  // register raw input devices
  RAWINPUTDEVICE Rid[2] ;
        
  Rid[0].usUsagePage = 0x01 ;
  Rid[0].usUsage = 0x02 ;
  
  Rid[0].dwFlags = 0 ; // (use this if you DO NOT WANT to capture mouse)
  //Rid[0].dwFlags = RIDEV_CAPTUREMOUSE |  RIDEV_NOLEGACY ;  // (use this to CAPTURE MOUSE)

  // If you are capturing mouse, then you should hide the
  // system default cursor and show your own.
  //ShowCursor( FALSE ) ;

  // RIDEV_CAPTUREMOUSE makes it so we
  // SEIZE UP THE MOUSE from the rest of the system.
  // This makes it so the user cannot accidently "click away"
  // from your app, which is good.

  // If you want the mouse to be "captured",
  // just set Rid[0].dwFlags=0;
  // You should also comment out the line
  // that says ShowCursor( FALSE ) ; if you
  // want the normal windows white mouse cursor
  // to show

  // To use this mode we must also specify RIDEV_NOLEGACY mode.
  // RIDEV_NOLEGACY makes it so we WILL NOT
  // get WM_LBUTTONDOWN ("old-school" aka 'legacy') messages.
  // Instead we will just get WM_INPUT messages.
  Rid[0].hwndTarget = hwnd ;

  // We don't really need raw input for the keyboard
  // but it is nice to have it hooked up because
  // 
  Rid[1].usUsagePage = 0x01 ;
  Rid[1].usUsage = 0x06 ;
  Rid[1].dwFlags = 0 ; // RIDEV_NOHOTKEYS ;  // use the RIDEV_NOHOTKEYS
  // option to turn off WINKEY
  
  // Also, for the keyboard, DO NOT specify RIDEV_NOLEGACY.
  // If you do, you will no longer be able to "hear"
  // WM_CHAR messages.  WM_CHAR messages are the best and
  // easiest way to get correct typing keystrokes
  // with UpPerCasEd aNd LowErCaseD letters as the user typed them.
  Rid[1].hwndTarget = hwnd ;

  if( !RegisterRawInputDevices( Rid, 2, sizeof(Rid[0]) ) )
  {
    //registration failed. Check your Rid structs above.
    printWindowsLastError( "RegisterRawInputDevices" ) ;
    bail( "Could not register raw input devices. Check your Rid structs, please." ) ;
  }

  
  // start the mouse in the middle of the screen
  mouse.setX( windowWidth / 2 ) ;
  mouse.setY( windowHeight / 2 ) ;

  // set the clipzone to match initialized window size
  RECT clipZone = { 0, 0, windowWidth, windowHeight } ;
  mouse.setClipZone( clipZone ) ;


}


// step the keyboard and mouse
void InputMan::inputManStep()
{
  keyboard.step() ;
  mouse.step() ;
}

#pragma region mouse
int InputMan::getMouseX()
{
  return mouse.getX() ;
}
int InputMan::getMouseDx()
{
  return mouse.getDx() ;
}
int InputMan::getMouseY()
{
  return mouse.getY() ;
}
int InputMan::getMouseDy()
{
  return mouse.getDy() ;
}

void InputMan::mouseUpdateInput( RAWINPUT * raw ) 
{
  mouse.updateInput( raw ) ;
}
bool InputMan::mouseJustPressed( Mouse::Button button )
{
  return mouse.justPressed( button ) ;
}
bool InputMan::mouseIsPressed( Mouse::Button button )
{
  return mouse.isPressed( button ) ;
}
bool InputMan::mouseJustReleased( Mouse::Button button )
{
  return mouse.justReleased( button ) ;
}
#pragma endregion

#pragma region keyboard
// Returns true if key is DOWN this frame
// and was UP previous frame.
bool InputMan::keyJustPressed( int vkCode )
{
  return KEY_IS_DOWN( keyboard.keyCurrentStates, vkCode ) &&  // Down this frame, &&
         KEY_IS_UP( keyboard.keyPreviousStates, vkCode ) ;     // AND up previous frame
  
  // bitwise AND logical AND's used here.

  // Two important bits of information from
  // GetKeyboardState() documentation on MSDN:
  // "If the high-order bit is 1,
  //  the key is down;
  //  otherwise, it is up."

  // "The low-order bit is meaningless for non-toggle keys."

}

// Tells you if a key is BEING HELD DOWN
bool InputMan::keyIsPressed( int vkCode )
{
  return KEY_IS_DOWN( keyboard.keyCurrentStates, vkCode ) ;
}

// Returns true if a key was JUST let go of.
// A complimentary function to justPressed()
bool InputMan::keyJustReleased( int vkCode )
{
  return KEY_IS_DOWN( keyboard.keyPreviousStates, vkCode ) &&  // Key __WAS__ down AND
         KEY_IS_UP( keyboard.keyCurrentStates, vkCode ) ;      // KEY IS UP NOW
}

bool InputMan::anyKeyPushed()
{
  // just use memcmp to compare previous states
  // of each mouse, keyboard, gamepad structs
  // to see if state changed
  return keyboard.anyKeyPushed() ;
}
#pragma endregion

void InputMan::inputManSetClipZone( RECT & clipZone )
{
  mouse.setClipZone( clipZone ) ;
}




