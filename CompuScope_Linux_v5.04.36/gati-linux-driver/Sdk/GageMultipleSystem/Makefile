all: GageMultipleSystem

CC = gcc

# where the include files are
INCLUDE = -I../../Include -I../CsAppSupport -I../C_Common -I../../Include/Linux -I../../Include/Public -I../../Include/Private


# options for development
CFLAGS = -g -Wall

#options for release
#CFLAGS = -O -Wall

LDFLAGS = -lCsSsm -lCsAppSupport -lpthread

GageMultipleSystem: MultipleSystem.o CsSdkMisc.o
	$(CC) MultipleSystem.o CsSdkMisc.o $(LDFLAGS) -o GageMultipleSystem

MultipleSystem.o: MultipleSystem.c
	$(CC) $(INCLUDE) $(CFLAGS) -c MultipleSystem.c

CsSdkMisc.o: ../C_Common/CsSdkMisc.c
	$(CC) $(INCLUDE) $(CFLAGS) -c ../C_Common/CsSdkMisc.c

.PHONY : clean install uninstall

clean:
	rm -rf *.o GageMultipleSystem

install: all
#	cp GageMultipleSystem ../../Bin

uninstall: clean
#	rm -rf ../../Bin/GageMultipleSystem


