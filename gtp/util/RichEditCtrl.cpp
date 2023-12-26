#include "RichEditCtrl.h"
#include "../window/WindowClass.h"

bool RichEdit::commonControlsIsInit = false ;

RichEdit::RichEdit()
{
  initCommonControls() ;

  parent = createParentWindow( 32, 32 ) ;
  hwnd = createRichTextWindow( parent ) ;

  //ShowWindow( parent, SW_NORMAL ) ;
  //ShowWindow( hwnd, SW_NORMAL ) ;
}

// This is the one that runs
RichEdit::RichEdit( int x, int y ) 
{
  initCommonControls() ;

  parent = createParentWindow( x, y ) ;
  hwnd = createRichTextWindow( parent ) ;

  // On Windows XP, calling SHOW here (which results in win o/s
  // sending a WM_SIZE message) actually causes a bug-
  // where hwnd is STILL NULL /uninitialized in RichText::setSize().
  // Even reading it before the ctor has returned results in an
  // access control violation.
  //ShowWindow( parent, SW_NORMAL ) ;
  //ShowWindow( hwnd, SW_NORMAL ) ;
}

RichEdit::RichEdit( HWND hwndOwner )
{
  initCommonControls() ;

  hwnd = createRichTextWindow( hwndOwner ) ;

  //ShowWindow( hwnd, SW_NORMAL ) ;
}

void RichEdit::show()
{
  ShowWindow( parent, SW_NORMAL ) ;
  ShowWindow( hwnd, SW_NORMAL ) ;
}

HWND RichEdit::createParentWindow( int x, int y )
{
  // Get the hInstance associated with this thread
  HINSTANCE hInstance = (HINSTANCE)GetModuleHandle( 0 ) ;
    
  // Makes its own parent window and fills it
  WNDCLASSEX wc = { 0 };
  wc.cbSize = sizeof( WNDCLASSEX ) ;
  wc.hbrBackground = (HBRUSH)GetStockObject( WHITE_BRUSH );
  wc.hCursor = LoadCursor( NULL, IDC_ARROW );
  wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
  wc.hInstance = hInstance ;
  //wc.lpfnWndProc = (WNDPROC)DefWindowProc ;
  wc.lpfnWndProc = RichEditWndProc ;
  wc.lpszClassName = TEXT("richtext");
  wc.style = CS_HREDRAW | CS_VREDRAW;

  if( !RegisterClassEx(&wc) )
    puts( "RichEdit: Couldn't register richtext parent window class!" ) ;
  
  RECT rect = { x,y,x+750,y+1000 } ;//ltrb
  AdjustWindowRectEx( &rect, WS_OVERLAPPEDWINDOW, NULL, 0 ) ;

  parent = CreateWindowEx( 0, TEXT("richtext"), TEXT("log"),
    WS_OVERLAPPEDWINDOW,
    rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
    0, 0, hInstance, 0 );

  if( !parent )
    puts( "RichEdit: Couldn't create parent window!" ) ;

  return parent ;
}

HWND RichEdit::createRichTextWindow( HWND parent )
{
  HINSTANCE hInstance = (HINSTANCE)GetWindowLong( parent, -6/*GWL_HINSTANCE*/ ) ;
  RECT rect ;
  GetClientRect( parent, &rect );
  HWND rt = CreateWindowEx(0, MSFTEDIT_CLASS, TEXT("Log"),
      WS_VISIBLE | WS_CHILD | WS_VSCROLL |
      ES_MULTILINE | ES_LEFT | ES_NOHIDESEL | /*ES_AUTOVSCROLL |*/ ES_READONLY | ES_WANTRETURN,
      0, 0, // top aligned
      rect.right-rect.left, rect.bottom-rect.top, 
      parent, NULL, hInstance, NULL);
  if( !rt )
  {
    printWindowsLastError( "Couldn't create RT window!" ) ;
  }

  return rt ;
}

void RichEdit::initCommonControls()
{
  if( !commonControlsIsInit )
  {
    commonControlsIsInit = true ;
    InitCommonControls() ;
    LoadLibrary(TEXT("Msftedit.dll"));
  }
}

void RichEdit::append( char* ascii )
{
  setCaretEnd();
  // convert to unicode first
  wchar_t* unicode = getUnicode( ascii ) ;
  SendMessage( hwnd, EM_REPLACESEL, TRUE, (LPARAM)unicode ) ;
  delete unicode ;
  //setCaretGoBack() ;
}

void RichEdit::append( char* ascii, COLORREF textColor, COLORREF bgColor )
{
  setCaretEnd();
  setColor( textColor, bgColor ) ;
  wchar_t* unicode = getUnicode( ascii ) ;
  SendMessage( hwnd, EM_REPLACESEL, TRUE, (LPARAM)unicode ) ;
  delete unicode ;
  //setCaretGoBack() ;
}

void RichEdit::append( wchar_t* text )
{
  setCaretEnd() ;
  SendMessage( hwnd, EM_REPLACESEL, TRUE, (LPARAM)text ) ;
  //setCaretGoBack() ;
}

void RichEdit::clear()
{
  // clear it with SETTEXTEX
  SETTEXTEX st ;
  st.codepage = CP_ACP ; // ansi codepage
  st.flags = ST_KEEPUNDO ;

  SendMessage( hwnd, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)"" ) ; // OVERWRITES all text
}

void RichEdit::setCaret( int pos )
{
  //SendMessage( hwnd, EM_EXGETSEL, 0, (LPARAM)&lastPos ) ;//remember where the caret was
  
  // Move the caret to the end
  CHARRANGE crg = { pos, pos } ;
  SendMessage( hwnd, EM_EXSETSEL, 0, (LPARAM)&crg ) ;
}

void RichEdit::setCaretGoBack()
{
  // this doesn't actually work now
  //SendMessage( hwnd, EM_EXSETSEL, 0, (LPARAM)&lastPos ) ; // go back to last point remembered
}

void RichEdit::setCaretEnd()
{
  setCaret( LONG_MAX ) ;
}

void RichEdit::replace( CHARRANGE range, TCHAR* text )
{
  SendMessage( hwnd, EM_EXSETSEL, 0, (LPARAM)&range ) ;
  SendMessage( hwnd, EM_REPLACESEL, FALSE, (LPARAM)text ) ;
}

long RichEdit::getCurrentPos()
{
  CHARRANGE currentPos ;
  SendMessage( hwnd, EM_EXGETSEL, 0, (LPARAM)&currentPos ) ;
  return currentPos.cpMin;
}

void RichEdit::setColor( COLORREF textColor, COLORREF bgColor )
{
  CHARFORMAT2 cf ;

  cf.cbSize = sizeof( CHARFORMAT2 ) ;
  cf.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_EFFECTS2 ;
  
  cf.crTextColor = textColor ;
  cf.crBackColor = bgColor ;
  cf.dwEffects = CFE_BOLD ;
  
  SendMessage( hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf ) ;
}

void RichEdit::setTextColor( COLORREF textColor )
{
  CHARFORMAT2 cf ;

  cf.cbSize = sizeof( CHARFORMAT2 ) ;
  cf.dwMask = CFM_COLOR | CFM_EFFECTS2 ;
  
  cf.crTextColor = textColor ;
  cf.dwEffects = CFE_BOLD ;
  
  SendMessage( hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf ) ;
}

void RichEdit::setBgColor( COLORREF bgColor )
{
  CHARFORMAT2 cf ;

  cf.cbSize = sizeof( CHARFORMAT2 ) ;
  cf.dwMask = CFM_BACKCOLOR | CFM_EFFECTS2 ;
  
  cf.crBackColor = bgColor ;
  cf.dwEffects = CFE_BOLD ;
  
  SendMessage( hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf ) ;
}

void RichEdit::setPos( int x, int y )
{
  SetWindowPos( hwnd, NULL, x,y, 0,0, SWP_NOSIZE|SWP_NOZORDER ) ;
}

void RichEdit::setPos( int x, int y, int w, int h )
{
  MoveWindow( hwnd, x, y, w, h, true ) ;
}

void RichEdit::setSize( int w, int h )
{
  SetWindowPos( hwnd, NULL, 0,0, w,h, SWP_NOMOVE|SWP_NOZORDER ) ;
}
 
LRESULT CALLBACK RichEditWndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam )
{
  switch( message )
  {
  case WM_PAINT:
    {
      // we would place our Windows painting code here.
      HDC hdc;
      PAINTSTRUCT ps;
      hdc = BeginPaint( hwnd, &ps );
      EndPaint( hwnd, &ps );
    }
    return 0;
    break;

  case WM_SIZE:
    {
      int w = LOWORD( lparam ) ;
      int h = HIWORD( lparam ) ;
      if( logRichText )  logRichText->setSize( w, h ) ;
    }
    break;

  case WM_CHAR:
    switch( wparam )
    {
      case VK_ESCAPE: 
        PostQuitMessage( 0 ) ;
        break ;
    }
    break ;

  case WM_NCHITTEST:
  case WM_LBUTTONDOWN:
    {
    }
    break ;

  case WM_RBUTTONDOWN:
    {
    }
    break ;

  case WM_DESTROY:
    PostQuitMessage( 0 ) ;
    return 0;
    break;

  }

  return DefWindowProc( hwnd, message, wparam, lparam );
} 

// Aug 14 2011:  Changed all usage of SendMessage() to PostMessage()
//               (sendmessage hangs when you send a message from another thread
//               than the thread the window was created on)
// Aug 14 2011:  Actually you can't do that, you have to SendMessage() from
//               the thread on which you created the window.
