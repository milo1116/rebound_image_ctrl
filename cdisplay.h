#ifndef _CDISPLAY_
#define _CDISPLAY_

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <unistd.h>

#include "crect.h"
#include <VG/openvg.h>
#include <VG/vgu.h>
#include "gl/fontinfo.h"
#include "gl/shapes.h" 

#define _COLOR_1_ 77,65,32			// Text and Button Outlines
#define _COLOR_2_ 255,249,212		// Background Color of Text Areas

#define _COLOR_3_ 125,194,66		// Lighter Green
#define _COLOR_4_ 232,217,189		// Background Color of Text Areas Gradient

#define _COLOR_5_ 245,245,245		// Light Text Button Outline Color...

#define _COLOR_6_ 120,120,120		// Welcome Screen Text...
#define _COLOR_7_ 150,33,23			// Red X to remove checkout Item...

#define _COLOR_8_ 255,255,255		// White ...
#define _COLOR_9_ 51,166,71			// Darker Green...
#define _COLOR_10_ 255,50,50		// Bright Red Error...

#define _BUTTON_CORNER_RADIUS_ 		9
#define _MARGIN_ 					6

////////////////////////////////////////////////////////////////////////
//
//	CDisplay class
//	Ths class wraps OpenVG and implements an interfavce for the
//	Rebound Viewer 
//
////////////////////////////////////////////////////////////////////////

class CDisplay
{
public:
	CDisplay();
	~CDisplay();
	
	int		mWidth;
	int		mHeight;
	bool	mBlank;
	
	
	void 	Init( void );
	void	Hide( void );

	const char* ScreenName( void );

	void 	DrawScreen( bool finish = true );
	void 	GradientRoundRect( CRect *r, int r1, int g1, int b1, int r2, int g2, int b2 );
	void 	GradientRoundRect( int x1, int y1, int x2, int y2, int r1, int g1, int b1, int r2, int g2, int b2 );
	void	TextOutRight( int x, int y, char* text, int size, bool color = true );
	void	TextOutLeft( int x, int y, char* text, int size, bool color = true );
	void	TextOutCenter( int x, int y, char* text, int size, bool color = true );
	void	FillRect(int x1, int y1, int x2, int y2);
	
	int 	TextOutWrap( CRect *r, char* text, int size, bool color = true, bool count = false );
	int 	TextOutWrapL( CRect *r, char* text, int size, bool color = true, bool count = false );
	int 	TextOutWrapR( CRect *r, char* text, int size, bool color = true, bool count = false );

	void 	DrawImage( int x, int y, int w, int h, char* file );

	void	DrawLine( int x1, int y1, int x2, int y2 );
	void	DrawButton( CRect *r, int x1, int y1, int x2, int y2, char* text );
	void 	Button( CRect *r, int r1, int g1, int b1, int r2, int g2, int b2, char* text, int size );
	void	ShiftButton( CRect *s, int x, int y, CRect *d );
	void	DrawCtrl( char* name, int x, int y, int val );
	
};

#endif
