#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Windows.h>

class Keyboard
{
public:
  BYTE keyPreviousStates[ 256 ] ; // holds which keys were down last frame
  BYTE keyCurrentStates[ 256 ] ; // holds which keys are down right now

  Keyboard()
  {
    // clear keystates
    memset( keyCurrentStates, 0, 256 ) ;
    memset( keyPreviousStates, 0, 256 ) ;

    // initialize states
    GetKeyboardState( keyCurrentStates ) ;
    memcpy( keyPreviousStates, keyCurrentStates, 256 ) ;
  }

  void step()
  {
    // Copy over "current" states to "previous" states
    memcpy( keyPreviousStates, keyCurrentStates, 256 ) ;

    // Grab all keystates, to know what the user is currently pushing down.
    if( !GetKeyboardState( keyCurrentStates ) )
    {
      printWindowsLastError( "GetKeyboardState()" ) ;
    }
  }

  /// Returns TRUE if (almost) any key was pushed
  /// between this frame and last frame
  bool anyKeyPushed()
  {
    //return memcmp( keyPreviousStates, keyCurrentStates, 256 ) != 0 ;
    // ^^This actually also detects changes
    // to mouse motion as well, which we probably don't want

    // Just check the letter, special, and digit keys
    for( int i = VK_BACK ; i < VK_RMENU ; i++ )
    {
      // Only the bit that survives
      // 0x80 mask matters
      if( (keyCurrentStates[i]  & KEY_DOWN_MASK) !=
          (keyPreviousStates[i] & KEY_DOWN_MASK) )
        return true ;
    }

    return false ;
  }
} ;


#endif