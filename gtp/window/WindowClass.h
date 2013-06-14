#ifndef WINDOW_CLASS_H
#define WINDOW_CLASS_H

// windows
#include <windows.h>
#include <tchar.h>

// stl
#include <stack>

#include "../util/StdWilUtil.h"
using namespace std ;

// Prototype for the Windows main callback
LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam ) ;

class Window
{ 
protected:
  // Windows
  HINSTANCE hInstance ;
  HWND hwnd ;
  
  stack<char*> directories ;
  bool focus ;

public:
  Window( HINSTANCE hInst, TCHAR* windowTitleBar, int windowXPos, int windowYPos, int windowWidth, int windowHeight ) ;
  ~Window() ;

  virtual void activate() ;
  virtual void deactivate() ;
  virtual bool setSize( int width, int height, bool fullScreen ) ;
  
  virtual int getWidth() ;
  virtual int getHeight() ;

  /// Switches you into a working directory
  void cd( char *path );

  /// Takes you back to the directory you were
  /// in previously (equivalent to "back button"
  /// in windows explorer.)
  bool cdPop();

  bool open( char* dlgTitle, char* initDir, char* filterStr, char * resultFile ) ;

  bool savefile( char* dlgTitle, char* initDir, char* filterStr, char* resultFile ) ;
} ;





#endif // WINDOW_CLASS_H