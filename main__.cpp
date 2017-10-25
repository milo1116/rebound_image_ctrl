
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <unistd.h>

#include "config.h"
#include "crect.h"
#include "coledatetime.h"
#include "cdatasource.h"
#include "lists.h"
#include "cexdbrecord.h"
#include "datatypes.h"
#include "cdisplay.h"

#include "inpay-6.0/include/icharge.h"

#include "BluKeyAPI.h"
#include "BlueKeyLinux.h"

//#include <pigpio.h>

#define SCREEN_TIMEOUT  15000

void SetScreen( int state, int timeout = SCREEN_TIMEOUT );
void UserMessage( char* msg, bool buttons = true );
void ErrorMessage( char* msg );
void* SaveLocalAccountThread( void* arg );
void SaveLocalAccount( void );
void* LoadLocalAccountThread( void* arg );
void LoadLocalAccount( char* barcode );
void CreditLocalAccounts( void );

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
DisplayText			gDT;

CDisplay			gDisplay;

Config 				gConfig;
CPointerList 		gSalesItemList;
CPointerList		gCart;
CKioskTextFromDB	gDBText;
CPointerList		gTaxTypes;
CStringList			gTouchDevices;
CReceipt			gReceipt;
CCrsAccount			gLocalAccount;
CPointerList 		gRequestList;
CPointerList		gWiFIList;

CPointerList 		gStockCounts;
CSalesItem*			gSalesItemPtr;

CPointerList		gThreadList;

int					gAdCount;
int					gCurrentAd;
char				gAds[MAX_ADS][MAX_AD_NAME];

int					gLanguageCount;
int					gCurrentLanguage;
char				gLanguage[MAX_LANG][MAX_LANG_NAME];

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void MakeThumbsThread( void )
{
	int i;
	char* pos;
	char line[100], file[200], cmd[200];
	FILE* fp;
	
	if( fp = popen("ls ./images/ads/*.jpg -1","r") ){
		while( fgets(line,sizeof(line),fp) ){
			if ((pos=strchr(line, '\n')) != NULL) *pos = '\0';
			sprintf( file, "./images/ads/local/%s", &line[13] );
			if( access( file, F_OK ) == -1 ){
				sprintf( cmd, "convert ./images/ads/%s -resize 200x200 ./images/ads/local/%s", &line[13], &line[13] );
				system( cmd );
				printf( cmd );
				printf( "\n" );
			}
		}
		pclose(fp);
	}	
}

void GetSysDate( char* string, int len )
{
	FILE* fp;
	char* pos;
	string[0] = 0;
	
	if( fp = popen("date","r") ){
		fgets(string,len,fp);
		if ((pos=strchr(string, '\n')) != NULL) *pos = '\0';
		pclose(fp);
	}	
}

void Log( const char* string )
{
	FILE* fp;
	char datetime[200];
	GetSysDate(datetime,sizeof(datetime));
	if( fp = fopen("./logfile.txt","a") ){
		fprintf( fp, "%s -- %s\n", datetime, string );
		fclose(fp);
	}
}

bool VideoPlaying( void )
{
	FILE* fp;
	char line[200];
	
	if( fp = popen("ps -ef | grep .mov","r") ){
		while( fgets(line,sizeof(line),fp) ){
			if( (strstr( line, "omxplayer" )) > 0 ){
				pclose(fp);
				return true;
			}
		}
		pclose(fp);
	}
	return false;
}

void ReadLanguages( void )
{
	int i;
	char line[40];
	FILE* fp;
	
	for( i = 0; i < MAX_LANG; i++ ){
		gLanguage[i][0] = 0;
	}
	
	gLanguageCount = 0;
	if( fp = popen("ls ./languages/ -1","r") ){
		while( fgets(line,sizeof(line),fp) && gLanguageCount < MAX_LANG ){
				if( (strstr( line, ".jpg" )) > 0 ){
					sscanf( line,"%s", gLanguage[gLanguageCount] );
					printf( "Found Language Named: %s\n", gLanguage[gLanguageCount] );
					gLanguageCount++;
				}
		}
		pclose(fp);
	}
}

void ResetTerminal( void )
{
	system( "stty 6d02:5:4bf:8a3b:3:1c:7f:15:4:0:1:ff:11:13:1a:ff:12:f:17:16:ff:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0");
}

void Display( void ){
	printf( "\n##########################################\n" );
	printf( "   Software Version: %s\n", gConfig.mVersion );
	printf( "          KioskName: %s\n", gConfig.mKioskName );
	printf( "           Operator: %s\n", gConfig.mOperator );
	printf( " SwipeImageReversed: %d\n", gConfig.mSwipeReversed );
	printf( "AllowNonStdBarcodes: %d\n", gConfig.mAllowNonStdBarcodes );
	printf( "      MinCCPurchase: $%d.%02d\n", gConfig.mMinCCPurchase/100, gConfig.mMinCCPurchase%100 );
	printf( "            TaxArea: %s\n", gConfig.mTaxArea );
	printf( "\n" );
	printf( "             BDHost: %s\n", gConfig.mDBHost );
	printf( "             DBName: %s\n", gConfig.mDBName );
	printf( "             DBUser: %s\n", gConfig.mDBUser );
	printf( "             DBPass: %s\n", gConfig.mDBPass );
	printf( "\n" );
	printf( "         GatewayURL: %s\n", gConfig.mGatewayURL );
	printf( "          GatewayID: %d\n", gConfig.mGatewayID );
	printf( "           XTranKey: %s\n", gConfig.mXTranKey );
	printf( "      MerchantLogin: %s\n", gConfig.mMerchantLogin );
	printf( "\n" );
	printf( "     Database Fails: %d\n", gConfig.mDatabaseFails );
	printf( "         Ping Fails: %d\n", gConfig.mPingFailCount );
	printf( "    Net Loss Reboot: %d\n", gConfig.mNetLossReboot );
	printf( "\n" );
	
	printf( "    WIFI (0) Has IP: %d\n", gConfig.mWIFIHasIP0 );
	printf( "    WIFI IP Address: %s\n", gConfig.mWIFIIPAddress0 );
	printf( "          WIFI SSID: %s\n", gConfig.mWIFISSID0 );
	printf( "\n" );
	printf( "    WIFI (1) Has IP: %d\n", gConfig.mWIFIHasIP1 );
	printf( "    WIFI IP Address: %s\n", gConfig.mWIFIIPAddress1 );
	printf( "          WIFI SSID: %s\n", gConfig.mWIFISSID1 );
	printf( "\n" );
	
	printf( "  Ethernet 0 Has IP: %d\n", gConfig.mEtheHasIP0 );
	printf( " Ethernet IPAddress: %s\n", gConfig.mEtheIPAddress0 );
	printf( "\n" );
	printf( "  Ethernet 1 Has IP: %d\n", gConfig.mEtheHasIP1 );
	printf( " Ethernet IPAddress: %s\n", gConfig.mEtheIPAddress1 );
	printf( "\n" );
	
	printf( "   Trace Route 3306: %d\n", gConfig.mTraRte3306 );
	printf( "\n" );
	
	printf( "             Screen: %s\n", gDisplay.ScreenName() );
	printf( "     My MAC Address: %s\n", gConfig.mMacAddress );
	printf( "\n" );
	printf( "  Pay Range Init OK: %s\n", gConfig.mPayRangeInitalized?"Yes":"No" );
	printf( "\n" );
	
	if( gDBText.mServiceBarcode )
	printf( "    Service Barcode: %s\n", gDBText.mServiceBarcode );
	if( gDBText.mOperatorEmail )
	printf( "     Operator EMail: %s\n", gDBText.mOperatorEmail );
	
	printf( "##########################################\n" );
}

void AppendConfigToEmail( char* file )
{
	FILE* fp;
	if( fp = fopen( file, "a" ) ){
		fprintf( fp, "\n#### Kioksk Config ####\n" );
		fprintf( fp, "  Software Version: %s\n", gConfig.mVersion );
		fprintf( fp, "         KioskName: %s\n", gConfig.mKioskName );
		fprintf( fp, "          Operator: %s\n", gConfig.mOperator );
		fprintf( fp, "SwipeImageReversed: %d\n", gConfig.mSwipeReversed );
		fprintf( fp, "     MinCCPurchase: $%d.%02d\n", gConfig.mMinCCPurchase/100, gConfig.mMinCCPurchase%100 );
		fprintf( fp, "           TaxArea: %s\n", gConfig.mTaxArea );
		fprintf( fp, "\n" );
		fprintf( fp, "            BDHost: %s\n", gConfig.mDBHost );
		fprintf( fp, "            DBName: %s\n", gConfig.mDBName );
		fprintf( fp, "            DBUser: %s\n", gConfig.mDBUser );
		fprintf( fp, "            DBPass: ########\n" );
		fprintf( fp, "\n" );
		fprintf( fp, "        GatewayURL: %s\n", gConfig.mGatewayURL );
		fprintf( fp, "         GatewayID: %d\n", gConfig.mGatewayID );
		fprintf( fp, "          XTranKey: ########\n" );
		fprintf( fp, "     MerchantLogin: ########\n" );
		fprintf( fp, "\n" );
		
		fprintf( fp, "   WIFI (0) Has IP: %d\n", gConfig.mWIFIHasIP0 );
		fprintf( fp, "   WIFI IP Address: %s\n", gConfig.mWIFIIPAddress0 );
		fprintf( fp, "         WIFI SSID: %s\n", gConfig.mWIFISSID0 );
		fprintf( fp, "\n" );
		
		fprintf( fp, "   WIFI (1) Has IP: %d\n", gConfig.mWIFIHasIP1 );
		fprintf( fp, "   WIFI IP Address: %s\n", gConfig.mWIFIIPAddress1 );
		fprintf( fp, "         WIFI SSID: %s\n", gConfig.mWIFISSID1 );
		fprintf( fp, "\n" );
	
		fprintf( fp, " Ethernet 0 Has IP: %d\n", gConfig.mEtheHasIP0 );
		fprintf( fp, "Ethernet IPAddress: %s\n", gConfig.mEtheIPAddress0 );
		fprintf( fp, "\n" );
		fprintf( fp, " Ethernet 1 Has IP: %d\n", gConfig.mEtheHasIP1 );
		fprintf( fp, "Ethernet IPAddress: %s\n", gConfig.mEtheIPAddress1 );
		fprintf( fp, "\n" );
	
		fprintf( fp, "    Database Fails: %d\n", gConfig.mDatabaseFails );
		fprintf( fp, "        Ping Fails: %d\n", gConfig.mPingFailCount );
		fprintf( fp, "   Net Loss Reboot: %d\n", gConfig.mNetLossReboot );
		fprintf( fp, "\n" );
		fprintf( fp, "  Trace Route 3306: %d\n", gConfig.mTraRte3306 );
		fprintf( fp, "\n" );
		fprintf( fp, "            Screen: %d\n", gConfig.mDisplayScreen );
		fprintf( fp, "    My MAC Address: %s\n", gConfig.mMacAddress );
		fprintf( fp, "\n" );
		fprintf( fp, " Pay Range Init OK: %s\n", gConfig.mPayRangeInitalized?"Yes":"No" );
		fprintf( fp, "\n" );
		fprintf( fp, "#########################\n" );
		fprintf( fp, "\n" );
		if( gDBText.mServiceBarcode )
		fprintf( fp, "   Service Barcode: %s\n", gDBText.mServiceBarcode );
		if( gDBText.mOperatorEmail )
		fprintf( fp, "    Operator EMail: %s\n", gDBText.mOperatorEmail );
		fclose(fp);
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/*
void* AddThread(char* desc )
{
	CThreadInfo* ti = new CThreadInfo;
	strcpy(ti->mDescription,desc);
	gThreadList.Add(ti);
	printf( "###################################\n" );
	printf( "### %d %s \n", ti, desc );
	printf( "###################################\n" );
	return ((void*)ti);
}

void RemoveThread( void * ptr )
{
	int i;
	CThreadInfo* ti = (CThreadInfo*)ptr;
	for( i = 0; i < gThreadList.Count(); i++ ){
		if( ti == (CThreadInfo*)gThreadList.GetAt(i) ){
			gThreadList.RemoveAt(i);
			delete ti;
			return;
		}
	}
	printf( "###################################\n" );
	printf( "###  %d  Cant Find Thread Pointer  \n", ptr );
	printf( "###################################\n" );
}

void ListThreads( void )
{
	int i;
	for( i = 0; i < gThreadList.Count(); i++ ){
		CThreadInfo* ti = (CThreadInfo*)gThreadList.GetAt(i);
		printf( "Thread: %d  %s\n", ti, ti->mDescription );
	}	
}

void AppendThreadListToEmail( char* file )
{
	int i;
	FILE* fp;	
	if( fp = fopen( file, "a" ) ){
		fprintf( fp, "\n#### Kioksk Thread List ####\n" );	
		for( i = 0; i < gThreadList.Count(); i++ ){
			CThreadInfo* ti = (CThreadInfo*)gThreadList.GetAt(i);
			fprintf( fp, "Thread: %s\n", ti->mDescription );
		}	
		fclose(fp);
	}
}
*/
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void WorkingOn( char* msg = NULL )
{
	if( msg ){
		gConfig.mWorkingMsg[49] = 0;
		strncpy(gConfig.mWorkingMsg,msg,49);
	}else gConfig.mWorkingMsg[0] = 0;
	gConfig.mWorking = true;
}

void WorkingOff( void )
{
	gConfig.mWorking = false;
	gConfig.mRefresh = true;
}

class MyICharge: public ICharge
{
public:
	virtual int FireSSLStatus(IChargeSSLStatusEventParams *e)
	{
		printf("SSLStatus: %s\n", e->Message);
		return 0;
	}

	virtual int FireSSLServerAuthentication(IChargeSSLServerAuthenticationEventParams *e)
	{
		if (e->Accept) return 0;
		printf("Server provided the following certificate:\nIssuer: %s\nSubject: %s\n",
		       e->CertIssuer, e->CertSubject);
		printf("The following problems have been determined for this certificate: %s\n", e->Status);
		printf("Would you like to continue anyway? [y/n] ");
		char buf[5];
		fgets(buf, 5, stdin);
		if (buf[0] == 'y') e->Accept = true;
		return 0;
	}
};



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
#include <pthread.h>
#include <unistd.h>
#include <termios.h>

class KeyboardBuffer
{
public:
	int		mHead;
	int		mTail;
	char	mBuff[500];
	int		mQuit;
	
	KeyboardBuffer(void)
	{
		mHead = 0;
		mTail = 0;
		mQuit = false;
		memset(mBuff,0,500);
	};

	void PutChr(char c)
	{
		mBuff[mHead%500] = c;
		mHead++;
	};
	
	char GetChr(void)
	{
		char c = 0;
		if( mHead > mTail ){
			c = mBuff[mTail%500];
			mTail++;
		}
		return c;
	};
	
	bool CharAvail(void)
	{
		return (mHead>mTail);
	};
};

char MyGetChar() {

	char buf = 0;
	struct termios old = {0};
	
	if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
	
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	
	if (tcsetattr(0, TCSANOW, &old) != 0) perror("tcsetattr ICANON");
	
	buf = getchar();
//	if (read(0, &buf, 1) < 0) perror ("read()");
	
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	
	if (tcsetattr(0, TCSADRAIN, &old) != 0) perror ("tcsetattr ~ICANON");
	return (buf);
}

void* KeyboardInputThread( void* arg )
{
	char c;
//	void* t = AddThread((char*)"Keyboard Thread");
	if( arg ){
		KeyboardBuffer *kb = (KeyboardBuffer*)arg;
		while(!kb->mQuit){
			c = MyGetChar();
			kb->PutChr(c);
			
			// Mask All Non-lowecase alpha input echoed to console
			// so that credit card track data doesnt show up...
			if( (c >= 'a' && c <= 'z') || c == 10 ){
				printf("%c",c);
			}else{
				printf("X");
			}
			
		}
	}
//	RemoveThread(t);
	return NULL;
}
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
#include <fcntl.h>
#include <linux/input.h>

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)

class TouchScreenBuffer
{
public:
	int		mHead;
	int		mTail;
	int		mBuffX[50];
	int		mBuffY[50];
	int		mQuit;
	int		mMaxX;
	int		mMaxY;
	
	TouchScreenBuffer(void)
	{
		mHead = 0;
		mTail = 0;
		mQuit = false;
		memset(mBuffX,0,sizeof(int)*50);
		memset(mBuffY,0,sizeof(int)*50);
	};

	void PutPoint(int x, int y)
	{
		mBuffX[mHead%50] = x;
		mBuffY[mHead%50] = y;
		mHead++;
		
		if( gConfig.mPulse ) printf( "Touches Waiting: %d\n", mHead - mTail );
	};
	
	bool GetPoint(int *x, int *y)
	{
		(*x) = 0;
		(*y) = 0;
		if( mHead > mTail ){
			(*x) = mBuffX[mTail%50];
			(*y) = mBuffY[mTail%50];
			mTail++;
			return true;
		}
		return false;
	};
	
	bool TouchAvail(void)
	{
		return (mHead>mTail);
	};
	
	int TouchQueCount( void )
	{
		return mHead - mTail;
	};
};

class TouchScreen
{
public:
	int		mX;
	int		mY;
	int		mMinX;
	int		mMinY;
	int		mMaxX;
	int		mMaxY;
	int		mDown;
	
	TouchScreen(){
		mX = mY = 0;
		mMinX = mMaxX = 0;
		mMinY = mMaxY = 0;
		mDown = 0;
	};
	
	bool 	GetDeviceInfo( int num, bool write_to_file = false );
	bool 	GetTouch( int num );
	int 	FindDevice( void );
	int		GuessDevice( void );
};

void AppendDeviceListToEmail( char* file )
{
	FILE* fp;
	int fd, i, j;
	char dev[256];
	char name[256];
	
	if( fp = fopen( file, "a" ) ){
		fprintf( fp, "\n#### Kioksk Devices ####\n" );
	
		for( i = 0; i < 10; i++ ){
			sprintf( dev,"/dev/input/event%d",i);
			fd = open (dev, O_RDONLY | O_NONBLOCK);
			if(fd < 0){
//				fprintf (fp,"Could not open device %d\n",i);
			}else{
				ioctl (fd, EVIOCGNAME(sizeof(name)), name);
				fprintf(fp,"Input device %d name: \"%s\"\n", i, name);
			}
		}
		fclose(fp);
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void ListDevices( void )
{
	int fd, i, j;
	char dev[256];
	char name[256];
	
	for( i = 0; i < 10; i++ ){
		sprintf( dev,"/dev/input/event%d",i);
		fd = open (dev, O_RDONLY | O_NONBLOCK);
		if(fd < 0){
//			printf ("Could not open device %d\n",i);
		}else{
			ioctl (fd, EVIOCGNAME(sizeof(name)), name);
			printf("Device %d name: \"%s\"\n", i, name);
		}
	}
	return;
}

int TouchScreen::FindDevice( void )
{
	int fd, i, j;
	char dev[256];
	char name[256];
	
	for( i = 0; i < 10; i++ ){
		sprintf( dev,"/dev/input/event%d",i);
		fd = open (dev, O_RDONLY | O_NONBLOCK);
		if(fd < 0){
//			printf ("Could not open device %d\n",i);
		}else{
			ioctl (fd, EVIOCGNAME(sizeof(name)), name);
			printf("Input device %d name: \"%s\"\n", i, name);
			for( j = 0; j < gTouchDevices.Count(); j++ ){
				char* n = gTouchDevices.GetAt(j);
				if( !strcasecmp(n,name) ){
					printf("Found Supported Touch Device %s On Input %d\n",  n, i );
					strcpy(gConfig.mTouchDeviceName,name);
					return i;
				}
			}
		}
	}
	return -1;
}

bool TouchScreen::GetDeviceInfo( int num, bool write_to_file )
{
	int fd;
	struct input_absinfo abs;
	char dev[256];
	char name[256] = "Unknown";
	unsigned long bits[NBITS(KEY_MAX)];

	sprintf( dev, "/dev/input/event%d", num );

	fd = open (dev, O_RDONLY | O_NONBLOCK);
	if(fd < 0){
		printf ("Could not open touchscreen device #%d\n", num);
		return false;
	}

	ioctl (fd, EVIOCGNAME(sizeof(name)), name);
	printf("Input device name: \"%s\"\n", name);

	if( write_to_file ){
		FILE* fp;
		if( fp = fopen( "./config/screens.txt", "a" ) ){
			fprintf( fp, "%s\n", name );
			fclose(fp);
		}
	}

	ioctl (fd, EVIOCGBIT(0, EV_MAX), bits);
	if (!test_bit (EV_ABS, bits)){
		printf( "Does not provide ABS events\n");
		close(fd);
		return false;
	}

	ioctl (fd, EVIOCGBIT (EV_ABS, KEY_MAX), bits);
	if (!(test_bit (ABS_MT_POSITION_X, bits) &&
	test_bit (ABS_MT_POSITION_Y, bits)))
	{
		ioctl (fd, EVIOCGABS (ABS_X), &abs);
		mMinX = (int)abs.minimum;
		mMaxX = (int)abs.maximum;
		ioctl (fd, EVIOCGABS (ABS_Y), &abs);
		mMinY = (int)abs.minimum;
		mMaxY = (int)abs.maximum;
		printf("No Multi Touch\n");
	}
	else
	{
		ioctl (fd, EVIOCGABS (ABS_MT_POSITION_X), &abs);
		mMinX = (int)abs.minimum;
		mMaxX = (int)abs.maximum;
		ioctl (fd, EVIOCGABS (ABS_MT_POSITION_Y), &abs);
		mMinY = (int)abs.minimum;
		mMaxY = (int)abs.maximum;
		printf("Multi Touch\n");
	}

	close(fd);
	printf ( "Touch Screen Range X: %d - %d, Y: %d - %d\n", 
				mMinX, mMaxX, mMinY, mMaxY );

	return true;
}

bool TouchScreen::GetTouch( int num )
{
	int i, fd, rb;
	bool ret = false;
	char dev[256];
	input_event ev[64];
	
	sprintf( dev, "/dev/input/event%d", num );
	
	try{
		if( fd = open(dev, O_RDONLY) ){
			
			rb=read(fd,(void*)&ev,sizeof(struct input_event)*64);
			
			if( rb > 0 ){
				for( i = 0; i < rb/sizeof(struct input_event); i++ ){
					if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 1){
						if( gConfig.mPulse ) printf("Touch Starting\n");
						ret = true;
					}
					else if (ev[i].type == EV_KEY && ev[i].code == 330 && ev[i].value == 0){ 
						if( gConfig.mPulse ) printf("Touch Finished\n");
						mDown = 0;
					}
					else if (ev[i].type == EV_ABS && ev[i].code == 0 && ev[i].value > 0){
						mX = ev[i].value;
						if( gConfig.mPulse ) printf("Raw X = %d\n", ev[i].value);
					}
					else if (ev[i].type == EV_ABS && ev[i].code == 1 && ev[i].value > 0){
						mY = ev[i].value;
						if( gConfig.mPulse ) printf("Raw Y = %d\n", ev[i].value);
					}
				}
			}else{
				strcpy(gConfig.mMessageText,"Touch Screen USB Unplugged\nPlease Reconnect.");
				gConfig.mDisplayScreen = SCREEN_USERMSG;
				gConfig.mScreenTimeout = 1000;
				gConfig.mRefresh = true;
				usleep(100000);
			}
			
			close(fd);
		}else{
			if( gConfig.mPulse ) printf("Unable To Open Touch Screen: %s\n", dev );
		}
	}catch(std::exception& e)
	{
		std::cerr << "Touch Screen Exception : " << e.what() << std::endl;
	}
	
	return ret;
}

// Thread to watch a ts device and end on any touch ////////////////////

void* GetTouchThread( void* arg )
{
	int *n = (int*)arg;
	printf( "Begin Touch Thread %d\n", *n );
	TouchScreen *ts = new TouchScreen;
	if( ts->GetDeviceInfo(*n) ){
		if( ts->GetTouch(*n) ){
			printf( "Found Touch Thread %d\n", *n );
			gConfig.mTouchDevice = *n;
		}
	}
	printf( "End Touch Thread %d\n", *n );
	delete ts;
	return NULL;
}

void* TouchScreeInputThread( void* arg )
{
	bool savename = false;
	int x, y, i, error;
	int num = -1;
	TouchScreen *ts = new TouchScreen;
	
	// Try To Find The Device From Saved Touch Screen Names ////////////
	num = ts->FindDevice();
	
	// Section To Find Touch Screen ////////////////////////////////////
	if( num == -1 ){
		printf( "######## Unable To Find Supported Touch Screen ########\n" );
		int tn[11];
		pthread_t tt[11];
		gConfig.mTouchDevice = -1;
		savename = true;		
		for( i = 0; i < 10; i++ ){
			tn[i] = i;
			printf( "Creating Touch Thread %d\n", i );
			error = pthread_create(&(tt[i]), NULL, &GetTouchThread, (void*)&(tn[i]) );
			if( error != 0 ){
				std::cout << "Can't Create Touch Test Thread\n";
			}
		}		
		while( gConfig.mTouchDevice == -1 ){
			usleep(1000);
		}
		printf( "######## mTouchDevice Is %d ##########\n", gConfig.mTouchDevice );
		for( i = 0; i < 10; i++ ){
			if( i != gConfig.mTouchDevice ) pthread_cancel(tt[i]);
		}
		num = gConfig.mTouchDevice;
		printf( "######## Touch Screen %d #########\n", num );
	}else{
		gConfig.mTouchDevice = num;
	}
	////////////////////////////////////////////////////////////////////
	gConfig.mRefresh = true;
	
	if( arg ){
		if( ts->GetDeviceInfo(num,savename) ){
			TouchScreenBuffer *tb = (TouchScreenBuffer*)arg;
			while(!tb->mQuit){
				if( ts->GetTouch(num) ){
					tb->PutPoint(ts->mX,ts->mY);
					tb->mMaxX = ts->mMaxX;
					tb->mMaxY = ts->mMaxY;
				}
				usleep(1000);
			}
		}
	}
	
	delete ts;
//	RemoveThread(t);
	return NULL;
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* KioskInfoThread( void* arg )
{
	int i,j,k;
	char file[150], temp[150];
	CDataSource db;
	
	bool time_set = false;
	bool text_loaded = false;
	bool taxes_loaded = false;
	bool ccinfo_loaded = false;

	CPointerList localprices;
	
//	void* t = AddThread((char*)"KioskInfo Thread");
	
	while( 	!time_set 		||
			!text_loaded 	||
			!taxes_loaded 	||
			!ccinfo_loaded 	){
	
		if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
			
			if( gSalesItemList.Count() == 0 ){
				WorkingOn((char*)"Loading Sales Items");
				
				CSalesItem si;
				si.GetBatch(&db,(char*)"select * from `salesitems` where `active` = 1 order by `description` asc", &gSalesItemList );
				printf("Loaded %d Sales Items\n",gSalesItemList.Count());

				for( i = 0; i < gSalesItemList.Count(); i++ ){
					CSalesItem* pSI = (CSalesItem*)gSalesItemList.GetAt(i);
					if( !strlen(pSI->mBarcode) ){
						
						FILE* fp;
						// Remove Zero (small) files....
						sprintf( file, "./images/visualmenu/%06d.jpg", pSI->mDBID );
						if( fp = fopen(file,"r") ){
							fseek(fp,0L,SEEK_END);
							k = ftell(fp);
							if( k < 500 ){
								printf( "Removing JPG, too small: %s\n", file );
								sprintf( temp, "rm %s", file );
								system( temp );
							}
							fclose(fp);
						}
							
						if( access( file, F_OK ) == -1 ) {
							printf( "Missing: %s\n", temp );
							sprintf( temp, "wget %s/%06d.jpg -O ./images/visualmenu/%06d.jpg", gConfig.mVisItemsURL, pSI->mDBID, pSI->mDBID );
							printf( "%s\n", temp );
							system( temp );
						}
						
						// Remove Zero (small) files....
						if( fp = fopen(file,"r") ){
							fseek(fp,0L,SEEK_END);
							k = ftell(fp);
							if( k < 500 ){
								printf( "Removing JPG, too small: %s\n", file );
								sprintf( temp, "rm %s", file );
								system( temp );
							}
							fclose(fp);
						}
					}
				}			

				char lpn[50];
				CLocalPrice lp;
				
				WorkingOn((char*)"Loading Local Prices");
				
				sprintf( temp, "select `localprices` from `kiosknames` where `kioskname` like '%s'", gConfig.mKioskName );
				db.GetSingleString(temp,lpn,50);
				printf("Local Price Name: %s\n", lpn );
				if( strlen(lpn) == 0 ) strcpy(lpn,gConfig.mKioskName);
				sprintf( temp, "select * from `localprices` where `kioskname` like '%s'", lpn );
				lp.GetBatch(&db,temp,&localprices);
				printf("Loaded %d Local Prices\n",localprices.Count());
				
				for( i = 0; i < gSalesItemList.Count(); i++ ){
					CSalesItem* pSI = (CSalesItem*)gSalesItemList.GetAt(i);
					for( j = 0; j < localprices.Count(); j++ ){
						CLocalPrice* pLP = (CLocalPrice*)localprices.GetAt(j);
						if( pLP->mSalesItemId == pSI->mDBID ){
							pSI->mLocalPriceId = pLP->mDBID;
							pSI->mInvoicePrice = pLP->mInvoicePrice;
							pSI->mUnitPrice = pLP->mKioskPrice;
							printf("Local Price %s $%d.%02d\n", pSI->mDescription, pLP->mKioskPrice/100, pLP->mKioskPrice%100 );
						}
					}
				}

				for( i = 0; i < localprices.Count(); i++ ){
					CLocalPrice* pLP = (CLocalPrice*)localprices.GetAt(i);
					delete pLP;
				}
				localprices.Clear();				
				
				gConfig.mRefresh = true;
				WorkingOff();
			}
			if( !text_loaded ){
				WorkingOn((char*)"Loading Text");
				gDBText.Init(&db);
				text_loaded = true;
				gConfig.mRefresh = true;
				WorkingOff();
			}
			if( !taxes_loaded ){
				WorkingOn((char*)"Load Sales Tax Table");
				CTaxTable tt;
				sprintf( temp, "select * from `taxtable` where `taxarea` like '%s'", gConfig.mTaxArea );
				tt.GetBatch(&db,temp, &gTaxTypes );
				printf("Loaded %d Tax Types\n",gTaxTypes.Count());
				taxes_loaded = true;
				gConfig.mRefresh = true;
				WorkingOff();
			}

			if( gConfig.mKioskNameDBID == 0 ){
				WorkingOn((char*)"Finding Kiosk Name in DB");
				sprintf(temp,"select `id` from `kiosknames` where `kioskname` like '%s'",gConfig.mKioskName);
				db.GetSingleInt(temp,&gConfig.mKioskNameDBID);
				
				if( gConfig.mKioskNameDBID == 0 ){
					sprintf(temp,"insert into `kiosknames` (`kioskname`) values ( '%s' )",gConfig.mKioskName);
					db.MySQLQuery(temp);
					gConfig.mKioskNameDBID = db.GetLastID((char*)"kiosknames");
				}
				printf( "DBID = %d\n", gConfig.mKioskNameDBID );
				
				sprintf( temp, "update `kiosknames` set `active` = '1', `version` = '%s' where `id`='%d'", gConfig.mVersion, gConfig.mKioskNameDBID );
				db.MySQLQuery(temp);
				
				WorkingOff();
			}

			if( gConfig.mKioskNameDBID && strlen(gConfig.mXTranKey) == 0 ){
				if( !ccinfo_loaded ){
					
					WorkingOn((char*)"Loading CC Processing Info");
					sprintf(temp,"select `gatewayid` from `kiosknames` where `kioskname` like '%s'",gConfig.mKioskName);
					ccinfo_loaded = db.GetSingleInt(temp,&(gConfig.mGatewayID));

					if( ccinfo_loaded ){
						sprintf(temp,"select `gatewayurl` from `kiosknames` where `kioskname` like '%s'",gConfig.mKioskName);
						ccinfo_loaded = db.GetSingleString(temp,gConfig.mGatewayURL,100);
					}
					if( ccinfo_loaded ){
						sprintf(temp,"select `merchantlogin` from `kiosknames` where `kioskname` like '%s'",gConfig.mKioskName);
						ccinfo_loaded = db.GetSingleString(temp,gConfig.mMerchantLogin,100);
					}
					if( ccinfo_loaded ){
						sprintf(temp,"select `xtrankey` from `kiosknames` where `kioskname` like '%s'",gConfig.mKioskName);
						ccinfo_loaded = db.GetSingleString(temp,gConfig.mXTranKey,100);
					}
					WorkingOff();
				}			
			}
			
			// Eventually set the system time here....
			if( !time_set ){
				COleDateTime t;
				printf( "%s\n", t.Format() );
				db.GetSingleTime((char*)"select NOW()", &t );
				printf( "%s\n", t.Format() );
				
				sprintf(temp,"select `servicedate` from `servicedate` where `kioskname` like '%s' order by servicedate desc limit 1", gConfig.mKioskName );
				db.GetSingleTime(temp, &(gConfig.mLastService) );
				
				time_set = true;
			}
			gConfig.mDatabaseFails = 0;
			db.DisConnect();	
		} else {
			sleep(10);
			gConfig.mDatabaseFails++;
		}
		usleep(100000);
	}
//	RemoveThread(t);
	return NULL;			
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* SetServiceDateThread( void* arg )
{
	char temp[150];
	CDataSource db;
//	void* t = AddThread((char*)"Set Service Thread");

	while( true ){
	
		if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){

			CServiceDate sd;
			strcpy( sd.mKioskName, gConfig.mKioskName );
			sd.mServiceDate.mValid = true;
			sd.PutData(&db);

			sprintf(temp,"select `servicedate` from `servicedate` where `kioskname` like '%s' order by servicedate desc limit 1", gConfig.mKioskName );
			db.GetSingleTime(temp, &(gConfig.mLastService) );
			
			gConfig.mRefresh = true;
			
			db.DisConnect();
//			RemoveThread(t);
			return NULL;
		} else {
			gConfig.mDatabaseFails++;
		}
		usleep(1000000);
	}
//	RemoveThread(t);
	return NULL;			
}

void SetServiceDate( void )
{
	pthread_t tID;
	int error = pthread_create(&tID, NULL, &SetServiceDateThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Refresh Service Date Thread\n";
	}
}

void* RefreshRequestsThread( void* arg )
{
	char temp[150];
	CDataSource db;
	
//	void* t = AddThread((char*)"Requests Thread");
	while( true ){
	
		if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){

			printf("Refreshing Requests\n");
			
			int i;
			CRequest r;
			for( i = 0; i < gRequestList.Count(); i++ ){
				CRequest* ptr = (CRequest*)gRequestList.GetAt(i);
				delete ptr;
			}
			gRequestList.Clear();
			
			sprintf( temp,"select * from `requests` where `kioskname` like '%s' order by `requestdate` desc limit 8", gConfig.mKioskName);
			r.GetBatch(&db,temp,&gRequestList);
			
			gConfig.mDatabaseFails = 0;
			db.DisConnect();
//			RemoveThread(t);
			return NULL;
		} else {
			gConfig.mDatabaseFails++;
		}
		usleep(1000000);
	}
//	RemoveThread(t);
	return NULL;			
}

void RefreshRequests( void )
{
	pthread_t tID;
	gConfig.mRefreshRequests = false;
			
	int error = pthread_create(&tID, NULL, &RefreshRequestsThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Refresh Requests Thread\n";
	}
}

void* UploadRequestThread( void* arg )
{
	CDataSource db;
	CRequest* req = (CRequest*)arg;

//	void* t = AddThread((char*)"Upload Requests Thread");
	while( true ){
		if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
			req->PutData(&db);
			db.DisConnect();
			delete req;
			gConfig.mRefreshRequests = true;
			gConfig.mDatabaseFails = 0;
//			RemoveThread(t);
			return NULL;
		} else {
			gConfig.mDatabaseFails++;
		}
		usleep(1000000);
	}
//	RemoveThread(t);
	return NULL;			
}

void UploadRequest( char* text )
{
	pthread_t tID;
	CRequest* req = new CRequest;
	
	strcpy(req->mRequest,text);
	strcpy(req->mKioskName,gConfig.mKioskName);
	
	int error = pthread_create(&tID, NULL, &UploadRequestThread, (void*)req );
	if( error != 0 ){
		std::cout << "Can't Create Upload Requests Thread\n";
	}
}

void* HelpVideoThread( void* arg )
{
	while( true ){
		if( gConfig.mPlayHelpVideo ){
			gConfig.mPlayHelpVideo = false;
			if( !VideoPlaying() ){
				system( "./playhelp.sh" );
			}
		}
		usleep(100000);
	}
	return NULL;			
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
// sudo iwlist wlan0 scan
//
// /etc/wpa_supplicant/wpa_supplicant.conf               
//
//ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
//update_config=1
//country=US

//network={
//        ssid="NachoWifi"
//        psk="needtoknow"
//        key_mgmt=WPA-PSK
//}
//
// /sbin/ifdown wlan0
// /sbin/ifup wlan0
//
// sudo sed s/psk=.*/psk=\"XXXX\"/ /etc/wpa_supplicant/wpa_supplicant.conf
//

void GetPrinterStatus()
{
	int fd, rb;
	char ret[100];
	char cmd[3] = {0x10, 0x04, 0x01};
	
	system( "sudo chmod 666 /dev/usb/lp0" );
	if( fd = open( "/dev/usb/lp0", O_WRONLY) ){
		write(fd,cmd,3);
		close(fd);
	}
	usleep(250000);
	if( fd = open( "/dev/usb/lp0", O_RDONLY) ){
		rb=read(fd,(void*)&ret,sizeof(ret));
		printf( "Bytes Returned: %d\n", rb );
		printf( "Byte Val: %d %d\n", ret[0], ret[1] );
		close(fd);
	}
}

bool PrinterAvail( void )
{
	FILE* fp;
	char* pos;
	char current[100];
	
	current[0] = 0;
	printf( "Checking For Printer\n" );
	if( fp = popen("ls /dev/usb", "r") ){
		while( fgets(current, sizeof(current)-1, fp) ){
			if( strstr(current, "lp0") != NULL ){
				if( !gConfig.mPrinterAvailable ) gConfig.mRefresh = true;
				gConfig.mPrinterAvailable = true;
				return true;
			}
		}
		pclose(fp);
	}
	if( gConfig.mPrinterAvailable ) gConfig.mRefresh = true;
	gConfig.mPrinterAvailable = false;
	return false;
}

bool CheckCronTab( void )
{
	FILE* fp;
	char* pos;
	char line[100];
	
	line[0] = 0;
	printf( "Checking For Our Cron Task\n" );
	if( fp = popen("crontab -l | grep reboot.sh", "r") ){
		while(fgets(line, sizeof(line)-1, fp) ){
			printf( "[%s]\n", line );
			if( !strcmp("*/1 * * * * /bin/bash /home/pi/Desktop/PiMicroMarket/reboot.sh\n",line) ){
				printf( "Our Crontab Is Running!\n" );			
				pclose(fp);
				return true;
			}
		}
		pclose(fp);
	}
	UserMessage((char*)"Setup Issue: Cron Tab Not Running");
	return false;
}

bool AdapterExsists( char* name )
{
	FILE *fp;
	char cmd[100], line[100];
	
	sprintf( cmd, "ifconfig | grep '%s'", name );
	if( fp = popen(cmd,"r") ){
		while( fgets(line,sizeof(line),fp) ){
			if( (strstr( line, name )) > 0 ){
				fclose(fp);
				return true;
			}
		}
		fclose(fp);
	}
	return false;
}

int GetAdapterInfo( char* name, char* val )
{
	FILE* fp;
	char* pos1;
	char* pos2;
	char current[100], cmd[100];
	
	current[0] = 0;
	sprintf( cmd, "ifconfig %s | grep 'inet addr:'", name );
	if( fp = popen(cmd, "r") ){
		fgets(current, sizeof(current)-1, fp);
		if((pos1=strstr(current, "inet addr:")) != NULL){
			pos1+=10;
			if((pos2=strchr(pos1,' ')) != NULL) *pos2 = '\0';
			strcpy(val,pos1);
			pclose(fp);
			return 1;
		}
		pclose(fp);
	}
	return 0;
}

int GetAdapterName( char* name, char* val )
{
	FILE* fp;
	char* pos1;
	char* pos2;
	char current[100], cmd[100];
	
	sprintf( cmd, "iwconfig %s | grep 'ESSID:'", name );
	if( fp = popen(cmd, "r") ){
		fgets(current, sizeof(current)-1, fp);
		if((pos1=strstr(current, "ESSID:")) != NULL){
			pos1+=6;
			if((pos2=strchr(pos1,' ')) != NULL) *pos2 = '\0';
			if( strcmp(val,pos1) ) gConfig.mRefresh = 1;
			strcpy(val,pos1);
			pclose(fp);
			return 1;
		}
		pclose(fp);
	}
	return 0;
}

bool GetOurIP( void )
{	
	int v;
//	printf( "Checking Our IP Addresse(s)\n" );
	
	v = GetAdapterInfo((char*)"eth0",gConfig.mEtheIPAddress0 );
	if( v != gConfig.mEtheHasIP0 ) gConfig.mRefresh = 1;
	gConfig.mEtheHasIP0 = v;
	
	if( AdapterExsists( (char*)"eth1" ) ){
		v = GetAdapterInfo((char*)"eth1",gConfig.mEtheIPAddress1 );
		if( v != gConfig.mEtheHasIP1 ) gConfig.mRefresh = 1;
		gConfig.mEtheHasIP1 = v;
	}
	
	v = GetAdapterInfo((char*)"wlan0",gConfig.mWIFIIPAddress0 );
	if( v ) GetAdapterName((char*)"wlan0",gConfig.mWIFISSID0);
	if( v != gConfig.mWIFIHasIP0 ) gConfig.mRefresh = 1;
	gConfig.mWIFIHasIP0 = v;

	if( AdapterExsists( (char*)"wlan1" ) ){
		v = GetAdapterInfo((char*)"wlan1",gConfig.mWIFIIPAddress1 );
		if( v ) GetAdapterName((char*)"wlan1",gConfig.mWIFISSID1);
		if( v != gConfig.mWIFIHasIP1 ) gConfig.mRefresh = 1;
		gConfig.mWIFIHasIP1 = v;
	}	

	if( gConfig.mEtheHasIP0 ) return true;
	if( gConfig.mEtheHasIP1 ) return true;
	if( gConfig.mWIFIHasIP0 ) return true;
	if( gConfig.mWIFIHasIP1 ) return true;
	return false;
}

void* ConnectToNetworkThread( void* arg )
{
	int i;
	FILE *fp;
	char cmd[200];
	char newssid[80];
	
	newssid[0] = 0;
	
//	void* t = AddThread((char*)"Connect To Network Thread");	
	
	for( i = 0; i < gWiFIList.Count(); i++ ){
		CWiFiName* ptr = (CWiFiName*)gWiFIList.GetAt(i);
		if( ptr->mChosen ){
			strcpy(newssid,ptr->mSSIDName);
			system( (char*)"sudo /sbin/ifdown wlan0" );
			
			sprintf( cmd, "sudo sed -i s/ssid=.*/ssid=\\\"%s\\\"/ /etc/wpa_supplicant/wpa_supplicant.conf", ptr->mSSIDName );
			system( cmd );
			
			sprintf( cmd, "sudo sed -i s/psk=.*/psk=\\\"%s\\\"/ /etc/wpa_supplicant/wpa_supplicant.conf", ptr->mPassword );
			system( cmd );
			
			system( (char*)"sudo /sbin/ifup wlan0" );
		}
	}
	
	WorkingOn( (char*) "Connecting..." );
	if( strlen(newssid) ){
		char* pos;
		char current[80];
		int timeout = 30;
		while( timeout > 0 ){
			usleep(1000000);		// Wait 1/100th second		
			timeout--;
			current[0] = 0;
			if( fp = popen("iwgetid -r", "r") ){
				fgets(current, sizeof(current)-1, fp);
				pclose(fp);
				if ((pos=strchr(current, '\n')) != NULL) *pos = '\0';
				if( strcmp(current,newssid) == 0 ){
					GetOurIP();
					WorkingOff();
					sprintf( cmd, "Connected To: %s", newssid );
					UserMessage(cmd);
//					RemoveThread(t);
					return NULL;
				}
			}
		}
	}
	
	gConfig.mEtheIPAddress0[0] = 0;
	gConfig.mEtheIPAddress1[0] = 0;
	gConfig.mWIFIIPAddress0[0] = 0;
	gConfig.mWIFIIPAddress1[0] = 0;
	
	WorkingOff();
	sprintf( cmd, "Failed To Connect To %s", newssid );
	UserMessage(cmd);
//	RemoveThread(t);
	return NULL;
}

void ConnectToNetwork( void )
{
	pthread_t tID;
	int error = pthread_create(&tID, NULL, &ConnectToNetworkThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Connect To Network Thread\n";
	}
}

void* ScanWifiThread( void* arg )
{
	int i;
	FILE *fp;
	char name[80];
	char current[80];
	char line[1035];

//	void* t = AddThread((char*)"Scan WiFi Thread");

	for( i = 0; i < gWiFIList.Count(); i++ ){
		CWiFiName* ptr = (CWiFiName*)gWiFIList.GetAt(i);
		delete ptr;
	}
	gWiFIList.Clear();

	gConfig.mRefresh = true;

	current[0] = 0;
	if( fp = popen("iwgetid -r", "r") ){
		fgets(current, sizeof(current)-1, fp);
		pclose(fp);
	}
	
	char* pos;
	if ((pos=strchr(current, '\n')) != NULL) *pos = '\0';
	printf( "Connected: '%s'\n", current );

	if( fp = popen("sudo iwlist wlan0 scan", "r") ){
		while (fgets(line, sizeof(line)-1, fp) != NULL) {
			char* e = strstr( line, "ESSID:" );
			if( e ){
				e = strchr(e,'\"');
				e++;
				*strchr(e,'\"') = 0;
				CWiFiName *ptr = new CWiFiName;
				strcpy(ptr->mSSIDName,e);
				ptr->mConnected = ( strcmp(e,current) == 0 );
				gWiFIList.Add(ptr);
				printf( "Found: %s\n", e );
			}
		}
		pclose(fp);
	}
	
	gWiFIList.mLoading = false;
	gConfig.mRefresh = true;		
	
//	RemoveThread(t);
	
	return NULL;
}

void ScanWifi( void )
{
	pthread_t tID;
	int error = pthread_create(&tID, NULL, &ScanWifiThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Scan Wifi Thread\n";
	}
}

void AppendWiFiCredToEmail( char* file )
{
	FILE* fp;
	FILE* pfp;
	char line[1024];
	if( fp = fopen( file, "a" ) ){
		fprintf( fp, "\n#### Kioksk WPA File ####\n" );
		if( pfp = popen("sudo cat /etc/wpa_supplicant/wpa_supplicant.conf", "r") ){
			while (fgets(line, sizeof(line)-1, pfp) != NULL) {
				fprintf( fp, "%s", line );
			}
			pclose(pfp);
		}		
		fclose(fp);
	}
}
/*
bool NetConnection( void )
{
	FILE *fp;
	char cmd[80];
	char line[1035];
	
	if( fp = popen("/sbin/ifconfig wlan0| grep 'inet addr:' | wc -l", "r") ){
		while (fgets(line, sizeof(line)-1, fp) != NULL) {
			gConfig.mWIFIHasIP0 = atoi(line);
		}
		pclose(fp);
	}
	printf( "     WIFI (0) Has IP: %d\n", gConfig.mWIFIHasIP0 );
	
	if( AdapterExsists( (char*)"wlan1" ) ){
		if( fp = popen("/sbin/ifconfig wlan1| grep 'inet addr:' | wc -l", "r") ){
			while (fgets(line, sizeof(line)-1, fp) != NULL) {
				gConfig.mWIFIHasIP1 = atoi(line);
			}
			pclose(fp);
		}
		printf( "     WIFI (1) Has IP: %d\n", gConfig.mWIFIHasIP1 );
	}
		
	if( fp = popen("/sbin/ifconfig eth0| grep 'inet addr:' | wc -l", "r") ){
		while (fgets(line, sizeof(line)-1, fp) != NULL) {
			gConfig.mEtheHasIP0 = atoi(line);
		}
		pclose(fp);
	}
	printf( " Ethernet (0) Has IP: %d\n", gConfig.mEtheHasIP0 );
	
	if( AdapterExsists( (char*)"eth1" ) ){	
		if( fp = popen("/sbin/ifconfig eth1| grep 'inet addr:' | wc -l", "r") ){
			while (fgets(line, sizeof(line)-1, fp) != NULL) {
				gConfig.mEtheHasIP1 = atoi(line);
			}
			pclose(fp);
		}
		printf( " Ethernet (1) Has IP: %d\n", gConfig.mEtheHasIP1 );
	}	
	
	if( gConfig.mEtheHasIP0 == 1 || 
		gConfig.mEtheHasIP1 == 1 || 
		gConfig.mWIFIHasIP0 == 1 || 
		gConfig.mWIFIHasIP1 == 1 ){
		return true;
	}
	return false;
}
*/
void* NetStatsThread( void* arg )
{
	FILE *fp;
	char cmd[80];
	char line[1035];

	printf("###### Starting NetStat ######\n");
	printf( "  Database Fails: %d\n", gConfig.mDatabaseFails );
	
	if( fp = popen("/sbin/ifconfig wlan0| grep 'inet addr:' | wc -l", "r") ){
		while (fgets(line, sizeof(line)-1, fp) != NULL) {
			gConfig.mWIFIHasIP1 = atoi(line);
		}
		pclose(fp);
	}
	printf( "     WIFI (0) Has IP: %d\n", gConfig.mWIFIHasIP0 );
	
	if( fp = popen("/sbin/ifconfig wlan1| grep 'inet addr:' | wc -l", "r") ){
		while (fgets(line, sizeof(line)-1, fp) != NULL) {
			gConfig.mWIFIHasIP0 = atoi(line);
		}
		pclose(fp);
	}
	printf( "     WIFI (1) Has IP: %d\n", gConfig.mWIFIHasIP1 );
	
	if( fp = popen("/sbin/ifconfig eth0| grep 'inet addr:' | wc -l", "r") ){
		while (fgets(line, sizeof(line)-1, fp) != NULL) {
			gConfig.mEtheHasIP0 = atoi(line);
		}
		pclose(fp);
	}
	printf( " Ethernet 0 Has IP: %d\n", gConfig.mEtheHasIP0 );
	
	if( fp = popen("/sbin/ifconfig eth1| grep 'inet addr:' | wc -l", "r") ){
		while (fgets(line, sizeof(line)-1, fp) != NULL) {
			gConfig.mEtheHasIP1 = atoi(line);
		}
		pclose(fp);
	}
	printf( " Ethernet 1 Has IP: %d\n", gConfig.mEtheHasIP1 );
	
	sprintf( cmd, "traceroute -p 3306 %s", gConfig.mDBHost );
	if( fp = popen(cmd, "r") ){
		gConfig.mTraRte3306 = 0;
		while (fgets(line, sizeof(line)-1, fp) != NULL) {
			if( strstr(line,gConfig.mDBHost) ){
				gConfig.mTraRte3306 = 1;
			}
		}
		pclose(fp);
	}
	printf( "Trace Route 3306: %d\n", gConfig.mTraRte3306 );
	printf("###### Finished NetStat######\n");

//	RemoveThread(t);
	return NULL;
}

void GetNetStats( void )
{
	pthread_t tID;
	int error = pthread_create(&tID, NULL, &NetStatsThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Net Stats Thread\n";
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* FindSIDBThread( void* arg )
{
	char* barcode = (char*)arg;
	CDataSource db;	
		
	if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
		CSalesItem * ptr = new CSalesItem;
		if( ptr->GetByBarcode(&db,barcode) ){
			gSalesItemList.Add(ptr);
		}else delete ptr;
		gConfig.mDatabaseFails = 0;
		db.DisConnect();
	}else {
		gConfig.mDatabaseFails++;
		return NULL;
	}
	
	int l;
	char temp[20];
	
	l = strlen(barcode);
	if( l > 20 ){
		
		strcpy(temp,&barcode[1]);
		temp[l-2] = 0;
		
		if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
			CSalesItem * ptr = new CSalesItem;
			if( ptr->GetByBarcode(&db,temp) ){
				gSalesItemList.Add(ptr);
			}else delete ptr;
			gConfig.mDatabaseFails = 0;
			db.DisConnect();
		}else {
			gConfig.mDatabaseFails++;
			return NULL;
		}
	}
	
	return NULL;
}

bool FindSIDB( char* inputline )
{
	pthread_t tID;
	
	char barcode[50];
	strcpy(barcode,inputline);
	
	int error = pthread_create(&tID, NULL, &FindSIDBThread, (void*)barcode );
	if( error != 0 ){
		std::cout << "Can't Create Saled Item Search Thread\n";
		return false;
	}
	
	// Wait a half second to see if our thread finds this sales item
	int t = 50;
	int cnt = gSalesItemList.Count();
	while( true ){
		usleep(10000);		// Wait 1/100th second
		if( gSalesItemList.Count() > cnt ) return true;	
		if( t == 0 ) return false;		
		t--;
	}
	
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* UploadUnknownBarcodeThread( void* arg )
{
	CDataSource db;	
	CUnknownBarcode *uk = (CUnknownBarcode*)arg;
//	void* t = AddThread((char*)"Upload Unknown Barcode Thread");	
	while( true ){
		if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
			uk->PutData(&db);
			db.DisConnect();
			delete uk;
			gConfig.mDatabaseFails = 0;
//			RemoveThread(t);
			return NULL;
		}else gConfig.mDatabaseFails++;
		usleep(1000000*10);	// Wait 10 Seconds and Retry....
	}
//	RemoveThread(t);
	return NULL;
}

bool UploadUnknownBarcode( char* inputline )
{
	pthread_t tID;
	
	CUnknownBarcode *uk = new CUnknownBarcode;
	strcpy(uk->mKioskName,gConfig.mKioskName);
	strcpy(uk->mBarcode,inputline);
	
	int error = pthread_create(&tID, NULL, &UploadUnknownBarcodeThread, (void*)uk );
	if( error != 0 ){
		std::cout << "Can't Create Upload Barcode Thread\n";
		return false;
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* UplaodCCErrorThread( void* arg )
{
	CDataSource db;	
	CCCError *err = (CCCError*)arg;
//	void* t = AddThread((char*)"Upload CC Error Thread");	
	while( true ){
		if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
			err->PutData(&db);
			db.DisConnect();
			delete err;
			gConfig.mDatabaseFails = 0;
//			RemoveThread(t);
			return NULL;
		}else gConfig.mDatabaseFails++;
		usleep(1000000*10);	// Wait 10 Seconds and Retry....
	}
//	RemoveThread(t);
	return NULL;
}

bool UplaodCCError( CCCError* err )
{
	pthread_t tID;
	int error = pthread_create(&tID, NULL, &UplaodCCErrorThread, (void*)err );
	if( error != 0 ){
		std::cout << "Can't Create Upload CCError Thread\n";
		return false;
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void EmailReport( void )
{
	char cmd[100];
	
	if( !gDBText.mOperatorEmail ) return;
	
	sprintf( cmd, "echo \"From: %s\" > ./config/email.txt", gDBText.mOperatorEmail ); system( cmd );
	sprintf( cmd, "echo \"To: %s\" >> ./config/email.txt", gDBText.mOperatorEmail ); system( cmd );
	sprintf( cmd, "echo \"Subject: Kiosk %s\" >> ./config/email.txt", gConfig.mKioskName ); system( cmd );
	
	AppendConfigToEmail( (char*)"./config/email.txt" );
	
	AppendDeviceListToEmail( (char*)"./config/email.txt" );
	
	system( "echo \"\n#### USB Devices ####\" >> ./config/email.txt" );
	system( "lsusb >> ./config/email.txt" );
	
//	AppendThreadListToEmail( (char*)"./config/email.txt" );
	
	system( "echo \"\n#### Uptime ####\" >> ./config/email.txt" );
	system ( "uptime >> ./config/email.txt" );	
	
	system( "echo \"\n#### CPU Info ####\" >> ./config/email.txt" );
	system ( "vcgencmd measure_temp >> ./config/email.txt" );	
	system ( "vcgencmd measure_volts core >> ./config/email.txt" );
	
	system( "echo \"\n#### Memory Info ####\" >> ./config/email.txt" );
	system ( "free -o -h >> ./config/email.txt" );	
	
//	system( "echo \"\n#### supplicant confif file ####\" >> ./config/email.txt" );
//	system( "cat /etc/wpa_supplicant/wpa_supplicant.conf >> ./config/email.txt" );
	
	AppendWiFiCredToEmail( (char*)"./config/email.txt" );
	
	system( "echo \"\n#### Connected Networks ####\" >> ./config/email.txt" );
	system( "iwgetid >> ./config/email.txt" );
	system( "ifconfig >> ./config/email.txt" );
	
	system( "echo \"\n#### Available Networks ####\" >> ./config/email.txt" );
	system( "sudo iwlist wlan0 scan | grep -v Unknown: | grep -v Frequency: | grep -v Extra: | grep -v Encription: | grep -v IE: | grep -v Mb/s | grep -v Cipher | grep -v Mode: | grep -v Auth | grep -v Encrition: >> ./config/email.txt" );
	
	system( "echo \"\n#### Trace Route -p 3306 ####\" >> ./config/email.txt" );
	sprintf( cmd, "traceroute -p 3306 %s >> ./config/email.txt", gConfig.mDBHost );
	system( cmd );
		
	sprintf( cmd, "./gdrive upload ./config/email.txt", gDBText.mOperatorEmail );
	system( cmd );
	
	printf( "Report Sent\n" );
}

void* DataBaseThread( void* arg )
{
	int kon = 0;
	int timer = 5;
	char temp[250];
	char pstat[50];
	char command[50];
	CDataSource db;
	bool startup = true;
	
//	void* t = AddThread((char*)"Database Heartbeat Thread");	

	PrinterAvail();
	
	while( true ){
		if( timer < 0 ){
				
			kon = 0;
			if( access( "./reboot.sh", F_OK ) != -1 ) kon = 1;
							
			if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
				
				GetOurIP();
				PrinterAvail();
				
				strcpy(pstat,"unknown");
				if( gConfig.mPrinterAvailable ) strcpy(pstat,"ready");
				
				if( gConfig.mKioskNameDBID ){			// Update The Heartbeat field in the database...
					if( startup ){
						sprintf(temp,"update `kiosknames` set `active` = '1', `heartbeat` = NOW(), `laststartup` = NOW(), `startupcount` = `startupcount`+ 1, `printerstat` = '%s' , `kon` = '%d' where `id` = '%d'", pstat, kon, gConfig.mKioskNameDBID );
						startup = false;
					}else{
						sprintf(temp,"update `kiosknames` set `active` = '1', `heartbeat` = NOW(), `printerstat` = '%s' , `kon` = '%d' where `id` = '%d'", pstat, kon, gConfig.mKioskNameDBID );
					}
					db.MySQLQuery(temp);

					sprintf(temp,"select `command` from `kiosknames` where `id` = '%d'", gConfig.mKioskNameDBID );
					db.GetSingleString(temp,command,50);
					if( strlen( command ) ){
						sprintf(temp,"update `kiosknames` set `command` = '' where `id` = '%d'", gConfig.mKioskNameDBID );
						db.MySQLQuery(temp);
						
						if( strcmp( command, "REBOOT" ) == 0 ){
							sprintf(temp,"update `kiosknames` set `response` = 'REBOOTING' where `id` = '%d'", gConfig.mKioskNameDBID );
							db.MySQLQuery(temp);
							db.DisConnect();
							sleep(1);
							Log("Get Reboot Command From Database, Rebooting");
							system( "sudo reboot" );
						}
						
						if( strcmp( command, "REPORT" ) == 0 ){
							sprintf(temp,"update `kiosknames` set `response` = 'REPORTING' where `id` = '%d'", gConfig.mKioskNameDBID );
							db.MySQLQuery(temp);
							EmailReport();
						}
					}
				}
								
				db.DisConnect();
				timer = 5*60;		// Restart the 5 Minute Timeout...
				gConfig.mDatabaseFails = 0;
				
			}else{
				gConfig.mDatabaseFails++;
				timer = 10;
//				GetNetStats();
			}
		}else timer--;
		usleep(1000000);			// One Second Sleep...
		
		if( gConfig.mCheckIP ||
			( gConfig.mEtheHasIP0 == 0 &&
			  gConfig.mEtheHasIP1 == 0 &&
			  gConfig.mWIFIHasIP0 == 0 &&
			  gConfig.mWIFIHasIP1 == 0 )
			){
			GetOurIP();
			gConfig.mCheckIP = 0;
		}
	}	
//	RemoveThread(t);
	return NULL;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* UploadReceiptThread( void* arg )
{
	int i;
	CDataSource db;
	CReceipt* r = (CReceipt*)arg;
//	void* t = AddThread((char*)"Upload Receipt Thread");

	while( true ){
		if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){

			r->PutData(&db);
			for( i = 0; i < r->mReceiptItems.Count(); i++ ){
				CReceiptItem *ri = (CReceiptItem*)r->mReceiptItems.GetAt(i);
				ri->mReceiptID = r->mDBID;
				ri->PutData(&db);
				delete ri;
			}
			r->mReceiptItems.Clear();
			delete r;

			db.DisConnect();
			gConfig.mDatabaseFails = 0;
//			RemoveThread(t);
			return NULL;
		}else gConfig.mDatabaseFails++;
		usleep(1000000*10);	// Wait 10 Seconds and Retry....
	}
//	RemoveThread(t);
	return NULL;
}

bool UploadReceipt( CReceipt* r )
{
	pthread_t tID;	
	int error = pthread_create(&tID, NULL, &UploadReceiptThread, (void*)r );
	if( error != 0 ){
		std::cout << "Can't Create Upload Receipt Thread\n";
		return false;
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void DeleteCart( void )
{
	int i;
	for( i = 0; i < gCart.Count(); i++ ){
		CReceiptItem* ri = (CReceiptItem*)gCart.GetAt(i);
		delete ri;
	}
	gCart.Clear();
}

void AddToCart( CSalesItem* ptr )
{
	int j;
	bool dup = false;

	for( j = 0; j < gCart.Count(); j++ ){
		CReceiptItem* ri = (CReceiptItem*)gCart.GetAt(j);
		if( ri->mSalesItemID ==  ptr->mDBID ){
			ri->mQty++;
			gConfig.mRefresh = true;
			dup = true;
			break;
		}
	}
	
	if( !dup ){
		CReceiptItem* ri = new CReceiptItem;
		gCart.Add(ri);
		
		ri->mSalesItemID = ptr->mDBID;
		strcpy( ri->mKioskName, gConfig.mKioskName );
		strcpy( ri->mDescription, ptr->mDescription );
		strcpy( ri->mBarcode, ptr->mBarcode );
		strcpy( ri->mTaxType, ptr->mTaxType );
		ri->mUnitPrice = ptr->mUnitPrice;
		ri->mInvoicePrice = ptr->mInvoicePrice;
		ri->mLocalPriceId = ptr->mLocalPriceId;
		ri->mDeposit = ptr->mDeposit;
		ri->mQty = 1;
										
	}	
}

int CartTax( void )
{
	int i, j;
	int taxtotal = 0;
	for( i = 0; i < gCart.Count(); i++ ){
		CReceiptItem* ptr = (CReceiptItem*)gCart.GetAt(i);		
		for( j = 0; j < gTaxTypes.Count(); j++ ){
			CTaxTable *tt = (CTaxTable*)gTaxTypes.GetAt(j);
			if( !strcasecmp(tt->mTaxType,ptr->mTaxType) ){
				taxtotal += tt->mTaxRate*ptr->mUnitPrice*ptr->mQty;
			}
		}
	}
	return taxtotal;
}

int CartSubTotal( void )
{
	int i;
	int subtotal = 0;
	for( i = 0; i < gCart.Count(); i++ ){
		CReceiptItem* ptr = (CReceiptItem*)gCart.GetAt(i);		
		subtotal += ptr->mUnitPrice*ptr->mQty;
	}
	return (subtotal);
}

int CartTotal( void )
{
	return (CartSubTotal()+CartTax());
}

void CutPaper()
{
	int fd;
	char cmd[4] = {0x1D,0x56,0x41,0x00};
	
	system( "sudo chmod 666 /dev/usb/lp0" );
	if( fd = open( "/dev/usb/lp0", O_WRONLY) ){
		write(fd,cmd,4);
		close(fd);
	}
}

void PrintReceipt( void )
{
	int fd;
	int i, p;
	FILE* fp;
	char temp[40];
	char desc[40];
	char qty[10];
	char line[60];
	
	system( "sudo chmod 666 /dev/usb/lp0" );
	system( "./png2pos -p ./images/graphics/head.png > /dev/usb/lp0" );
	
	if( fd = open( "/dev/usb/lp0", O_WRONLY) ){
//	if( fp = fopen( "./config/receipt.txt","w" ) ){
		sprintf( line, " %30s \n", gReceipt.mReceiptDate.Format() );		write(fd,line,strlen(line));
		sprintf( line, " Qty Item                           Price\n");		write(fd,line,strlen(line));
		sprintf( line, "------------------------------------------\n");		write(fd,line,strlen(line));
		for( i = 0; i < gCart.Count(); i++ ){
			CReceiptItem* ri = (CReceiptItem*)gCart.GetAt(i);
			int p = ri->mUnitPrice*ri->mQty;
			strncpy(desc,ri->mDescription,30);
			sprintf( temp, "$%d.%02d", p/100, p%100 );
			sprintf( qty, "%d", ri->mQty );
			sprintf( line, " %2s %-30s %6s\n", qty, desc, temp );
			write(fd,line,strlen(line));
		}
		
		sprintf( line, "                         -----------------\n" );
		write(fd,line,strlen(line));
		
		p = CartSubTotal();
		sprintf( temp, "$%d.%02d", p/100, p%100 );
		sprintf( line, "                       Sub Total: %7s\n", temp );
		write(fd,line,strlen(line));
		
		p = CartTax();
		sprintf( temp, "$%d.%02d", p/100, p%100 );
		sprintf( line, "                             Tax: %7s\n", temp );	write(fd,line,strlen(line));
		
		p = CartTotal();
		sprintf( temp, "$%d.%02d", p/100, p%100 );
		sprintf( line, "                           Total: %7s\n", temp );
		write(fd,line,strlen(line));
		
		if( strlen(gReceipt.mCustomerName) ){
			sprintf( line, "      Name: %s\n", gReceipt.mCustomerName );
			write(fd,line,strlen(line));
		}
		if( strlen(gReceipt.mCCLastFour) ){
			sprintf( line, "      Card: %s\n", gReceipt.mCCLastFour );
			write(fd,line,strlen(line));
		}
		if( strlen(gReceipt.mAuthCode) ){
			sprintf( line, " Auth Code: %s\n\n", gReceipt.mAuthCode );
			write(fd,line,strlen(line));
		}
		sprintf( line, "\n\n" );											
		write(fd,line,strlen(line));
		close(fd);
	}
	
	CutPaper();
//	system( "sudo cat cut.txt > /dev/usb/lp0" );
}

void EmailReceipt( char* email )
{
	int i, p;
	FILE* fp;
	char cmd[50];
	
	if( fp = fopen( "./config/receipt.txt","w" ) ){
		fprintf( fp, "Date: %s\n\n", gReceipt.mReceiptDate.Format() );
		fprintf( fp, "  Qty   Item                                              Price\n");
		fprintf( fp, " _______________________________________________________________\n");
		for( i = 0; i < gCart.Count(); i++ ){
			CReceiptItem* ri = (CReceiptItem*)gCart.GetAt(i);
			int p = ri->mUnitPrice*ri->mQty;
			fprintf( fp, "  %d   %-50s  $%d.%02d\n", ri->mQty, ri->mDescription, p/100, p%100 );
		}
		fprintf( fp, "                                              __________________\n" );
		p = CartSubTotal();
		fprintf( fp, "                                               Sub Total: $%d.%02d\n", p/100, p%100 );
		p = CartTax();
		fprintf( fp, "                                                     Tax: $%d.%02d\n", p/100, p%100 );
		p = CartTotal();
		fprintf( fp, "                                                   Total: $%d.%02d\n", p/100, p%100 );
		fclose(fp);
	}
	sprintf( cmd, "mail -s \"Micro Market Receipt\" %s < ./config/receipt.txt", email );
	system( cmd );
	
}

void FinishPayRangeSale( void )
{
	// Credit Local Accounts...
	CreditLocalAccounts();

	// Save The Receipt...
	int i;
	CReceipt* r = new CReceipt;
	printf( "New Receipt: %d\n", r );
	r->Copy(&gReceipt);
	r->mTax = CartTax();
	r->mSubtotal = CartSubTotal();
	strcpy(r->mKioskName,gConfig.mKioskName);
	strcpy(r->mOperator,gConfig.mOperator);
	if( strlen(gReceipt.mCustomerName) > 0 ){
		if( strlen(gReceipt.mCustomerName) < 50 ){
			strcpy(r->mCustomerName,gReceipt.mCustomerName);
		}
	}
	
	for( i = 0; i < gCart.Count(); i++ ){
		CReceiptItem* ri = (CReceiptItem*)gCart.GetAt(i);
		strcpy(ri->mOperator,gConfig.mOperator);
		r->mReceiptItems.Add(ri);
	}
	
	UploadReceipt(r);
	
	gCart.Clear();
	gReceipt.ClearAll();
}

void FinishSale( void )
{
	printf( "Begin Finish Sale\n" );
	
	// Credit Local Accounts...
	CreditLocalAccounts();
	
	printf( "Setting Receipt Date\n" );
	gReceipt.mReceiptDate.Set();
	
	if( strlen(gReceipt.mEMail) ){
		printf( "EMailing Receipt\n" );
		EmailReceipt(gReceipt.mEMail);
		UserMessage( (char*)"Your Receipt Has Been Emailed." );
	}

	// Save The Receipt...
	int i;
	CReceipt* r = new CReceipt;
	printf( "New Receipt: %d\n", r );
	r->Copy(&gReceipt);
	r->mTax = CartTax();
	r->mSubtotal = CartSubTotal();
	strcpy(r->mKioskName,gConfig.mKioskName);
	strcpy(r->mOperator,gConfig.mOperator);
	
//	if( strlen(gReceipt.mCustomerName) > 0 ){
//		if( strlen(gReceipt.mCustomerName) < 50 ){
//			strcpy(r->mCustomerName,gReceipt.mCustomerName);
//		}
//	}
	
	printf( "Adding Receipt Items\n" );
	for( i = 0; i < gCart.Count(); i++ ){
		CReceiptItem* ri = (CReceiptItem*)gCart.GetAt(i);
		strcpy(ri->mOperator,gConfig.mOperator);
		r->mReceiptItems.Add(ri);
	}
	
	printf( "Calling Upload Receipt\n" );
	UploadReceipt(r);
	
	if( strlen(gReceipt.mEMail) > 0 ){
		UserMessage((char*)"Your Receipt Has Been EMailed.\nThank You For Your Purchase.");
		gCart.Clear();
		gReceipt.ClearAll();
	}else{
	
		if( gConfig.mPrinterAvailable ){
			SetScreen(SCREEN_WANTPRNT);
		}else{
			UserMessage((char*)"Thank You For Your Purchase.");
			gCart.Clear();
			gReceipt.ClearAll();
		}	
	}
	
	printf( "Exiting Finish Sale\n" );
}

CSalesItem* FindSalesItem( char* barcode )
{
	int i;
	char temp[20];
	
	int l = strlen(barcode);
	if( l > 20 ) return NULL;
	
	strcpy( temp, &barcode[1] );
	temp[l-2] = 0;

	printf( "Barcode:  %s  %s\n", barcode, temp );

	int cnt = gSalesItemList.Count();
	for( i = 0; i < cnt; i++ ){
		CSalesItem* ptr = (CSalesItem*)gSalesItemList.GetAt(i);
		gDisplay.mScroll = i;
		if( !strcasecmp(barcode,ptr->mBarcode) ){
			return ptr;
		}
		if( !strcasecmp(temp,ptr->mBarcode) ){
			return ptr;
		}
	}
	return NULL;
}

bool PossibleBarcode( char* input )
{
	int i;
	int len = strlen(input);
	if( gConfig.mAllowNonStdBarcodes == 1 && len > 3 && len < 17 ) return true;
	if( len < 6 || len > 25 ) return false;
	for( i = 1; i < len; i++ ){	// Allow barcode to start with non-numeric
		if( !isdigit(input[i]) ){
			printf( "Not Barcode Digit %d\n", i );
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void ParseName( char* data, char* name )
{
	int i;
	name[0] = 0;
	char* n = strchr(data,'^')+1;
	if( n ){
		for( i = 0; i < 48; i++ ){
			if( n[i] == '^' ) return;
			name[i] = n[i];
			name[i+1] = 0;
		}
	}
}

void ParseCCLast4( char* data, char* last )
{
	int i;
	last[0] = 0;
	char* n = strchr(data,'%')+14;
	if( n ){
		for( i = 0; i < 4; i++ ){
			if( n[i] == '^' ) return;
			last[i] = n[i];
			last[i+1] = 0;
		}
	}
}

bool ChargeLocal( void )
{
	int total = CartTotal();
	printf( "Cart Total: $%d.%02d\n", total/100, total%100 );
	if( gLocalAccount.mDBID > 0 ){
		
		if( strlen(gReceipt.mEMail) ){
			strncpy(gLocalAccount.mEMail,gReceipt.mEMail,49);
		}else
		if( strlen(gLocalAccount.mEMail) ){
			strncpy(gReceipt.mEMail,gLocalAccount.mEMail,49);
		}
		
		printf( "Account Credit: $%d.%02d\n", gLocalAccount.mCredit/100, gLocalAccount.mCredit%100 );
		if( gLocalAccount.mCredit >= total ){
			
			gLocalAccount.mCredit -= total;
			gLocalAccount.mLastUseDate.Set();			
			strcpy(gLocalAccount.mLastUsedKioskName,gConfig.mKioskName);
			strcpy(gReceipt.mAuthCode,gLocalAccount.mBarcode);
			
			printf( "Saving Local Account\n" );
			SaveLocalAccount();
			return true;
		}
	}else{
		printf( "No Local Account To Charge\n" );
	}
	return false;
}

void CopyNoQ( char* dest, char* src )
{
	if( src == NULL ){
		dest[0] = 0;
		return;
	}
	
	int i, p;
	int l = strlen(src);
	
	p = 0;
	for( i = 0; i < l; i++ ){
		if( src[i] != '"' ){
			dest[p] = src[i]; p++;
			dest[p] = 0;
		}
	}
}

bool ReadyForCard( void )
{
	if( gConfig.mGatewayID == 0 ) return false;
	if( strlen( gConfig.mGatewayURL ) == 0 )return false;
	if( strlen( gConfig.mMerchantLogin ) == 0 ) return false;
	if( strlen( gConfig.mXTranKey ) == 0 ) return false;
	return true;
}
void LookupEmailFromCC( void )
{
	char temp[250];
	CDataSource db;
	
	if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){		
		sprintf( temp,"select `email` from `receipt` where `name` like '%s' and `cclastfour` like '%s' order by `receiptdate` desc limit 1", gReceipt.mCustomerName, gReceipt.mCCLastFour );
		db.GetSingleString( temp, gReceipt.mEMail, 50 );
		db.DisConnect();
	}
}

bool ChargeCC( char* track1 )
{
	int			ret_code = 0;
	MyICharge 	myICharge;
	char		stot[10];
	int 		total = CartTotal();
	
	char rc[256], version[10], code[10], subcode[10], rtext[100], auth[20], tran[20], foo[20];
	char message[100];
	
	printf( "Begin CC Charge Code\n" );

	ParseName(track1,gReceipt.mCustomerName);
	printf( "Customer Name: %s\n", gReceipt.mCustomerName );

	ParseCCLast4(track1,gReceipt.mCCLastFour);
	printf( "CC Last 4: %s\n", gReceipt.mCCLastFour );

	if( strlen(gReceipt.mEMail) == 0 ){
		LookupEmailFromCC();
	}

	sprintf( stot, "%d.%02d", total/100, total%100 );
	
	myICharge.SetGateway(gConfig.mGatewayID);	
	myICharge.SetGatewayURL(gConfig.mGatewayURL);
	myICharge.SetMerchantLogin(gConfig.mMerchantLogin);
	myICharge.SetMerchantPassword(gConfig.mXTranKey);

	printf( "\nTotal To Charge: $%s\n", stot );
//	printf( "Track1 Data: %s\n", track1 );
	
	myICharge.SetTransactionAmount(stot);
	myICharge.AddSpecialField("x_track1", track1 );

	myICharge.AddSpecialField("x_market_type", "2" );
	myICharge.AddSpecialField("x_device_type", "3" );
	myICharge.AddSpecialField("x_type", "AUTH_CAPTURE" );

	printf("\nPlease wait while your credit card is authorized...\n\n");

	myICharge.SetTimeout(30);
	
	if( !gConfig.mSandBox ){
		ret_code = myICharge.Sale();
	}else{
		printf( "SandBox Mode On\n" );
	}
	
	if(ret_code){
		
		printf( "End CC Charge Code - Error Start\n" );
		sprintf( rtext, "Error: [%i] %s\n", ret_code, myICharge.GetLastError() ); 
		
		CCCError* ccerr = new CCCError;
		sprintf(ccerr->mReturnCode,"%d",ret_code);
		strcpy(ccerr->mKioskName,gConfig.mKioskName);
		strcpy(ccerr->mOperator,gConfig.mOperator);
		strcpy(ccerr->mResponseCode,code);
		strcpy(ccerr->mErrorText,rtext);
		UplaodCCError( ccerr );
		
		ErrorMessage( rtext );
		
		printf( "End CC Charge Code - Error End\n" );
		
		return false;
	}
	
//	printf( "RawRequest: %s\n", myICharge.Config("RawRequest") );
//	printf( "RawResponnse: %s\n", myICharge.Config("RawResponse") );

	strcpy( rc, myICharge.Config("RawResponse") );

	printf( "Raw Response: %s\n", rc );

	CopyNoQ(version,	strtok(rc,"|"));
	CopyNoQ(code,		strtok(NULL,"|"));
	CopyNoQ(subcode,	strtok(NULL,"|"));
	CopyNoQ(rtext,		strtok(NULL,"|"));
	CopyNoQ(auth,		strtok(NULL,"|"));
	CopyNoQ(foo,		strtok(NULL,"|"));
	CopyNoQ(foo, 		strtok(NULL,"|"));
	CopyNoQ(tran,		strtok(NULL,"|"));

	printf( "\n" );
	printf("Version: %s\n", version );
	printf("RetCode: %s\n", code );
	printf("SubCode: %s\n", subcode );
	printf("Ret Txt: %s\n", rtext );
	printf("Auth No: %s\n", auth );
	printf("Tran No: %s\n", tran );
	printf( "\n" );

	if( !gConfig.mSandBox ){
		if( strcmp(code,"1") == 0 ){
			printf( "Approved\n", auth );
	//		ParseName(track1,gReceipt.mCustomerName);
	//		ParseCCLast4(track1,gReceipt.mCCLastFour);
			printf( "Authorization: %s\n", auth );
			strcpy(gReceipt.mAuthCode,auth);
			printf( "Transaction #: %s\n", tran );
			strcpy(gReceipt.mTranID,tran);
			printf( "End CC Charge Code - Approved\n" );			
			return true;
		}else{
			printf( "SandBox Mode\n", auth );
			return true;
		}
	}
	
	ErrorMessage( rtext );

	CCCError* ccerr = new CCCError;
	sprintf(ccerr->mReturnCode,"%d",ret_code);
	strcpy(ccerr->mKioskName,gConfig.mKioskName);
	strcpy(ccerr->mOperator,gConfig.mOperator);
	strcpy(ccerr->mResponseCode,code);
	strcpy(ccerr->mErrorText,rtext);
	UplaodCCError( ccerr );

	return false;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


void SetScreen( int state, int timeout )
{
	gConfig.mDisplayScreen = state;
	gConfig.mScreenTimeout = timeout;
	gConfig.mRefresh = true;
}

void GotInput( void )
{
	gConfig.mScreenTimeout = SCREEN_TIMEOUT;
	gConfig.mLanguageTimeout = SCREEN_TIMEOUT*3;
	return;
}

void ScreenTimeOut( void )
{
	if( !gConfig.mPassedStartupCheck ){
		SetScreen(SCREEN_STARTUP_CHK);
		return;
	}
	
	if( gConfig.mDisplayScreen == SCREEN_STOCKCNT 		||
		gConfig.mDisplayScreen == SCREEN_THEFT_CHECK	||
		gConfig.mDisplayScreen == SCREEN_ITEMCNT		||
		gConfig.mDisplayScreen == SCREEN_THEFTCNT 		||
		gConfig.mDisplayScreen == SCREEN_ENTER_KNAME	||
		gConfig.mDisplayScreen == SCREEN_EDIT_ADS		){
		return;
	}
	
	if( gConfig.mDisplayScreen == SCREEN_WELCOME ){
		SetScreen(SCREEN_WELCOME);
		return;
	}
	
	if( gConfig.mDisplayScreen == SCREEN_WANTPRNT ){
		gCart.Clear();
		gReceipt.ClearAll();
		SetScreen(SCREEN_SCANITEM);
		return;
	}

	if( gConfig.mDisplayScreen == SCREEN_SCANITEM ){
		if( gCart.Count() ){
			SetScreen( SCREEN_STLTHERE );
			return;
		}else{
			SetScreen( SCREEN_WELCOME );
			return;
		}
	}
	
	if( gConfig.mDisplayScreen == SCREEN_STLTHERE ){
		DeleteCart();
		gReceipt.mEMail[0] = 0;
		SetScreen( SCREEN_WELCOME );
		return;
	}
	
	if( gConfig.mDisplayScreen == SCREEN_LOCALACT	||
		gConfig.mDisplayScreen == SCREEN_ADDACCNT	){
		gLocalAccount.ClearAll();
		SetScreen( SCREEN_SCANITEM );
	}
	
	if( gConfig.mDisplayScreen == SCREEN_USERMSG	||
		gConfig.mDisplayScreen == SCREEN_ERRORSCR	||
		gConfig.mDisplayScreen == SCREEN_REQUEST	||
		gConfig.mDisplayScreen == SCREEN_REPPROBL	||
		gConfig.mDisplayScreen == SCREEN_ENTEMAIL	||
		gConfig.mDisplayScreen == SCREEN_REQUESTS	||
		gConfig.mDisplayScreen == SCREEN_PAY_RANGE  ||
		gConfig.mDisplayScreen == SCREEN_USERMSG_NB	){
		SetScreen( SCREEN_SCANITEM );
		return;
	}
	
	SetScreen(SCREEN_WELCOME);
}

void PayRangeMessage( char* msg )
{
	strcpy(gConfig.mMessageText,msg);
	SetScreen(SCREEN_PAY_RANGE);	
}

void UserMessage( char* msg, bool buttons )
{
	strcpy(gConfig.mMessageText,msg);
	if( buttons ){
		SetScreen(SCREEN_USERMSG);
	}else{
		SetScreen(SCREEN_USERMSG_NB);
	}
}

void ErrorMessage( char* msg )
{
	strcpy(gConfig.mMessageText,msg);
	SetScreen(SCREEN_ERRORSCR);
}

void* DsiplayThread( void* arg )
{
	int		mWorkingTimer = 0;
	bool 	hidden = true;
//	void* t = AddThread((char*)"Display Thread");	
	
	Pulse p( (char*) "Display Loop", 1000 );
	
	while( true ){
		
		if( gConfig.mDisplay && hidden ){
			gDisplay.Init();
			gConfig.mRefresh = true;
			hidden = false;
		}
		if( !gConfig.mDisplay && !hidden ){
			gDisplay.Hide();
			hidden = true;
		}
		
		if( gConfig.mWorking ){
			mWorkingTimer++;
		}else mWorkingTimer = 0;
		
		// Refresh the screen if needed...
		if( gConfig.mDisplay && gConfig.mWorking && mWorkingTimer%1000 == 0 ){
			gDisplay.DrawWorking();
		}else
		if( gConfig.mRefresh && gConfig.mDisplay ){
			gDisplay.DrawScreen();
		}else
		if( gConfig.mCaptureScreen && gConfig.mDisplay ){
			gDisplay.DrawScreen();
		}
		
		if( gConfig.mLanguageTimeout > 0 ){
			gConfig.mLanguageTimeout--;
			if( gConfig.mLanguageTimeout == 0 && gCurrentLanguage > 0 ){
				gDT.LoadLanguage(0);
				SetScreen(gConfig.mDisplayScreen);
			}
		}
		
		if( gConfig.mScreenTimeout > 0 ){
			gConfig.mScreenTimeout--;
			if( gConfig.mScreenTimeout == 0 ) ScreenTimeOut();
		}
		
		if( gConfig.mPulse ) p.Ping();
		
		usleep(1000);	// 1/1000 Second Sleep...		
	}

//	RemoveThread(t);
	
	return NULL; 
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* LoadLocalAccountThread( void* arg )
{
	CCrsAccount* act = (CCrsAccount*)arg;
//	void* t = AddThread((char*)"Load Local Account Thread");		
	CDataSource db;
	if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
		if( act->GetByBarcode( &db, act->mBarcode ) ){
			act->mLoaded = true;
			if( !act->mWaiting ){
				printf("Found Account Too Late\n");
				delete act;
			}
		}else{
			printf("Couldn't Find Account\n");
		}
		db.DisConnect();
		gConfig.mDatabaseFails = 0;
	} else gConfig.mDatabaseFails++;
//	RemoveThread(t);
	return NULL;	
}

void LoadLocalAccount( char* barcode )
{	
	pthread_t tID;
	gLocalAccount.ClearAll();
	
	CCrsAccount* act = new CCrsAccount;
	act->mWaiting = true;
	act->mLoaded = false;
	strcpy(act->mBarcode,barcode);
	
	UserMessage((char*)"Accessing Account Info...");
	
	int error = pthread_create(&tID, NULL, &LoadLocalAccountThread, (void*)act );
	if( error != 0 ){
		std::cout << "Can't Create Load Local Accont Thread\n";
	}
	
	int t = 300;
	while( t > 0 ){
		usleep(10000);	// 1/100 Second Sleep...		
		if( act->mLoaded ){
			gLocalAccount.Copy(act);
			printf( "Got Data, Showing Account\n" );
			WorkingOff();
			SetScreen(SCREEN_LOCALACT);
			delete act;
			return;
		}
		t--;
	}
	act->mWaiting = false;
	WorkingOff();
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* SaveLocalAccountThread( void* arg )
{
	CDataSource db;
	CCrsAccount* act = (CCrsAccount*)arg;
//	void* t = AddThread((char*)"Save Local Account Thread");		
	
	if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
		act->PutData(&db,act->mDBID);
		db.DisConnect();
		delete act;
		gConfig.mDatabaseFails = 0;
	} else gConfig.mDatabaseFails++;
//	RemoveThread(t);
	return NULL;	
}

void SaveLocalAccount( void )
{	
	pthread_t tID;
	
	CCrsAccount* act = new CCrsAccount;
	act->Copy(&gLocalAccount);
	
	int error = pthread_create(&tID, NULL, &SaveLocalAccountThread, (void*)act );
	if( error != 0 ){
		std::cout << "Can't Create Save Local Accont Thread\n";
	}	
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* CreditLocalAccountThread( void* arg )
{
	CDataSource db;
	CCrsAccount* act = (CCrsAccount*)arg;
//	void* t = AddThread((char*)"Credit Local Account Thread");
	
	if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
		
		if( act->GetByBarcode(&db,act->mBarcode) ){
			act->mCredit += act->mAddCredit;
			act->PutData(&db,act->mDBID);
		}
		
		db.DisConnect();
		delete act;
		gConfig.mDatabaseFails = 0;
	} else gConfig.mDatabaseFails++;
//	RemoveThread(t);
	return NULL;	
}

void CreditLocalAccounts( void )
{
	int i;
	pthread_t tID;
	
	for( i = 0; i < gCart.Count(); i++ ){
		CReceiptItem* pRI = (CReceiptItem*)gCart.GetAt(i);
		if( pRI->mCreditLocalAccount ){
			
			CCrsAccount* act = new CCrsAccount;
			strcpy(act->mBarcode,pRI->mBarcode);
			act->mLastUseDate.Set();
			strcpy(act->mLastUsedKioskName,gConfig.mKioskName);
			act->mAddCredit = pRI->mUnitPrice;

			int error = pthread_create(&tID, NULL, &CreditLocalAccountThread, (void*)act );
			if( error != 0 ){
				std::cout << "Can't Create Credit Local Accont Thread\n";
			}					
		}
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* UploadStockCountThread( void* arg )
{
	CDataSource db;
	CStockCount *pSC = (CStockCount*)arg;
//	void* t = AddThread((char*)"Upload Stock Count Thread");	
	while( true ){
		if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
			pSC->PutData(&db);
			delete pSC;
			db.DisConnect();
//			RemoveThread(t);
			return NULL;
			gConfig.mDatabaseFails = 0;
		} else gConfig.mDatabaseFails++;
		sleep(5);
	}
//	RemoveThread(t);
	return NULL;			
}

void UploadStockCount( CSalesItem* ptr )
{	
	pthread_t tID;
	
	CStockCount *pSC = new CStockCount;	
	pSC->mItemID = ptr->mDBID;
	strcpy(pSC->mBarcode,ptr->mBarcode);
	pSC->mQty = ptr->mInvQty;
	pSC->mInventory = 1;
	ptr->mSoldQty = 0;
	
	strcpy(pSC->mKioskName,gConfig.mKioskName);
	
	int error = pthread_create(&tID, NULL, &UploadStockCountThread, (void*)pSC );
	if( error != 0 ){
		std::cout << "Can't Create Load Local Accont Thread\n";
	}
	UserMessage((char*)"Accessing Account Info...");
}

void UploadTheftCount( CSalesItem* ptr )
{	
	pthread_t tID;
	
	CStockCount *pSC = new CStockCount;	
	pSC->mItemID = ptr->mDBID;
	strcpy(pSC->mBarcode,ptr->mBarcode);
	pSC->mTheft = (ptr->mInvQty-ptr->mSoldQty) - ptr->mCheckQty;
	pSC->mInventory = 0;
	
	strcpy(pSC->mKioskName,gConfig.mKioskName);
	
	int error = pthread_create(&tID, NULL, &UploadStockCountThread, (void*)pSC );
	if( error != 0 ){
		std::cout << "Can't Create Load Local Accont Thread\n";
	}
	UserMessage((char*)"Accessing Account Info...");
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void* LoadStockCountThread( void* arg )
{	
	int i, j;
	char sql[200];
	CDataSource db;
//	void* t = AddThread((char*)"Load Stock Count Thread");
	
	if(db.Connect( gConfig.mDBHost,	gConfig.mDBUser,gConfig.mDBPass, gConfig.mDBName ) ){
		
		CSalesItem *pSI;
		CStockCount *pSC;
		for( j = 0 ; j < gStockCounts.Count(); j++ ){
			CStockCount *pSC = (CStockCount*)gStockCounts.GetAt(j);
			delete pSC;
		}
		gStockCounts.Clear();
		
		CStockCount sc;
		sprintf(sql,"select * from `stockcount` where `kioskname` like '%s' and `inventory` = 1 order by date asc", gConfig.mKioskName );
		sc.GetBatch(&db,sql,&gStockCounts);
		
		for( i = 0; i < gSalesItemList.Count(); i++ ){
			pSI = (CSalesItem*)gSalesItemList.GetAt(i);
			pSI->mInvQty = 0;
			pSI->mSoldQty = 0;
			pSI->mLastUpdate.mValid = false;
		}
		
		for( j = 0 ; j < gStockCounts.Count(); j++ ){
			pSC = (CStockCount*)gStockCounts.GetAt(j);
			for( i = 0; i < gSalesItemList.Count(); i++ ){
				pSI = (CSalesItem*)gSalesItemList.GetAt(i);
				if( pSI->mDBID == pSC->mItemID ){
					if( !pSI->mLastUpdate.mValid || pSC->mDate.mTime > pSI->mLastUpdate.mTime ){
						if( pSC->mInventory == 1  ){
							pSI->mInvQty = pSC->mQty;
							pSI->mLastUpdate.mValid = true;
							pSI->mLastUpdate.mTime = pSC->mDate.mTime;
						}else{
							pSI->mInvQty += pSC->mQty;
						}
					}
				}
			}
		}

		CExDBRecord dbr;
		for( i = 0; i < gSalesItemList.Count(); i++ ){
			pSI = (CSalesItem*)gSalesItemList.GetAt(i);
			if( pSI->mLastUpdate.mValid ){
				sprintf(sql,"select sum(qty) from `receiptitem` where `salesitemid` = '%i' and `kioskname` like '%s' and `receiptdate` >= '%s'", pSI->mDBID, gConfig.mKioskName, pSI->mLastUpdate.Format() );
				dbr.GetSingleInt(&db,(char*)sql,&(pSI->mSoldQty));
			}
		}		
		
		gStockCounts.mLoading = false;
		
		db.DisConnect();
		gConfig.mDatabaseFails = 0;
	} else gConfig.mDatabaseFails++;
//	RemoveThread(t);
	return NULL;			
}

void LoadStockCount( void )
{	
	pthread_t tID;
	gStockCounts.mLoading = true;
	int error = pthread_create(&tID, NULL, &LoadStockCountThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Load Stock Count Thread\n";
	}
	
	int t = 6000;
	WorkingOn((char*)"Loading Stock Counts" );
	while( gStockCounts.mLoading ){
		usleep(10000);	// 1/100 Second Sleep...		
		if( t == 0 ){
			WorkingOff();
			return;
		}
		t--;
	}
	WorkingOff();
}

bool PointInKeyboard(int x, int y, int len )
{
	int i;
	
	if( gDisplay.mBackspace.PointIn( x, y )){
		if( gConfig.mKeyBuffLen > 0 ){
			gConfig.mKeyBuffer[gConfig.mKeyBuffLen-1] = 0;
			gConfig.mKeyBuffLen--;
			gConfig.mRefresh = true;
		}
	}	
	if( gConfig.mKeyBuffLen < len-3 )
	for( i = 0; i < KB_KEYS; i++ ){
		if( gDisplay.mKBRect[i].PointIn( x, y ) ){
			gConfig.mKeyBuffer[gConfig.mKeyBuffLen] = gDisplay.mKBKVal[i];
			gConfig.mKeyBuffLen++;
			gConfig.mKeyBuffer[gConfig.mKeyBuffLen] = 0;
			gConfig.mRefresh = true;
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

bool AdapterExsistsPriv( char* name )
{
	FILE *fp;
	char cmd[100], line[100];
	
	sprintf( cmd, "sudo ifconfig | grep '%s'", name );
	if( fp = popen(cmd,"r") ){
		while( fgets(line,sizeof(line),fp) ){
			if( (strstr( line, name )) > 0 ){
				fclose(fp);
				return true;
			}
		}
		fclose(fp);
	}
	return false;
}

void* CheckNetConnectionThread( void* arg )
{
	FILE* fp;
	bool found;
	char line[400];
	gConfig.mPingFailCount = 0;
	
	while( true ){

		found = false;
		if( fp = popen("ping -c 1 google.com | grep '1 received'","r") ){
			while( fgets(line,sizeof(line),fp) ){
				if( (strstr( line, "1 received" )) > 0 ){
					gConfig.mPingFailCount = 0;
					found = true;
				}else{
					gConfig.mPingFailCount++;
					printf( "ping google.com Returned: %s [%d fails]\n", line, gConfig.mPingFailCount );
				}
			}
			pclose(fp);
		}
		
		if( !found ){
			
			if( AdapterExsistsPriv( (char*)"wlan0" ) ){
				printf( "Trying To Restore Connection With ifdown/ifup on wlan0\n" );
				system( (char*)"sudo /sbin/ifdown wlan0" );
				system( (char*)"sudo /sbin/ifup wlan0" );
			}
			if( AdapterExsistsPriv( (char*)"wlan1" ) ){
				printf( "Trying To Restore Connection With ifdown/ifup on wlan1\n" );
				system( (char*)"sudo /sbin/ifdown wlan1" );
				system( (char*)"sudo /sbin/ifup wlan1" );
			}
		}
		
		if( gConfig.mNetLossReboot == 1 ){
			if( gConfig.mPingFailCount > 20 && gConfig.mDatabaseFails > 2 ){
				
				char temp[100];
				sprintf( temp, "Net Loss Reboot, Ping Fails: %d, Database Fails: %d, Rebooting", gConfig.mPingFailCount, gConfig.mDatabaseFails );
				Log((const char*)temp);
				sleep(1);
				system( (char*)"sudo reboot" );
				
			}
		}
		
		sleep(30);
		gConfig.mCheckIP = 1;
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////

//typedef enum { BK_INIT, BK_READY, BK_CREDIT, BK_END_SESSION, BK_SUCCESS, BK_ERROR } bk_state;
//	char msg[100];
//#define WAIT_TICKS 300

const char* Sts( blukey::EventType e )
{
	switch (e){
		case blukey::E_None:
			return " None";
			break;
		case blukey::E_CreditLoaded:
			return "Credit Load";
			break;
		case blukey::E_SessionEnded:
			return "Session End";
			break;
		case blukey::E_BKCurState:
			return "Cur State";
			break;
		default:
			return "Other";
			break;
	}
}

void* PayRangeThread( void* arg )
{
	blukey::EventType event;
	blukey::EventType lastEvent;
	blukey::EventData data;

	// Try to initalize the blue key...
	bool rc = false;
	rc = blukey::init(10);
	if(!rc){
		printf("Blue Key Init Failed\n");
		return NULL;
	}else{
		printf("Blue Key Init Success\n");
		gConfig.mPayRangeInitalized = true;
	}

	usleep(300000);

	int count = 0;
	int chargeStep = 0;
	bool session = false;

	while(true){

		usleep(300000);		// Loop every 300ms
		
		event = blukey::tick(GetTickCount(), &data);

		// Print state changes with run length...
		if( event == lastEvent ){
			count++;
		}else{
			printf( "Old Key State: %s [%d]\n", Sts(lastEvent), count );
			count = 1;
			printf( "New Key State: %s [%d]\n", Sts(event), count );
			lastEvent = event;
		}

		// Change our internal state to reflect the blue key...
		if( event == blukey::E_CreditLoaded ){
			printf("Blue Key Says Credit Loaded: $%u.%02u \n", data.credit / 100, data.credit % 100);
			gConfig.mPayRangeCredit = data.credit;
			session = true;
		}
		if( event == blukey::E_SessionEnded ){
			printf("Blue Key Says Session Ended\n");
			
			if( chargeStep == 4 ){
				PayRangeMessage(gDT.Get(TXT_THANKS_PURCHASE));
			}
			
			gConfig.mPayRangeCredit = 0;
			session = false;
		}

		// See if the main app wants to end the session...
		if( gConfig.mPayRangeEndSession ){
			printf( "Calling endSession \n" );
			blukey::endSession();
			gConfig.mPayRangeEndSession = false;	
		}
		
		// Actually perform the calls to charge the customer...
		if( chargeStep == 3 ){
			usleep(300000);
			
			PayRangeMessage((char*)"Charging Pay Range Account (3)");

			printf( "Calling endSession \n" );
			blukey::endSession();
			chargeStep = 4;
		}
		if( chargeStep == 2 ){
			usleep(300000);
			
			PayRangeMessage((char*)"Charging Pay Range Account (2)");
			
			printf( "Calling vendCompleted \n" );
			blukey::vendCompleted(blukey::VS_Success);
			chargeStep = 3;
		}
		if( gConfig.mPayRangeCharge ){
			usleep(300000);
			
			PayRangeMessage((char*)"Charging Pay Range Account (1)");
			
			printf( "vendStarted: Charging\n" );
			blukey::vendStarted( 0, gConfig.mPayRangeCharge );
			gConfig.mPayRangeCharge = 0;
			chargeStep = 2;
		}
	}
	
	return NULL;
}

void LogStartup( void )
{
	char temp[100];
	sprintf( temp, "Starting Up Version %s", gConfig.mVersion);
	Log(temp);
}

void LogNameChange( char* newname )
{
	char temp[250];
	sprintf( temp, "User Changed Kiosk Name Old:%s  New:", gConfig.mKioskName, newname);	
	Log(temp);
}

void CheckHeap( void )
{
	printf( "### Point 2 ###\n" );
	system("sudo vcdbg reloc | grep corrupt");
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#define STDIN 0

int main()
{	
	int error;
	int inputptr = 0;
	int keepAlive = 0;
	char inputline[1024];
	memset(inputline,0,1024);
	char track1[100];
		
	gCurrentAd = 0;
	gCurrentLanguage = 0;
	
	LogStartup();
	
	ReadLanguages();
			
	MyICharge 	myICharge;
	printf( "%s\n", myICharge.VERSION() );
	
	printf( "ttyname(3): %s\n", ttyname(0) );
	
	if(mysql_library_init(0, NULL, NULL)) {
		printf( "could not initialize MySQL library\n" );
		return (0);
	}	
	
	// Read Config File ////////////////////////////////////////////////
	if( !gConfig.Read() ){
		std::cout << "Can't Open File Config.txt\n";
		return (0);
	}
	
	// Read Screens File ////////////////////////////////////////////////
	if( !gConfig.ReadScreens() ){
		std::cout << "Can't Open File screens.txt\n";
		return (0);
	}
	
	// Start Touchscreen Thread ////////////////////////////////////////
	TouchScreenBuffer tsb;
	pthread_t TouchScreeInputThreadID;
	error = pthread_create(&TouchScreeInputThreadID, NULL, &TouchScreeInputThread, (void*)&tsb );
	if( error != 0 ){
		std::cout << "Can't Create Touchscreen Thread\n";
		return (0);
	}
	
	// Start Keyboard Thread ///////////////////////////////////////////
	KeyboardBuffer kbb;
	pthread_t KeyboardThredID;
	error = pthread_create(&KeyboardThredID, NULL, &KeyboardInputThread, (void*)&kbb );
	if( error != 0 ){
		std::cout << "Can't Create Keyboard Thread\n";
		return (0);
	}

	// Start Display Thread ///////////////////////////////////////////
	pthread_t displayThredID;
	error = pthread_create(&displayThredID, NULL, &DsiplayThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Display Thread\n";
		return (0);
	}

	////////////////////////////////////////////////////////////////////
	// Find Network Loop ///////////////////////////////////////////////
	if( !GetOurIP() ){
		int timeout = 1000;
		gWiFIList.mLoading = true;
		ScanWifi();
		SetScreen( SCREEN_STARTUP_CHK );
		while( true ){
			
			if( timeout > 0 ){
				timeout--;
			}else{
				if( GetOurIP() ) break;
				timeout = 1000;
			}
						
			if( kbb.CharAvail() ){
				GotInput();
				char c = kbb.GetChr();
			}
			
			if( tsb.TouchAvail() ){
				int i, x, y, scrX, scrY;
				GotInput();
				tsb.GetPoint(&x,&y);
				scrX = (gDisplay.mWidth*y)/tsb.mMaxY;			
				scrY = gDisplay.mHeight - (gDisplay.mHeight*x)/tsb.mMaxX;
			
				if( gConfig.mDisplayScreen == SCREEN_STARTUP_CHK ){
					if( gDisplay.mNetworkInfo.PointIn( scrX, scrY )){
						SetScreen(SCREEN_NETONLY);
					}
					if( gDisplay.mTestPrinter.PointIn( scrX, scrY )){
//						TestPrinter();
					}
					
				}else
				if( gConfig.mDisplayScreen == SCREEN_NETONLY ){
					for( i = 0; i < gWiFIList.Count(); i++ ){
						CWiFiName *ptr = (CWiFiName*)gWiFIList.GetAt(i);
						ptr->mChosen = false;
						if( ptr->mRect.PointIn(scrX,scrY) ){
							ptr->mChosen = true;
							SetScreen( SCREEN_NETPASS );
						}
					}
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						gWiFIList.mLoading = true;
						ScanWifi();
						SetScreen( SCREEN_NETONLY );
					}
					if( gDisplay.mContinue.PointIn( scrX, scrY )){
						SetScreen( SCREEN_STARTUP_CHK );
					}
				}else
				if( gConfig.mDisplayScreen == SCREEN_NETPASS ){
					if( gDisplay.mEnter.PointIn( scrX, scrY )){
						for( i = 0; i < gWiFIList.Count(); i++ ){
							CWiFiName *ptr = (CWiFiName*)gWiFIList.GetAt(i);
							if( ptr->mChosen ){
								strcpy(ptr->mPassword,gConfig.mKeyBuffer);
							}
						}
						ConnectToNetwork();
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_NETONLY );
					}
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						SetScreen( SCREEN_NETONLY );
					}
					PointInKeyboard(scrX,scrY,80);
				}else
				if( gConfig.mDisplayScreen == SCREEN_USERMSG 	||
					gConfig.mDisplayScreen == SCREEN_ERRORSCR	){
					if( gDisplay.mContinue.PointIn( scrX, scrY )){
						SetScreen(SCREEN_NETONLY);
					}
				}
				
			}
						
			usleep(1000);	

		}
	}
	gConfig.mPassedStartupCheck = true;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
		
	// Start Kiosk Info Thread /////////////////////////////////////////
	pthread_t KioskInfoThreadID;
	error = pthread_create(&KioskInfoThreadID, NULL, &KioskInfoThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Kiosk Info Thread\n";
		return (0);
	}
	
	RefreshRequests();

	// Start Database Thread ///////////////////////////////////////////
	pthread_t DatabaseThredID;
	error = pthread_create(&DatabaseThredID, NULL, &DataBaseThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Database Thread\n";
		return (0);
	}
	////////////////////////////////////////////////////////////////////

	// Start Help Video Thread /////////////////////////////////////////
	pthread_t HelpVideoThredID;
	error = pthread_create(&HelpVideoThredID, NULL, &HelpVideoThread, (void*)NULL );
	if( error != 0 ){
		std::cout << "Can't Create Help Video Thread\n";
		return (0);
	}
	////////////////////////////////////////////////////////////////////

	// Start Check Net Connection Thread ///////////////////////////////
	pthread_t CheckNetConnectionThreadID;
	error = pthread_create(&CheckNetConnectionThreadID, NULL, &CheckNetConnectionThread, NULL );
	if( error != 0 ){
		std::cout << "Can't Check Net Connection Thread\n";
		return (0);
	}		
	////////////////////////////////////////////////////////////////////

	// Start Pay Range Thread ///////////////////////////////////////////
	pthread_t PayRangeThreadID;
	error = pthread_create(&PayRangeThreadID, NULL, &PayRangeThread, NULL );
	if( error != 0 ){
		std::cout << "Can't Start Pay Range Thread\n";
		return (0);
	}		
	////////////////////////////////////////////////////////////////////

	Pulse p( (char*) "Main Loop", 1000 );

	SetScreen(SCREEN_WELCOME);
	
	CheckCronTab();
	
	// Main Loop ///////////////////////////////////////////////////////

	while( true ){

		char c;
		int len;
		bool cmdfile;
		
		cmdfile = false;
		// Read Commands From File
		if( access( "./cmd.txt", F_OK ) != -1 ) {
			printf( "Cmd File Found\n" );
			FILE *fp = NULL;
			char *pos;
			if( fp = fopen("./cmd.txt","r") ){
				fgets(inputline,sizeof(inputline),fp);
				if((pos=strchr(inputline, '\n')) != NULL) *pos = '\0';
				printf( "Cmd File Line: %s\n", inputline );
				fclose(fp);
			}
			system("rm ./cmd.txt");
			cmdfile = true;
		}

		// Check For Pay Range Credit //////////////////////////////////
		if( gConfig.mPayRangeCredit ){
			if( gCart.Count() > 0 ){
				gConfig.mPayRangeCharge = CartTotal();
				gConfig.mPayRangeCredit = 0;
				strcpy(gReceipt.mCustomerName,"Pay Range");
				FinishPayRangeSale();
			}else{
				PayRangeMessage(gDT.Get(TXT_CC_SCAN_FIRST));
				gConfig.mPayRangeCredit = 0;
				gConfig.mPayRangeEndSession = true;
			}
		}
		
		if( kbb.CharAvail() || cmdfile ){
			
			GotInput();			
			if(!cmdfile) c = kbb.GetChr();

			// Credit Card Swipe Stuff /////////////////////////////////
			if( 
				c == '?' && inputline[0] == '%' &&
				inputptr < sizeof(inputline) &&
				inputptr < 99
				
				){
				
				inputline[inputptr] = c;
				inputptr++;
				inputline[inputptr] = 0;
				len = strlen(inputline);
				
				printf( "\nCaptured Track 1, Len = %d\n", len );
				strcpy( track1, inputline );				
				inputline[0] = 0;
				inputptr = 0;
				
				if( len > 30 ){
					if( GetOurIP() ){
						if( gCart.Count() > 0 ){
							if( CartSubTotal() < gConfig.mMinCCPurchase ){
								char msg[60];
								sprintf( msg, gDT.Get(TXT_MIN_CC_PURCHASE),
								gConfig.mMinCCPurchase/100, gConfig.mMinCCPurchase%100 );
								ErrorMessage(msg);
								inputline[0] = 0;
								inputptr = 0;
							}else{
								UserMessage( gDT.Get(TXT_AUTH_TRAN), false);
								printf( "Going To charge buff\n" );
								if( ReadyForCard() ){
									if( ChargeCC(track1) ){
										FinishSale();
									}
								}else{
									UserMessage(gDT.Get(TXT_CC_PROC_INFO));
								}
							}
						}else{
							UserMessage(gDT.Get(TXT_CC_SCAN_FIRST));
						}
					}else{
						UserMessage(gDT.Get(TXT_CC_NO_INET));
					}
				}else{			
					UserMessage(gDT.Get(TXT_BAD_CC_READ));
				}
				
				int i;
				for( i = 0; i < sizeof(track1); i++ ) track1[i] = 0;

				
			}else // Other CC Track ... got it all /////////////////////
			if( c == '?' && inputline[0] == ';' && inputptr < sizeof(inputline) ){
				
				printf( "Captured Track 2, Len = %d\n", strlen(inputline) );
				
				inputline[0] = 0;
				inputptr = 0;
			}else
			if( c == 10 || cmdfile ){  // Input Ending With A Return //////////////
				bool done = true;
							
				if( !strcasecmp(inputline,"svc") ){
					if( gConfig.mDisplayScreen == SCREEN_SVCMENU ){
						SetScreen( SCREEN_SCANITEM );
						printf( "Showing Scan Item Screen\n" );
					}else{
						SetScreen( SCREEN_SVCMENU );
						printf( "Showing Service Menu\n" );
					}
				}else
				if( !strcasecmp(inputline,"check") ){
					SetScreen( SCREEN_STARTUP_CHK );
				}else
				if( !strcasecmp(inputline,"screen") ){
					printf( "Screen: %s\n", gDisplay.ScreenName() );
				}else
				if( !strcasecmp(inputline,"quit") ){
					ResetTerminal();
					break;
				}else
				if( !strcasecmp(inputline,"cut") ){
					CutPaper();
				}else
				if( !strcasecmp(inputline,".") ){
					gConfig.mRefresh = true;
				}else
				if( !strcasecmp(inputline,"pstat") ){
					GetPrinterStatus();
				}else
				if( !strcasecmp(inputline,"report") ){
					EmailReport();
				}else
				if( !strcasecmp(inputline,"payrange") ){
					PayRangeMessage((char*)"Pay Range Installed");
				}else
				if( !strcasecmp(inputline,"stat") ){
					GetNetStats();
				}else 
				if( !strcasecmp(inputline,"sale") ){
					FinishSale();
				}else 
				if( !strcasecmp(inputline,"dump") ){
					gDT.DumpToDisk();
				}else 
				if( !strcasecmp(inputline,"pulse") ){
					if( gConfig.mPulse ){
						printf( "Pulse Off\n" );
						gConfig.mPulse = false;
					}else{
						printf( "Pulse On\n" );
						gConfig.mPulse = true;
					}
				}else 

				if( !strcasecmp(inputline,"thumbs") ){
					MakeThumbsThread();
					SetScreen(SCREEN_EDIT_ADS);
				}else
				if( !strcasecmp(inputline,"config") ){
					Display();
				}else
				if( !strcasecmp(inputline,"cap") ){
					gConfig.mCaptureScreen = true;
				}else
				if( !strcasecmp(inputline,"cor") ){
					system("sudo vcdbg reloc | grep corrupt");
				}else
				if( !strcasecmp(inputline,"lg") ){
					system("sudo vcdbg reloc | grep largest");
				}else
				if( !strcasecmp(inputline,"trace") ){
					char temp[100];
					sprintf( temp, "traceroute -p 3306 %s", gConfig.mDBHost );
					printf("############# Starting Trace #############\n%s\n",temp);
					system(temp);
					printf("############# Trace Complete #############\n");
					done = true;
				}else
				if( !strcasecmp(inputline,"help") ){				
					FILE *fp = NULL;
					char line[255];
					if( fp = fopen("./config/help.txt","r") ){
						while( fgets(line,sizeof(line),fp) ) printf( "%s", line );
						fclose(fp);
					}
					done = true;
				}else
				if( !strcasecmp(inputline,"wifi") ){
					ScanWifi();
					done = true;
				}else
				if( !strcasecmp(inputline,"sandbox") ){
					gConfig.mSandBox = !gConfig.mSandBox;
					if(gConfig.mSandBox){
						printf("SandBox: On\n");
					}else{
						printf("SandBox: Off\n");
					}
				}else
				if( !strcasecmp(inputline,"disp") ){
					gConfig.mDisplay = !gConfig.mDisplay;
				}else
				if( !strcasecmp(inputline,"items") ){
					int i;
					int cnt = gSalesItemList.Count();
					printf( "%d Sales Items Loaded\n", cnt );
					for( i = 0; i < cnt; i++ ){
						CSalesItem* ptr = (CSalesItem*)gSalesItemList.GetAt(i);
						printf( "%4d  %-40s $%d.%02d   %-15s %-16s\n", i+1, ptr->mDescription, ptr->mUnitPrice/100, ptr->mUnitPrice%100, ptr->mBarcode, ptr->mTaxType );
					}
					printf( "\n" );
				}else
				if( !strcasecmp(inputline,"vitems") ){
					int i;
					char temp[100];
					int cnt = gSalesItemList.Count();
					for( i = 0; i < cnt; i++ ){
						CSalesItem* ptr = (CSalesItem*)gSalesItemList.GetAt(i);
						if( !strlen(ptr->mBarcode) ){
							sprintf(temp,"%06d.jpg",ptr->mDBID);
							printf( "%4d  %-40s $%d.%02d   %-15s %-16s File: %s", i+1, ptr->mDescription, ptr->mUnitPrice/100, ptr->mUnitPrice%100, ptr->mBarcode, ptr->mTaxType, temp );
							if( access( temp, F_OK ) != -1 ) {
								printf( " (Missing)\n");
							}else{
								printf( " (Found)\n");
							}
						}
					}
					printf( "\n" );
				}else				
				if( !strcasecmp(inputline,"tax") ){
					int i;
					int cnt = gTaxTypes.Count();
					printf( "%d Tax Types Loaded\n", cnt );
					for( i = 0; i < cnt; i++ ){
						CTaxTable* ptr = (CTaxTable*)gTaxTypes.GetAt(i);
						printf( "%4d  %-20s %-20s %d.%02d%%\n", i+1, ptr->mTaxArea, ptr->mTaxType, (int)((ptr->mTaxRate)*10000.0)/100, (int)((ptr->mTaxRate)*10000.0)%100 );
					}
					printf( "\n" );
				}else{
					done = false;					
				}
				
				// Remove tab, cr and nl chrs from barcode...
				char* pos;
				if ((pos=strchr(inputline, '\t')) != NULL) *pos = '\0';
				if ((pos=strchr(inputline, '\r')) != NULL) *pos = '\0';
				if ((pos=strchr(inputline, '\n')) != NULL) *pos = '\0';
				
				// Customer Barcode?
				if( inputline[0] == 'K' && strlen(inputline) == 11 ){
					LoadLocalAccount(inputline);
					done = true;
				}

				// Service Barcode?
				if( gDBText.mServiceBarcode ){
					if( strcmp( gDBText.mServiceBarcode, inputline ) == 0 ){
						SetScreen(SCREEN_SVCMENU);
						done = true;
					}
				}
				
				// Search Sales Items For This Barcode /////////////////
				if( !done && PossibleBarcode(inputline) ){
					CSalesItem* ptr;
					ptr = FindSalesItem(inputline);
					if( ptr == NULL ){
						// Check in database...
						if( FindSIDB(inputline) ){
							ptr = FindSalesItem(inputline);
						}
					}

					if( ptr ){
						if( gConfig.mDisplayScreen == SCREEN_STOCKCNT ||
							gConfig.mDisplayScreen == SCREEN_THEFT_CHECK ){
							gSalesItemPtr = ptr;
							printf( "Stock Count Item: %s\n", ptr->mDescription );
							if( gConfig.mDisplayScreen == SCREEN_STOCKCNT ) SetScreen(SCREEN_ITEMCNT);
							if( gConfig.mDisplayScreen == SCREEN_THEFT_CHECK ) SetScreen(SCREEN_THEFTCNT);
							gConfig.mKeyBuffLen = 0;
							gConfig.mKeyBuffer[0] = 0;
							done = true;
						}else{
							AddToCart(ptr);
							printf( "Scanned Item: %-50s $%d.%02d   %-20s\n", ptr->mDescription, ptr->mUnitPrice/100, ptr->mUnitPrice%100, ptr->mBarcode );
							SetScreen(SCREEN_SCANITEM);
							done = true;
						}
					}					
				}
								
				// Unkown Barcode?
				if( !done && PossibleBarcode(inputline) ){
					UploadUnknownBarcode(inputline);
					char msg[100];
					sprintf( msg, gDT.Get(TXT_UNKNOWN_BARCODE),inputline );
					ErrorMessage(msg);
				}
				
				if( !done ){
					printf( "Unknown: %s\n", inputline );
					if(PossibleBarcode(inputline)) printf("Barcode?\n");
				}
				
				// Input Ended With A Return....
				inputline[0] = 0;
				inputptr = 0;
			}else
			if( inputptr < sizeof(inputline) ){
				inputline[inputptr] = c;
				inputline[inputptr+1] = 0;
				inputptr++;
			}
		}
		
		// Touch Screen Events /////////////////////////////////////////		
		if( tsb.TouchAvail() ){
			int i, x, y, scrX, scrY;
			GotInput();
			tsb.GetPoint(&x,&y);
			scrX = (gDisplay.mWidth*y)/tsb.mMaxY;			
			scrY = gDisplay.mHeight - (gDisplay.mHeight*x)/tsb.mMaxX;

			// Are We Displaying The GUI?
			if( gConfig.mDisplay ){
				// Screen Buttons //////////////////////////////////////////
				if( gConfig.mDisplayScreen == SCREEN_WELCOME ){
					bool button = false;
					for(  i = 0; i < gLanguageCount; i++ ){
						if( gDisplay.mLanguage[i].PointIn( scrX, scrY ) ){
							gCurrentLanguage = i;
							gDT.LoadLanguage(gCurrentLanguage);
							gConfig.mLanguageTimeout = SCREEN_TIMEOUT*3;
							gConfig.mRefresh = true;
							button = true;
							break;
						}
					}
					if( !button ) SetScreen(SCREEN_SCANITEM);
				}else
				if( gConfig.mDisplayScreen == SCREEN_SCANITEM ){
										
					if( gDisplay.mSMenuButton.PointIn( scrX, scrY )){					
						SetScreen(SCREEN_SVCMENU);
					}
					if( gDisplay.mPurchaseButton.PointIn( scrX, scrY )){
						EmailReceipt((char*)"corporaterefreshment@gmail.com");
						FinishSale();
					}
					if( gDisplay.mHelpButton.PointIn( scrX, scrY )){
						gConfig.mScreenTimeout = SCREEN_TIMEOUT*5;
						gConfig.mPlayHelpVideo = true;
					}
					if( gDisplay.mRequestButton.PointIn( scrX, scrY )){
						SetScreen(SCREEN_REQUEST);
					}
					if( gDisplay.mFixButton.PointIn( scrX, scrY )){
						SetScreen(SCREEN_REPPROBL);
					}
					if( gDisplay.mEnterEmailButton.PointIn( scrX, scrY )){
						SetScreen(SCREEN_ENTEMAIL);
					}
					if( gDisplay.mShowRequests.PointIn( scrX, scrY )){
						SetScreen(SCREEN_REQUESTS);
					}
					for( i = 0; i < gCart.Count(); i++ ){
						CReceiptItem* ptr = (CReceiptItem*)gCart.GetAt(i);
						if( ptr->mRect.PointIn( scrX, scrY )){
							if( ptr->mQty == 1 ){
								gCart.RemoveAt(i);
								gConfig.mRefresh = true;
								delete ptr;
								break;
							}else
							if( ptr->mQty > 1 ){
								ptr->mQty--;
								gConfig.mRefresh = true;
							}
						}			
					}
					for( i = 0; i < gSalesItemList.Count(); i++ ){
						CSalesItem* ptr = (CSalesItem*)gSalesItemList.GetAt(i);
						if( ptr->mRect.PointIn( scrX, scrY )){
							AddToCart(ptr);
							printf( "Touched Item: %4d  %-50s $%d.%02d   %-20s\n", i+1, ptr->mDescription, ptr->mUnitPrice/100, ptr->mUnitPrice%100, ptr->mBarcode );
							SetScreen(SCREEN_SCANITEM);
							GotInput();
							break;
						}
					}
					for( i = 0; i < gLanguageCount; i++ ){
						if( gDisplay.mLanguage[i].PointIn( scrX, scrY ) ){
							gCurrentLanguage = i;
							gDT.LoadLanguage(gCurrentLanguage);
							gConfig.mLanguageTimeout = SCREEN_TIMEOUT*3;
							gConfig.mRefresh = true;
							break;
						}
					}
										
				}else
				if( gConfig.mDisplayScreen == SCREEN_CONFIG ){
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						SetScreen( SCREEN_SVCMENU );
					}
				}else
				////////////////////////////////////////////////////////
				if( gConfig.mDisplayScreen == SCREEN_THEFT_CHECK ){
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						SetScreen( SCREEN_SVCMENU );
					}
					for( i = 0; i < gSalesItemList.Count(); i++ ){
						CSalesItem* ptr = (CSalesItem*)gSalesItemList.GetAt(i);
						if( ptr->mRect.PointIn( scrX, scrY )){
							gSalesItemPtr = ptr;
							SetScreen(SCREEN_THEFTCNT);
						}
					}
				}else
				if( gConfig.mDisplayScreen == SCREEN_THEFTCNT ){
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_THEFT_CHECK );
					}
					if( gDisplay.mEnter.PointIn( scrX, scrY )){
						
						gSalesItemPtr->mCheckQty = atoi(gConfig.mKeyBuffer);
						UploadTheftCount(gSalesItemPtr);
						
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_THEFT_CHECK );
					}
					PointInKeyboard(scrX,scrY,7);
					
				}else
				////////////////////////////////////////////////////////
				if( gConfig.mDisplayScreen == SCREEN_STOCKCNT ){
					if( gDisplay.mScrollDown.PointIn( scrX, scrY )){
						gDisplay.mScroll+=60;
						if( gDisplay.mScroll + 60 > gSalesItemList.Count() ) gDisplay.mScroll = gSalesItemList.Count() - 60;
						gConfig.mRefresh = true;
					}
					if( gDisplay.mScrollUp.PointIn( scrX, scrY )){
						gDisplay.mScroll-=60;
						if( gDisplay.mScroll < 0 ) gDisplay.mScroll = 0;
						gConfig.mRefresh = true;
					}
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						SetScreen( SCREEN_SVCMENU );
					}
					for( i = 0; i < gSalesItemList.Count(); i++ ){
						CSalesItem* ptr = (CSalesItem*)gSalesItemList.GetAt(i);
						if( ptr->mRect.PointIn( scrX, scrY )){
							gSalesItemPtr = ptr;
							SetScreen(SCREEN_ITEMCNT);
						}
					}
				}else
				if( gConfig.mDisplayScreen == SCREEN_ITEMCNT ){
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_STOCKCNT );
					}
					if( gDisplay.mEnter.PointIn( scrX, scrY )){
						
						gSalesItemPtr->mInvQty = atoi(gConfig.mKeyBuffer);
						UploadStockCount(gSalesItemPtr);
						
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_STOCKCNT );
					}
					PointInKeyboard(scrX,scrY,7);
					
				}else
				if( gConfig.mDisplayScreen == SCREEN_NETWORK ){
					for( i = 0; i < gWiFIList.Count(); i++ ){
						CWiFiName *ptr = (CWiFiName*)gWiFIList.GetAt(i);
						ptr->mChosen = false;
						if( ptr->mRect.PointIn(scrX,scrY) ){
							ptr->mChosen = true;
							SetScreen( SCREEN_NETPASS );
						}
					}
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						SetScreen( SCREEN_SVCMENU );
					}
				}else
				if( gConfig.mDisplayScreen == SCREEN_ENTER_KNAME ){
					if( gDisplay.mEnter.PointIn( scrX, scrY )){
						LogNameChange(gConfig.mKeyBuffer);
						strcpy(gConfig.mKioskName,gConfig.mKeyBuffer);
						gConfig.Write();
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_SVCMENU );
					}
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_SVCMENU );
					}
					PointInKeyboard(scrX,scrY,80);
				}else
				if( gConfig.mDisplayScreen == SCREEN_NETPASS ){
					if( gDisplay.mEnter.PointIn( scrX, scrY )){
						for( i = 0; i < gWiFIList.Count(); i++ ){
							CWiFiName *ptr = (CWiFiName*)gWiFIList.GetAt(i);
							if( ptr->mChosen ){
								strcpy(ptr->mPassword,gConfig.mKeyBuffer);
							}
						}
						ConnectToNetwork();
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_SVCMENU );
					}
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_SVCMENU );
					}
					PointInKeyboard(scrX,scrY,80);
				}else
				if( gConfig.mDisplayScreen == SCREEN_SVCMENU ){
					if( gDisplay.mSetAsServiced.PointIn( scrX, scrY )){
						SetServiceDate();
					}
					if( gDisplay.mNetworkInfo.PointIn( scrX, scrY )){
						gWiFIList.mLoading = true;
						ScanWifi();
						SetScreen( SCREEN_NETWORK );
					}
					if( gDisplay.mDispConfig.PointIn( scrX, scrY )){
						SetScreen( SCREEN_CONFIG );
					}
					if( gDisplay.mStockCount.PointIn( scrX, scrY )){
						LoadStockCount();
						SetScreen(SCREEN_STOCKCNT);
					}
					if( gDisplay.mTheftCount.PointIn( scrX, scrY )){
						LoadStockCount();
						SetScreen(SCREEN_THEFT_CHECK);
					}
					if( gDisplay.mEditAds.PointIn( scrX, scrY )){
						SetScreen(SCREEN_EDIT_ADS);
					}
					if( gDisplay.mSetKName.PointIn( scrX, scrY )){
						SetScreen( SCREEN_ENTER_KNAME );
					}
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						SetScreen( SCREEN_SCANITEM );
					}
					if( gDisplay.mExitGUI.PointIn( scrX, scrY )){
						gConfig.mDisplay = false;
					}
					if( gDisplay.mExitApp.PointIn( scrX, scrY )){
						ResetTerminal();
						printf( "User Selected Quit Apliation Button\n" );
						return 0;
					}
				}else
				if( gConfig.mDisplayScreen == SCREEN_WANTPRNT ){
					if( gDisplay.mContinue.PointIn( scrX, scrY )){
							PrintReceipt();
							gCart.Clear();
							gReceipt.ClearAll();
							SetScreen(SCREEN_SCANITEM);
						}
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						gCart.Clear();
						gReceipt.ClearAll();
//						DeleteCart();
//						gReceipt.mEMail[0] = 0;
						SetScreen( SCREEN_SCANITEM );
					}
				}else
				if( gConfig.mDisplayScreen == SCREEN_STLTHERE ){
					if( gDisplay.mContinue.PointIn( scrX, scrY )){ SetScreen(SCREEN_SCANITEM); }
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						DeleteCart();
						gReceipt.mEMail[0] = 0;
						SetScreen( SCREEN_WELCOME );
					}
				}else
				if( gConfig.mDisplayScreen == SCREEN_USERMSG 	||
					gConfig.mDisplayScreen == SCREEN_ERRORSCR	||
					gConfig.mDisplayScreen == SCREEN_PAY_RANGE){
					if( gDisplay.mContinue.PointIn( scrX, scrY )){
						SetScreen(SCREEN_SCANITEM);
					}
				}else
				if( gConfig.mDisplayScreen == SCREEN_REQUESTS ){
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						SetScreen( SCREEN_SCANITEM );
					}
				}else
				if( gConfig.mDisplayScreen == SCREEN_LOCALACT ){
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						gLocalAccount.ClearAll();
						SetScreen( SCREEN_SCANITEM );
					}
					if( gDisplay.mAddToAccount.PointIn( scrX, scrY )){
						SetScreen( SCREEN_ADDACCNT );
					}
					if( gDisplay.mPurchaseOnAccount.PointIn( scrX, scrY )){
						if( ChargeLocal() ){
							FinishSale();
						}
					}
				}else
				if(	gConfig.mDisplayScreen == SCREEN_REQUEST	||
					gConfig.mDisplayScreen == SCREEN_REPPROBL	||
					gConfig.mDisplayScreen == SCREEN_ENTEMAIL	||
					gConfig.mDisplayScreen == SCREEN_ADDACCNT	){
					
					if( gDisplay.mAddToAccount.PointIn( scrX, scrY )){						
						CReceiptItem* ri = new CReceiptItem;
						gCart.Add(ri);
						ri->mCreditLocalAccount = atoi(gConfig.mKeyBuffer);
						strcpy(ri->mBarcode,gLocalAccount.mBarcode);
						sprintf( ri->mDescription,"Credit to %s", gLocalAccount.mBarcode );
						ri->mQty = 1;
						ri->mCreditLocalAccount = 1;
						ri->mUnitPrice = atoi(gConfig.mKeyBuffer);
						
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						SetScreen( SCREEN_SCANITEM );
					}
					if( gDisplay.mCancel.PointIn( scrX, scrY )){
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						if( gConfig.mDisplayScreen == SCREEN_ADDACCNT ) gLocalAccount.ClearAll();
						SetScreen( SCREEN_SCANITEM );
					}
					if( gDisplay.mSkip.PointIn( scrX, scrY )){
						gConfig.mKeyBuffLen = 0;
						gConfig.mKeyBuffer[0] = 0;
						gReceipt.mEMail[0] = 0;
						SetScreen( SCREEN_SCANITEM );
					}
					if( gDisplay.mEnter.PointIn( scrX, scrY )){
						if( gConfig.mDisplayScreen == SCREEN_REQUEST	||
							gConfig.mDisplayScreen == SCREEN_REPPROBL ){
							UploadRequest(gConfig.mKeyBuffer);
							gConfig.mKeyBuffLen = 0;
							gConfig.mKeyBuffer[0] = 0;
							SetScreen( SCREEN_SCANITEM );
							UserMessage((char*)"Thank You For The Feedback");
						}
						if( gConfig.mDisplayScreen == SCREEN_ENTEMAIL ){
							if( gConfig.mKeyBuffLen > 49 ){
								gConfig.mKeyBuffer[49] = 0;
							}
							strcpy(gReceipt.mEMail,gConfig.mKeyBuffer);
							gConfig.mKeyBuffLen = 0;
							gConfig.mKeyBuffer[0] = 0;
							SetScreen( SCREEN_SCANITEM );
						}
					}
					
					PointInKeyboard(scrX,scrY,255);
					
				}
			}else{
				if( !(gConfig.mPulse) ){
					gConfig.mDisplay = true;
					gConfig.mRefresh = true;
				}
			}			
		}
						
		if( gConfig.mRefreshRequests ){
			RefreshRequests();
		}
		
		if( gConfig.mPulse ) p.Ping();
		
		usleep(1000);
	}

	return (0);
}

