#ifndef MAIN_H
#define MAIN_H

#include <Windows.h>

/// Loads a room and sets up the program
void init() ;

/// For moving the camera interactively
void update() ;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow );

// function for hittesting
void hittest( int x, int y ) ;

// function for rpp str
void rppStratifiedTest() ;

#endif