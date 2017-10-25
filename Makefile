
INCLUDEFLAGS=-I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..

LIBFLAGS= -L/opt/vc/lib -lGLESv2 -lEGL -lbcm_host -lpthread -ljpeg -lz -lresolv -lpigpio

main: main.cpp ./gl/libshapes.o ./gl/oglinit.o 
		g++ $(INCLUDEFLAGS) $(LIBFLAGS) -o reboundviewer main.cpp crect.cpp cdisplay.cpp mcp3008spi.cpp ./gl/libshapes.o ./gl/oglinit.o


		
