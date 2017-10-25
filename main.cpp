#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include "crect.h"
#include "cdisplay.h"

#include "mcp3008spi.h"
#include <pigpio.h>

#include "main.h"
////////////////////////////////////////////////////////////////////////
//
//	Global Values
//
////////////////////////////////////////////////////////////////////////

bool 				gRefresh = true;
bool				gSendValues = true;
bool				gKBShutdown = false;
bool				gTakeStill = false;
bool				gShowStill = false;
bool				gRestart = false;
bool				gGuide = false;
bool				gTestFeatures = false;

int					gStillNum = 0;
int					gSendValuesIn = 0;

char				gCmdLine[1024];
int					gValues[13];

////////////////////////////////////////////////////////////////////////
//
//	DsiplayThread()
//
//	This function implements the body of the Display Thread.
//	It is responsible for holding, initalizing and calling the draw
//	function of the display class when needed.
//
////////////////////////////////////////////////////////////////////////

void* DsiplayThread( void* arg )
{
	int			mTimeout = CONTROLS_TIMEOUT;
	CDisplay	display;												// Global instance of the display class
	
	display.Init();														// Initalize the graphics library
	display.mBlank = false;												// Set Control blanking off
	
	while( true ){														// Loop forever...
		
		usleep(1000);													// 1ms Sleep...
		
		if( gRefresh ){													// If the interface needs to be refreshed, redraw it
			gRefresh = false;											// set the as not needing refresh
			display.DrawScreen();										// and rederaw it
			mTimeout = CONTROLS_TIMEOUT;
		}

		if( mTimeout > 0 ){
			if( display.mBlank ){
				display.mBlank = false;
				gRefresh = true;
			}else{
				mTimeout--;
				if( mTimeout == 0 ){
					display.mBlank = true;
					display.DrawScreen();										// rederaw it
				}
			}
		}
	}
	
	return NULL; 
}

#include <unistd.h>
#include <termios.h>

char MyGetChar() {

	int buf;
	struct termios old = {0};
	struct termios cfg = {0};
	
	if (tcgetattr(STDIN_FILENO, &old) < 0) perror("tcsetattr()");
	if (tcgetattr(STDIN_FILENO, &cfg) < 0) perror("tcsetattr()");
	
	cfg.c_lflag &= ~ICANON;
//	cfg.c_lflag &= ~ECHO;
//	cfg.c_cc[VMIN] = 1;
//	cfg.c_cc[VTIME] = 0;
	
	if (tcsetattr(STDIN_FILENO,TCSANOW, &cfg) != 0) perror("tcsetattr ICANON");
	
//	system("/bin/stty raw");
	buf = getchar();
//	system("/bin/stty cooked");
	
	if (tcsetattr(STDIN_FILENO,TCSADRAIN,&old) != 0) perror ("tcsetattr ~ICANON");
	return (buf);
}

void* KeyboardInputThread( void* arg )
{
	char c;
	while(true){
		c = MyGetChar();
		printf( "Keyboard: %d\n", c );
		if( c == 27 ){
			printf( "Got Keyboard Quit Cmd\n" );
			gKBShutdown = true;
			return NULL;
		}
		if( c == 49 && gTestFeatures ){
			printf( "Got Take Still Cmd\n" );
			gTakeStill = true;
		}
		if( c == 50 && gTestFeatures ){
			printf( "Toggle Guide\n" );
			gGuide = !gGuide;
		}
		if( c == 32  && gTestFeatures ){
			printf( "Clear Still Command\n" );
			gShowStill = !gShowStill;
			gRestart = true;
			gRefresh = true;
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////
//
//	SendValues()
//
//	Actually sends the values of the knobs to raspistill using a
//	TCP socket.
//
//	This function is oly called from the GPOI_Thread 
////////////////////////////////////////////////////////////////////////

void SendValues( void )
{
	int server;
    struct sockaddr_in server_addr;

    // use DNS to get IP address
    struct hostent *hostEntry;
    hostEntry = gethostbyname("localhost");
    if (!hostEntry) {
		printf("No such host name\n");
        return;
    }

    // setup socket address structure
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CY_PORT);
    memcpy(&server_addr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);

    // create socket
    server = socket(PF_INET,SOCK_STREAM,0);
    if (!server) {
        perror("socket");
   		gSendValuesIn = 500;											// Set a countdown to send the knob positions in 500ms
        return;
    }

    // connect to server
    if (connect(server,(const struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        perror("connect");
        return;
    }
		
	int nwritten = send(server, gValues, sizeof(gValues), 0);			// Send the array of values to raspistill
    
    close(server);
    
    return;
}
////////////////////////////////////////////////////////////////////////
//
//	GPOI_Thread()
//
//	This function implements the body of the GPIO Thread.
//	The function is responsible for reading the values of the knobs and
//	determining if any have changed by more than 
//
////////////////////////////////////////////////////////////////////////

void* KnobsThread( void* arg )
{
	int i;
	int val[KNOB_COUNT];												// Our local values for position of the knobs 
	int lval[KNOB_COUNT];												// Previos local values for the knob positions
	bool change;														// flag a change in the 
	unsigned char data[3];												// Values to pass to the Spi API
	
	for( i = 0; i < KNOB_COUNT; i++ ){
		val[i] = 0;
		lval[i] = 0;
		gValues[i] = 0;
	}
	mcp3008Spi a2d("/dev/spidev0.0", SPI_MODE_0, 1000000, KNOB_COUNT);
	
	while( true ){

		// Step through the 8 knobs
		for( i = 0; i < KNOB_COUNT; i++ ){
			data[0] = 1;  												//  first byte transmitted -> start bit
			data[1] = 0x80 | ((i&7)<< 4); 								// second byte transmitted -> (SGL/DIF = 1, D2=D1=D0=0)
			data[2] = 0; 												// third byte transmitted....don't care

			a2d.spiWriteRead( data, sizeof(data) );						// Call the Spi intercafe to read the value of the knob

			val[i]  = 0;
			val[i]  = (data[1] << 8) & 0b1100000000; 					//merge data[1] & data[2] to get result
			val[i] |= (data[2] & 0xff);
		}
		
		
		// Don't Compare values re just read to last values while the data is still sneding to raspstill
		if( gSendValues == false ){
			// Check the value of the data from the knobs to see if it has 
			// changed
			change = false;
			for( i = 0; i < KNOB_COUNT; i++ ){
				if( abs(val[i]-lval[i]) > KNOB_CHANGE_THRESHOLD ){
					gValues[i] = lval[i] = val[i];
					change = true;
				}
			}
			// If it has changed then copy the values to our global
			// and previos local values 
			if( change ){
//				for( i = 0; i < KNOB_COUNT; i++ ){
//					gValues[i] = lval[i] = val[i];
//				}
				gSendValues = true;											// Data need to be send via socket
				gRefresh = true;											// Screen Needs to be refreshed
			}
		}
				
		usleep(1000);													// Sleep for a milisecond
				
		// Global value to send the knob position after we first start
		if( gSendValuesIn > 0 ){
			gSendValuesIn--;
			if( gSendValuesIn == 0 ) gSendValues = true;
		}
	}
	return NULL; 
}


void* Still_Thread( void* s )
{
	int i;
	char cmd[128];
	while(1){
		if( access( "./still.tga", F_OK ) != -1 ){
			usleep(10000);
			if( gStillNum == 4 ){
				gStillNum = 0;
				for( i = 0; i < 4; i++ ){
					sprintf( cmd, "sudo rm ./out%02d.jpeg", i );
					system( cmd );
				}
			}
			printf( "Converting Still\n" );
			sprintf( cmd, "sudo rm ./out%02d.jpeg", gStillNum );
			system( cmd );
			sprintf( cmd, "convert ./still.tga -resize 540x540 ./out%02d.jpeg", gStillNum );
			system( cmd );
			system( "sudo rm ./still.tga" );
			gShowStill = true;
			gRefresh = true;
			gStillNum++;
		}
		usleep(500000);
	}
}

////////////////////////////////////////////////////////////////////////
//
//	GPOI_Thread()
//
//	This function implements the body of the GPIO Thread.
//	This thread reads the values of the 4 GPIO buttons into the global
//	values gValues[8] - gValues[11].
//	This function also calls the SendValues function to actually send
//	the gValues array to raspistill via TCP socket.
//
////////////////////////////////////////////////////////////////////////

void* GPOI_Thread( void* s )
{
	printf( "GPIO Init ...\n" );
	if( gpioInitialise() < 0 ){
		printf( "GPIO Init failed\n" );
		exit(0);
	}

	int i, val[5];
	
	gpioSetMode(17, PI_INPUT);
	gpioSetMode(27, PI_INPUT);
	gpioSetMode(18, PI_INPUT);
	gpioSetMode(04, PI_INPUT);
	
	while( 1 ){
		if( gpioRead(17) ) gValues[8] = 0; else gValues[8] = 1;
		if( gpioRead(18) ) gValues[9] = 0; else gValues[9] = 1;
		if( gpioRead(27) ) gValues[10] = 0; else gValues[10] = 1;
		if( gpioRead(04) ) gValues[11] = 0; else gValues[11] = 1;
		if( gKBShutdown ) gValues[11] = 1;
		if( gTakeStill ) gValues[8] = 1; gTakeStill = false;
		if( gRestart ) gValues[10] = 1; gRestart = false;
		if( gGuide ) gValues[12] = 1; else gValues[12] = 0;
		
		for( i = 0; i < 5; i++ ){
			if( val[i] != gValues[i+8] ){
				val[i] = gValues[i+8];				
				gSendValues = true;
				printf("Sending\n");
			}
		}

		FILE *fp;		
		if( fp = fopen( "./knobs.bin", "w" ) ){
			fwrite( gValues, sizeof(gValues),1,fp);
			fclose(fp);
		}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

//	double xc = (double)1.0-((gValues[0])/1024.0);						// The viewer controls are were designed as "move horizontal", "move vertical"
//	double yc = (double)1.0-((gValues[1])/1024.0);						// and "zoom around the center" of view because this seemed more intitive.
//	double zoom = (double)((gValues[3])/512.0);							// Raspistill wants a "reigon of interest" expressed as rectangular	
																		// coordinates.  This math translates the knob vlaues for move and zoom
//	double w = 1.0 - (zoom/2.0);										//  to the recatangular coordinates.
//	double h = 1.0 - (zoom/2.0);
//	double x = xc - (w/2.0);
//	double y = yc - (h/2.0);
	
//	if( x + w > 1.0 ) x = 1.0 - w;
//	if( y + h > 1.0 ) y = 1.0 - h;
	
//	printf( "Zoom: %f\n", zoom );
//	printf( "W x H: %fx%f\n",w,h );
//	printf( "X x Y: %fx%f\n",x,y );
	
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

		if( gSendValues ){
			SendValues();
			gSendValues = false;
		}
		
		if( gValues[11] ){
			printf( "GPIO Terminate\n" );
			gpioTerminate();
			return NULL;
		}
		
		usleep(10000);
	}
}

////////////////////////////////////////////////////////////////////////
//
//	ValToCmd()
//
//	This finction translates the global knob values into a command line
//	stored in the global value gCmdLine, suitable for passing to
//	raspistill and raspivid.
//
////////////////////////////////////////////////////////////////////////

void ValToCmd( void )
{
//	int br = ((gValues[4]*100)/1024);									// Translate the knob ranges from 0 to 1024 to the raspistill ranges
	int shutt = ((gValues[4]*100000)/1024);									// Translate the knob ranges from 0 to 1024 to the raspistill ranges

	int co = ((gValues[5]*200)/1024)-100;								// 0 - 100 for brighness and -100 to +100 for the others.
	int sa = ((gValues[6]*200)/1024)-100;			
	int sh = ((gValues[7]*200)/1024)-100;
			
	double xc = (double)1.0-((gValues[0])/1024.0);						// The viewer controls are were designed as "move horizontal", "move vertical"
	double yc = (double)1.0-((gValues[1])/1024.0);						// and "zoom around the center" of view because this seemed more intitive.
	double zoom = (double)((gValues[3])/512.0);							// Raspistill wants a "reigon of interest" expressed as rectangular	
																		// coordinates.  This math translates the knob vlaues for move and zoom
	double w = 1.0 - (zoom/2.0);										//  to the recatangular coordinates.
	double h = 1.0 - (zoom/2.0);
	double x = xc - (w/2.0);
	double y = yc - (h/2.0);
	
	if( gShowStill ){
		sprintf( gCmdLine, " -p 75,0,540,540 -ss %d -co %d -sa %d -sh %d -roi %.2f,%.2f,%.2f,%.2f", shutt,co,sa,sh,x,y,w,h );
	}else{
		sprintf( gCmdLine, " -p 10,10,1030,1030 -ss %d -co %d -sa %d -sh %d -roi %.2f,%.2f,%.2f,%.2f", shutt,co,sa,sh,x,y,w,h );
	}
}

////////////////////////////////////////////////////////////////////////
//
//	main()
//
//	Starts three threads then enters a loop to toggle between raspistill
//	and raspivid till the user presses the GPIO button to quit the
//	aplication.
//
////////////////////////////////////////////////////////////////////////

int main( int argc, char *argv[] )
{
	int i;
	int error;
	char cmdline[1024];					// Local copy of the command line
	char line[1024];					// buffer
	char startline[1024];				// starting command line
	gKBShutdown = false;
	gTakeStill = false;
	gShowStill = false;
	gTestFeatures = false;
	bool interface = true;
	
	printf( "Starting\n" );
	
	// Parse Command Line Options //////////////////////////////////////
	for( i = 0; i < argc; i++ ){
		printf( "Command Line Option: %d  %s\n", i, argv[i] );
		if( !strcmp( "-t", argv[i] ) ){
			printf( "Test Features On\n" );
			gTestFeatures = true;
		}
		if( !strcmp( "-o", argv[i] ) ){
			printf( "Interface Off\n" );
			interface = false;
		}
	}
	
	// Start Knobs Thread //////////////////////////////////////////////
	pthread_t KnobsThreadID;
	error = pthread_create(&KnobsThreadID, NULL, &KnobsThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Knobs Thread\n";
		return (0);
	}
	
	// Start Buttons Thread ////////////////////////////////////////////
	pthread_t GPOI_ThreadID;
	error = pthread_create(&GPOI_ThreadID, NULL, &GPOI_Thread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Buttons Thread\n";
		return (0); 
	}

	// Start Display Thread ////////////////////////////////////////////
	pthread_t DsiplayThreadID;
	if( interface ){
		error = pthread_create(&DsiplayThreadID, NULL, &DsiplayThread, (void*)NULL );
		if( error != 0 ){
			std::cout << "Can't Create Display Thread\n";
			return (0);
		}
	}
	
	// Start Keyboard Thread ///////////////////////////////////////////
	pthread_t KeyboardThredID;
	error = pthread_create(&KeyboardThredID, NULL, &KeyboardInputThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Keyboard Thread\n";
		return (0);
	}
	
	// Still Thread ////////////////////////////////////////////////////
	pthread_t StillThredID;
	error = pthread_create(&StillThredID, NULL, &Still_Thread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Still Image Thread\n";
		return (0);
	}

	// Read the starup line from a text file to expose the other options of raspistill
	FILE *fp;
	if( fp = fopen("./cmdline.txt","r") ){
		while(fgets(line,1024,fp)){
			if( line[0]!='#' ){
				if( line[0]=='s' && line[1]=='u' && line[2]=='d' && line[3]=='o' ){
					int l = strlen(line)-1;
					if( line[l]=='\n' ) line[l] = 0;
					strcpy(startline,line);
					break;
				}
			}
		}
		fclose(fp);
	}

	if( fp = fopen( "./knobs.bin", "r" ) ){
		int i;
		fread( gValues, sizeof(gValues),1,fp);
		fclose(fp);
		for( i = 0; i < 12; i++ ){
			printf("Knob: %d Value: %d\n",i+1,gValues[i]);
		}
	}else{
		printf("Unable To Open Knob File\n");
	}


	printf( "Start Line: [%s]\n", startline );

	usleep(50000);
	
	// This loop toggles between raspistill and raspivid
	// to switch the mode we send a command to raspistill to quit
	// this loop then starts up raspivid for 5 seconds of recording
	// then when raspivid exits it restarts rasipstill...
	while( true ){
		
		ValToCmd();														// Translate the knob positions to a command line string
		gSendValuesIn = 1000;											// Set a countdown to send the knob positions in 500ms
		sprintf( cmdline, startline, gCmdLine );
		system( cmdline );												// Start up raspistill...this thread will hang till that task quits
		
		// If the GPIO "Quit" button (11) was pressed then
		// break out of our loop...
		if( gValues[11] ) break;
		
//		ValToCmd();														// Translate the knob positions to a command line string
//		sprintf( cmdline, "sudo ./userland/userland/build/bin/raspivid -o test_video.mov -p 10,10,1030,1030 -w 300 -h 300 %s", gCmdLine );
//		system( cmdline );												// Start up raspivis...this thread will hang till that task quits
		
		usleep(10000);
	}

	return (0);
}
