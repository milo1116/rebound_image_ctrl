#include "cdisplay.h"
#include "main.h"

extern int	gValues[KNOB_COUNT];
extern bool gShowStill;
extern int gStillNum;

char* knob_labels[] = {
	(char*)"Brightness",
	(char*)"Contrast",
	(char*)"Saturation",
	(char*)"Sharpness",
	(char*)"Move H",
	(char*)"Move V",
	(char*)"Rotate",
	(char*)"Zoom"
};

CDisplay::CDisplay()
{
	Init();
	mBlank = false;
};

CDisplay::~CDisplay()
{
	Hide();
};

void CDisplay::Init( void )
{
	init(&mWidth, &mHeight);			// Graphics initialization	
}

void CDisplay::Hide( void )
{
	finish();							// Graphics cleanup		
}

void CDisplay::DrawCtrl( char* name, int x, int y, int val )
{
	char file[128];
	sprintf(file,"./images/%s.jpg",name);
	DrawImage( x,y,433,145,(char*)"./images/control.jpg");
	DrawImage( x+15,y+17,399,64,file);
	int v = ((val*360)/1024);
	DrawImage( x+20+v,y+93,29,28,(char*)"./images/knob.jpg");
	
}

////////////////////////////////////////////////////////////////////////
//
//	DrawScreen()
//
//	Draws the Rebound Viewer interface on the right half of the screen,
//	or a black box if the mBlank is true. 
//
////////////////////////////////////////////////////////////////////////
void CDisplay::DrawScreen( bool finish )
{
	int w = mWidth;
	int h = mHeight;
	char temp[100];
	CRect dlg, tick, lab, head, foot;
	
	Start(mWidth, mHeight);
	
	Background(0,0,0);
	if( gShowStill ){
		printf( "Drawing Screen\n" );
		char file[128];
		
		// Upper Right
		sprintf( file, "./out%02d.jpeg", 2 ); //(gStillNum-3)%4 );
		if( access( file, F_OK ) != -1 ){
			DrawImage( 75*3+540*2,0,540,540,(char*)file);
		}
		// Lower Left
		sprintf( file, "./out%02d.jpeg", 1 ); //(gStillNum-2)%4 );
		if( access( file, F_OK ) != -1 ){
			DrawImage( 75*2+540*1,540,540,540,(char*)file);
		}
		// Lower Right
		sprintf( file, "./out%02d.jpeg", 3 ); //(gStillNum-0)%4 );
		if( access( file, F_OK ) != -1 ){
			DrawImage( 75*3+540*2,540,540,540,(char*)file);
		}
		// Upper Left
		sprintf( file, "./out%02d.jpeg", 0 ); //(gStillNum-1)%4 );
		if( access( file, F_OK ) != -1 ){
			DrawImage( 75*2+540*1,0,540,540,(char*)file);
		}
		
		if( access( "./std.jpeg", F_OK ) != -1 ){
			DrawImage( 75,540,540,540,(char*)"./std.jpeg");
		}
		
		if( finish ) End();
		return;
	}
	
	if( mBlank ){
		if( finish ) End();
		return;
	}
	
//	Fill(255,255,255,255);
//	FillRect(1040,0,w,h);
//	Fill(0,0,0,0);
	
	DrawImage( 1258,20,433,145,(char*)"./images/logo.jpg");	

	int x = 1040;
	int y = 200;
	
	DrawCtrl((char*)"brightness",x,y,gValues[4]); y+= 200;
	DrawCtrl((char*)"contrast",x,y,gValues[5]); y+= 200;
	DrawCtrl((char*)"saturation",x,y,gValues[6]); y+= 200;
	// replaced by light intensity MA 10.24.17: DrawCtrl((char*)"sharpness",x,y,gValues[7]); y+= 200;
	
	x = 1475;
	y = 200;
	
	DrawCtrl((char*)"moveh",x,y,gValues[0]); y+= 200;
	DrawCtrl((char*)"movev",x,y,gValues[1]); y+= 200;
	DrawCtrl((char*)"rotate",x,y,gValues[2]); y+= 200;
	DrawCtrl((char*)"zoom",x,y,gValues[3]); y+= 200;
	
	
	if( finish ) End();

	return;
	
	int i;
	
	dlg.mTop = 10;
	dlg.mLeft = 1040 + 20;
	dlg.mRight = 1900 - 20;
	dlg.mBottom = 1040 - 20;
	GradientRoundRect( &dlg, _COLOR_2_, _COLOR_4_ );

	head.Set(&dlg);
	head.mBottom = head.mTop + 140;
	head.DeflateRect(10,10);
	GradientRoundRect( &head, _COLOR_8_, _COLOR_8_ );
//	DrawImage( head.mLeft+20,head.mTop+5,193,114,(char*)"./images/reboundviewer.jpg");
	TextOutWrap(&head,(char*)"\nRebound Viewer",22);
	
	foot.Set(&dlg);
	foot.mTop = dlg.mBottom - 115;
	foot.DeflateRect(10,10);
	GradientRoundRect( &foot, _COLOR_8_, _COLOR_8_ );
	DrawImage( 1700,foot.mTop+5,115,81,(char*)"./images/rebound.jpg");
	TextOutWrap(&foot,(char*)"\nCopyright 2016, Rebound Therapeutics - V 0.5",14);
	
	lab.Set(&dlg);
	lab.mRight = lab.XCenter();
	lab.mBottom = lab.mTop + 150;
	lab.DeflateRect(10,10);
	
	// Draw the left 4 sliders...
	for( i = 0; i < 4; i++ ){
		lab.OffsetRect(0,160);
		GradientRoundRect( &lab, _COLOR_2_, _COLOR_4_ );
		tick.mTop = lab.mTop + 60;
		tick.mBottom = lab.mTop + 60 + 20;
		tick.mLeft = lab.mLeft + 10 + ((gValues[i+4]*(lab.Width()-40))/1024);
		tick.mRight = tick.mLeft+20;
		DrawLine(lab.mLeft + 20,lab.mTop + 70,lab.mRight-20,lab.mTop + 70);
		GradientRoundRect( &tick, _COLOR_2_, _COLOR_4_ );
		TextOutWrap(&lab,knob_labels[i],18);
	}
	
	lab.Set(&dlg);
	lab.mLeft = lab.XCenter();
	lab.mBottom = lab.mTop + 150;
	lab.DeflateRect(10,10);
	
	// Draw the right 4 sliders...
	for( i = 0; i < 4; i++ ){
		lab.OffsetRect(0,160);
		GradientRoundRect( &lab, _COLOR_2_, _COLOR_4_ );
		tick.mTop = lab.mTop + 60;
		tick.mBottom = lab.mTop + 60 + 20;
		tick.mLeft = lab.mLeft + 10 + ((gValues[i]*(lab.Width()-40))/1024);
		tick.mRight = tick.mLeft+20;
		DrawLine(lab.mLeft + 20,lab.mTop + 70,lab.mRight-20,lab.mTop + 70);
		GradientRoundRect( &tick, _COLOR_2_, _COLOR_4_ );
		TextOutWrap(&lab,knob_labels[i+4],18);
	}
	
	if( finish ) End();

}

////////////////////////////////////////////////////////////////////////
//
//	TextOutLeft()
//
//	Draws text, left justified in a rectangle.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::TextOutLeft( int x, int y, char* text, int size, bool color )
{
	if( color ) Fill(0,0,0,1);
	Text(x,mHeight-y-size-_MARGIN_,text,SansTypeface,size); 
};

////////////////////////////////////////////////////////////////////////
//
//	TextOutCenter()
//
//	Draws text, right justified in a rectangle.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::TextOutRight( int x, int y, char* text, int size, bool color )
{
	if( color ) Fill(0,0,0,1);
	TextEnd(x,mHeight-y-size-_MARGIN_,text,SansTypeface,size); 
};

////////////////////////////////////////////////////////////////////////
//
//	TextOutCenter()
//
//	Draws text, center justified in a rectangle.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::TextOutCenter( int x, int y, char* text, int size, bool color )
{
	if( color ) Fill(0,0,0,1);
	TextMid(x,mHeight-y-size-_MARGIN_,text,SansTypeface,size); 
};

////////////////////////////////////////////////////////////////////////
//
//	TextOutWrap()
//
//	Draws text, left justified in a rectangle, breaking the lines as
//	needed to fit.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////


int CDisplay::TextOutWrap( CRect *r, char* text, int size, bool color, bool count )
{
	int i;
	int lp = 0;
	int ls = 0;
	char line[150];
	int lines = 0;
	int tlen = strlen(text);
	int yp = r->mTop+_MARGIN_*2.5;
	int width = r->Width()-_MARGIN_*2;
	
	for( i = 0; i < tlen; i++ ){
		if( text[i] == ' ' ) ls = 0;
		line[lp] = text[i];
		lp++;
		line[lp] = 0;
		ls++;
		if( text[i] == '\n'){
			if( !count ) TextOutCenter( r->XCenter(), yp, line, size, color );
			line[0] = 0;
			lp = 0;
			yp += size+_MARGIN_*2;
			lines++;
		}else
		if( TextWidth(line,SansTypeface,size) > width ){
			line[lp-ls] = 0;
			i-=ls;
			if( !count ) TextOutCenter( r->XCenter(), yp, line, size, color );
			line[0] = 0;
			lp = 0;
			yp += size+_MARGIN_*2;
			lines++;
		}
	}
	if( lp ){
		if( !count ) TextOutCenter( r->XCenter(), yp, line, size, color );
		lines++;
	}
	return lines;
}

////////////////////////////////////////////////////////////////////////
//
//	TextOutWrapR()
//
//	Draws text, left justified in a rectangle, breaking the lines as
//	needed to fit.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

int CDisplay::TextOutWrapL( CRect *r, char* text, int size, bool color, bool count )
{
	int i;
	int lp = 0;
	int ls = 0;
	char line[150];
	int lines = 0;
	int tlen = strlen(text);
	int yp = r->mTop+_MARGIN_*2.5;
	int width = r->Width()-_MARGIN_*2;
	
	for( i = 0; i < tlen; i++ ){
		if( text[i] == ' ' ) ls = 0;
		line[lp] = text[i];
		lp++;
		line[lp] = 0;
		ls++;
		if( text[i] == '\n'){
			if( !count ) TextOutLeft( r->mLeft, yp, line, size, color );
			line[0] = 0;
			lp = 0;
			yp += size+_MARGIN_*2;
			lines++;
		}else
		if( TextWidth(line,SansTypeface,size) > width ){
			line[lp-ls] = 0;
			i-=ls;
			if( !count ) TextOutLeft( r->mLeft, yp, line, size, color );
			line[0] = 0;
			lp = 0;
			yp += size+_MARGIN_*2;
			lines++;
		}
	}
	if( lp ){
		if( !count ) TextOutLeft( r->mLeft, yp, line, size, color );
		lines++;
	}
	return lines;
}

////////////////////////////////////////////////////////////////////////
//
//	TextOutWrapR()
//
//	Draws text, right justified in a rectangle, breaking the lines as
//	needed to fit.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

int CDisplay::TextOutWrapR( CRect *r, char* text, int size, bool color, bool count )
{
	int i;
	int lp = 0;
	int ls = 0;
	char line[150];
	int lines = 0;
	int tlen = strlen(text);
	int yp = r->mTop+_MARGIN_*2.5;
	int width = r->Width()-_MARGIN_*2;
	
	for( i = 0; i < tlen; i++ ){
		if( text[i] == ' ' ) ls = 0;
		line[lp] = text[i];
		lp++;
		line[lp] = 0;
		ls++;
		if( text[i] == '\n'){
			if( !count ) TextOutRight( r->mRight, yp, line, size, color );
			line[0] = 0;
			lp = 0;
			yp += size+_MARGIN_*2;
			lines++;
		}else
		if( TextWidth(line,SansTypeface,size) > width ){
			line[lp-ls] = 0;
			i-=ls;
			if( !count ) TextOutRight( r->mRight, yp, line, size, color );
			line[0] = 0;
			lp = 0;
			yp += size+_MARGIN_*2;
			lines++;
		}
	}
	if( lp ){
		if( !count ) TextOutRight( r->mRight, yp, line, size, color );
		lines++;
	}
	return lines;
}

////////////////////////////////////////////////////////////////////////
//
//	GradientRoundRect()
//
//	Draws a button with rounded corners and a color gradient.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::GradientRoundRect( CRect *r, int r1, int g1, int b1, int r2, int g2, int b2 )
{
	GradientRoundRect(r->mLeft,r->mTop,r->mRight,r->mBottom,r1,g1,b1,r2,g2,b2);
};

////////////////////////////////////////////////////////////////////////
//
//	GradientRoundRect()
//
//	Draws a button with rounded corners and a color gradient.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::GradientRoundRect( int x1, int y1, int x2, int y2, int r1, int g1, int b1, int r2, int g2, int b2 )
{
	VGfloat outline[] = {0.0, 0.0, 0.0, 1.0};
	VGfloat color[10];
	color[0] = 0.0; color[1] = (VGfloat)r2/255.0f; color[2] = (VGfloat)g2/255.0f; color[3] = (VGfloat)b2/255.0f; color[4] = 1.0f;
	color[5] = 1.0; color[6] = (VGfloat)r1/255.0f; color[7] = (VGfloat)g1/255.0f; color[8] = (VGfloat)b1/255.0f; color[9] = 1.0f;
	
	FillLinearGradient(x1,mHeight-y2-_BUTTON_CORNER_RADIUS_,x1,mHeight-y1+_BUTTON_CORNER_RADIUS_, color, 2 );
	
	setstroke(outline);
	StrokeWidth(1.0);
	
	Roundrect(x1,mHeight-y2,x2-x1,y2-y1,_BUTTON_CORNER_RADIUS_,_BUTTON_CORNER_RADIUS_);
	
	RoundrectOutline(x1,mHeight-y2,x2-x1,y2-y1,_BUTTON_CORNER_RADIUS_,_BUTTON_CORNER_RADIUS_);
};	

////////////////////////////////////////////////////////////////////////
//
//	DrawImage()
//
//	Draws an image given a file and coordinates.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::DrawImage( int x, int y, int w, int h, char* file )
{
	Image( x, mHeight-y-h, w, h, file );
}

////////////////////////////////////////////////////////////////////////
//
//	Button()
//
//	Draws an button with text given coordinates.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::DrawButton( CRect *r, int x1, int y1, int x2, int y2, char* text )
{
	r->mLeft = x1;
	r->mTop = y1;
	r->mRight = x2;
	r->mBottom = y2;
	Button( r, _COLOR_3_,_COLOR_9_, text, 18 );
}

////////////////////////////////////////////////////////////////////////
//
//	Button()
//
//	Draws an button with text given coordinates.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::Button( CRect *r, int r1, int g1, int b1, int r2, int g2, int b2, char* text, int size )
{
	int top = (r->mTop+r->mBottom)/2 - size/2 - _MARGIN_;
	GradientRoundRect(r,r1,g1,b1,r2,g2,b2);
	TextOutCenter(r->XCenter(),top,text,size);
}

////////////////////////////////////////////////////////////////////////
//
//	FillRect()
//
//	Draws a filled rectangle given coordinates.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::FillRect(int x1, int y1, int x2, int y2)
{
	Rect(x1,mHeight-y2,x2-x1,y2-y1);
}

////////////////////////////////////////////////////////////////////////
//
//	DrawLine()
//
//	Draws a line given coordinates.
//	Moves the screen origin from the bottom to the top of the display.
//
////////////////////////////////////////////////////////////////////////

void CDisplay::DrawLine( int x1, int y1, int x2, int y2 )
{
	Line(x1,mHeight-y1,x2,mHeight-y2);
}
