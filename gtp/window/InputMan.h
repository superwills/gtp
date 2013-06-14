#ifndef INPUTMAN_H
#define INPUTMAN_H

#include <windows.h>
#include "../util/StdWilUtil.h"
#include "Mouse.h"
#include "Keyboard.h"

class InputMan
{
private:
  Keyboard keyboard ;  /*!< manages keyboard state */
  Mouse mouse ;        /*!< manages mouse state */

protected:
  /** Fire up the input manager */
  void initInputMan( HWND hwnd, int windowWidth, int windowHeight );

  /** Update all the input devices */
  void inputManStep() ;

public:
  #pragma region mouse
  /** Gets you the mouse's x location */
  int getMouseX() ;

  /** Gets you the mouse's CHANGE in x since last frame */
  int getMouseDx() ;

  /** Gets you the mouse's current y location */
  int getMouseY() ;

  /** Gets you the mouse's CHANGE in y since last frame */
  int getMouseDy() ;

  /// Send the mouse
  /// updated raw input (sent from
  /// a Windows message)
  /// @param raw The RAWINPUT* pointer from Windows
  void mouseUpdateInput( RAWINPUT *raw ) ;

  /** The mouse was just clicked (once) */
  bool mouseJustPressed( Mouse::Button button ) ;

  /** The mouse button is being held down */
  bool mouseIsPressed( Mouse::Button button ) ;

  /** Mouse-up:  The mouse button was just released */
  bool mouseJustReleased( Mouse::Button button ) ;
  #pragma endregion

  #pragma region keyboard
  /// Tells you if "key" was JUST PRESSED DOWN.
  /// 
  /// JUST PRESSED DOWN means the key was
  /// UP in previous frame, but is DOWN in
  /// THIS frame.
  /// 
  /// If you press and hold a key, for no matter
  /// how many seconds you hold it down for,
  /// you'll only have JustPressed return true ONCE.
  /// @param vkCode The virtual keycode.
  bool keyJustPressed( int vkCode ) ;

  /// Tells you if a key is BEING HELD DOWN
  /// @param vkCode The virtual keycode (starts
  /// with VK_*, listing at 
  /// http://msdn.microsoft.com/en-us/library/ms927178.aspx).
  /// Don't use the VK_Mousebutton ones, use
  /// mouseJustPressed for that.
  bool keyIsPressed( int vkCode ) ;

  /// Returns true if a key was JUST let go of.
  /// A complimentary function to justPressed()
  bool keyJustReleased( int vkCode ) ;

  /// Returns true when
  /// ANY KEY on the keyboard,
  /// mouse, or joypads have
  /// been pushed.
  bool anyKeyPushed() ;
  #pragma endregion

  /** Stop the mouse from going out of
      clipZone's bounds.
      @param clipZone A rectangle that
      describes how far left, right, up and down
      the mouse can go in pixels */
  void inputManSetClipZone( RECT & clipZone ) ;

} ;

#endif

















