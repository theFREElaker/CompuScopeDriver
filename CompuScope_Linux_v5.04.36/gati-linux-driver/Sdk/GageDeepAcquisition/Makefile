all: GageDeepAcquisition

CC = gcc

# where the include files are
INCLUDE = -I../../Include -I../CsAppSupport -I../C_Common -I../../Include/Public


# options for development
CFLAGS = -g -Wall

#options for release
#CFLAGS = -O -Wall

LDFLAGS = -lCsSsm -lCsAppSupport

GageDeepAcquisition: DeepAcquisition.o CsSdkMisc.o
	$(CC) DeepAcquisition.o CsSdkMisc.o $(LDFLAGS) -o GageDeepAcquisition

DeepAcquisition.o: DeepAcquisition.c
	$(CC) $(INCLUDE) $(CFLAGS) -c DeepAcquisition.c

CsSdkMisc.o: ../C_Common/CsSdkMisc.c
	$(CC) $(INCLUDE) $(CFLAGS) -c ../C_Common/CsSdkMisc.c

.PHONY : clean install uninstall

clean:
	rm -rf *.o GageDeepAcquisition

install: all
#	cp GageDeepAcquisition ../../Bin

uninstall: clean
#	rm -rf ../../Bin/GageDeepAcquisition

