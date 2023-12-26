#ifndef RICHEDITCTRL_H
#define RICHEDITCTRL_H

#include <windows.h>
#include <CommCtrl.h>
#include <Richedit.h>
#include <map>
using namespace std ;

#pragma comment( lib, "comctl32.lib" )

#include "StdWilUtil.h"

class RichEdit
{
  static bool commonControlsIsInit ;
  HWND parent, hwnd ;
    
public:
  RichEdit() ;

  RichEdit( int x, int y ) ;

  RichEdit( HWND hwndOwner ) ;

  void show() ;

private:
  HWND createParentWindow( int x, int y ) ;
  HWND createRichTextWindow( HWND parent ) ;
  void initCommonControls() ;

public:
  void append( char* ascii ) ;
  void append( char* ascii, COLORREF textColor, COLORREF bgColor ) ;
  void append( wchar_t* text ) ;

  void trackProgress( int id, char* text ) ;
  void updateProgress( int id, float completionRatio ) ;
  void killProgress( int id ) ;

  void clear() ;

  void setCaret( int pos ) ;
  void setCaretGoBack() ;
  void setCaretEnd() ;
  void replace( CHARRANGE range, TCHAR* text ) ;
  long getCurrentPos() ;

  void setColor( COLORREF textColor, COLORREF bgColor ) ;

  void setTextColor( COLORREF textColor ) ;

  void setBgColor( COLORREF bgColor ) ;

  void setPos( int x, int y ) ;

  void setPos( int x, int y, int w, int h ) ;

  void setSize( int w, int h ) ;
 
} ;

LRESULT CALLBACK RichEditWndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam ) ;

#endif