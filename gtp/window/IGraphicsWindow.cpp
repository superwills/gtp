#include "IGraphicsWindow.h"

IGraphicsWindow::IGraphicsWindow( HINSTANCE hInst, TCHAR* windowTitleBar,
                     int windowXPos, int windowYPos,
                     int windowWidth, int windowHeight ) :
Window( hInst, windowTitleBar,
        windowXPos, windowYPos,
        windowWidth, windowHeight )
{
}
