#include "WindowClass.h"

Window::Window( HINSTANCE hInst, TCHAR* windowTitleBar, int windowXPos, int windowYPos, int windowWidth, int windowHeight )
{
  // Save off these parameters in private instance variables.
  hInstance = hInst ;

  // Create a window.
  WNDCLASSEX window = { 0 } ;
  window.cbSize			= sizeof( WNDCLASSEX ) ;
  window.hbrBackground	= (HBRUSH)GetStockObject( WHITE_BRUSH );
  window.hCursor = LoadCursor( NULL, IDC_ARROW ) ;
  window.hIcon = LoadIcon( NULL, IDI_APPLICATION ) ;
  window.hIconSm = LoadIcon( NULL, IDI_APPLICATION ) ;
  window.hInstance = hInstance ;
  window.lpfnWndProc = WndProc ;
  window.lpszClassName = TEXT( "gtpmain" ) ;
  window.lpszMenuName = NULL;
  window.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC ;

  if(!RegisterClassEx( &window ))
  {
    bail( "Something's wrong with the WNDCLASSEX structure you defined.. quitting" ) ;
  }

  RECT wndSize ;
  wndSize.left = windowXPos ;
  wndSize.right = windowXPos + windowWidth ;
  wndSize.top = windowYPos ;
  wndSize.bottom = windowYPos + windowHeight ;

  AdjustWindowRectEx( &wndSize, WS_OVERLAPPEDWINDOW, NULL, 0 ) ;

  // Create the main window
  hwnd = CreateWindowEx(

    0 /*WS_EX_TOPMOST*/,  // extended window style.. if set to WS_EX_TOPMOST, for example,
    // then your window will be the topmost window all the time.  Setting it to 0
    // will make your window just another regular everyday window (not topmost or anything).

    TEXT( "gtpmain" ),   // window class name

    windowTitleBar,      // text in title bar of your window

    WS_OVERLAPPEDWINDOW, // window style.  Try using WS_POPUP, for example.

    wndSize.left, wndSize.top,
    wndSize.right - wndSize.left, wndSize.bottom - wndSize.top,

    NULL, NULL,
    hInstance, NULL
  ) ;
  if( !hwnd )
  {
    bail( "Couldn't createwindowex!" ) ;
  }
  ShowWindow( hwnd, SW_SHOWNORMAL ) ;
  UpdateWindow( hwnd ) ;
}

Window::~Window()
{
  // ... clean up and shut down ... 
  info( "Window destroy" ) ;

  while( !directories.empty() )
  {
    free( directories.top() ) ;
    directories.pop() ;
  }


  // cheesy fade exit
  AnimateWindow( hwnd, 200, AW_HIDE | AW_BLEND );
}

void Window::activate()
{
  //info( "WINDOW activate" ) ;
  focus = true ;
}
void Window::deactivate()
{
  focus = false ;
}
  

bool Window::setSize( int width, int height, bool fullScreen )
{
  info( "WindowClass resizing window to %d %d", width, height ) ;

  RECT wndSize ;
  wndSize.left = 0 ;
  wndSize.right = width ;
  wndSize.top =  0 ;
  wndSize.bottom = height ;

  // tell the mouse about the resize, before you
  // adjust the rect

  // We have to AdjustWindowRectEx() so the client area
  // is exactly the right size
  AdjustWindowRectEx( &wndSize, WS_OVERLAPPEDWINDOW, NULL, 0 ) ;

  SetWindowPos( hwnd, HWND_TOP, 0, 0, wndSize.right - wndSize.left, wndSize.bottom - wndSize.top, 
    SWP_NOMOVE | SWP_NOZORDER ) ;

  return true ;
}

int Window::getWidth()
{
  RECT r ;
  GetClientRect( hwnd, &r ) ;
  return r.right - r.left ;
}

int Window::getHeight()
{
  RECT r ;
  GetClientRect( hwnd, &r ) ;
  return r.bottom - r.top ;
}

/// Switches you into a working directory
void Window::cd( char *path )
{
  if( !path )
  {
    error( "You can't change directory to NULL, nothing done" ) ;
    return ;
  }

  // Save the current directory to the stack
  char *cwd = (char*)malloc( MAX_PATH ) ;
  GetCurrentDirectoryA( MAX_PATH, cwd ) ;
  directories.push( cwd ) ;
  
  if( !SetCurrentDirectoryA( path ) )
  {
    error( "Couldn't switch directory to %s", path ) ;
  }
  else
  {
    // This verifies the cd command worked
    char nowDir[MAX_PATH];
    GetCurrentDirectoryA( MAX_PATH, nowDir ) ;
    info( "Current working directory is '%s'", nowDir ) ;
  }
}

/// Takes you back to the directory you were
/// in previously (equivalent to "back button"
/// in windows explorer.)
bool Window::cdPop()
{
  if( directories.empty() )
  {
    error( "You are already at the top level directory" ) ;
    return false ;
  }
  
  if( !SetCurrentDirectoryA( directories.top() ) )
  {
    error( "Couldn't switch directory to %s", directories.top() ) ;
  }

  free( directories.top() ) ;
  directories.pop() ;
  return true ;
}
bool Window::open( char* dlgTitle, char* initDir, char* filterStr, char * resultFile ) 
{
  OPENFILENAMEA ofn = { 0 };
    
  ofn.Flags = OFN_FILEMUSTEXIST |  // file user picks must exist, else dialog box won't return
              OFN_PATHMUSTEXIST;  // path must exist, else dialog box won't return

  ofn.hInstance = hInstance ;
  ofn.hwndOwner = hwnd;

  ofn.lStructSize = sizeof(OPENFILENAMEA);
  
  ofn.lpstrTitle = dlgTitle ;
  ofn.lpstrInitialDir = initDir;
  ofn.lpstrFilter = filterStr; //"md2 files (*.md2)\0*.md2\0All files (*.*)\0*.*\0\0";

  ofn.lpstrFile = resultFile;   // ptr to string that will contain 
                                // FILE USER CHOSE when call to
                                // GetOpenFileNameA( &ofn ) returns.

  ofn.nMaxFile  = MAX_PATH;     // length of the resultFile string

  return (GetOpenFileNameA( &ofn ));   // GetOpenFileName returns false
                                       // when user clicks cancel, or if err
}

bool Window::savefile( char* dlgTitle, char* initDir, char* filterStr, char* resultFile )
{
  OPENFILENAMEA ofn = { 0 };
    
  ofn.Flags = OFN_OVERWRITEPROMPT |  // prompts in case selects file that exists
              OFN_PATHMUSTEXIST;  // path must exist, else dialog box won't return

  ofn.hInstance = hInstance ;
  ofn.hwndOwner = hwnd;

  ofn.lStructSize = sizeof(OPENFILENAMEA);
    
  ofn.lpstrTitle = dlgTitle ;
  ofn.lpstrInitialDir = initDir;
  ofn.lpstrFilter = filterStr; //"md2 files (*.md2)\0*.md2\0All files (*.*)\0*.*\0\0";

  ofn.lpstrFile = resultFile;   // ptr to string that will contain 
                                // FILE USER CHOSE when call to
                                // GetOpenFileNameA( &ofn ) returns.

  ofn.nMaxFile  = MAX_PATH;     // length of the resultFile string

  return (GetSaveFileNameA( &ofn ));   // GetOpenFileName returns false
                                       // when user clicks cancel, or if err
}