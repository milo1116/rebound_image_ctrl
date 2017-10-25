#ifndef _CRECT_
#define _CRECT_

////////////////////////////////////////////////////////////////////////
//
//	CRect class
//
//	Implements a rectagle class with functions usefull for dealing with
//	rectagles on the screen.
//
////////////////////////////////////////////////////////////////////////

class CRect
{
public:
	int mTop, mBottom, mLeft, mRight;
	CRect();
	CRect(int,int,int,int);
	int		Width( void );
	int		Height( void );
	int 	XCenter( void );
	int 	YCenter( void );
	bool	PointIn( int x, int y );
	void	Set( CRect* src );
	void	Set( int x1,int y1,int x2,int y2 );
	void	DeflateRect( int x, int y );
	void	OffsetRect( int x, int y );
	void	CenterOn( CRect* src, int w, int h );
};

#endif
