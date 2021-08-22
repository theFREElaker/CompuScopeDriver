all: GageComplexTrigger

CC = gcc

# where the include files are
INCLUDE = -I../../Include -I../CsAppSupport -I../C_Common -I../../Include/Public


# options for development
CFLAGS = -g -Wall

#options for release
#CFLAGS = -O -Wall

LDFLAGS = -lCsSsm -lCsAppSupport

GageComplexTrigger: ComplexTrigger.o CsSdkMisc.o
	$(CC) ComplexTrigger.o CsSdkMisc.o $(LDFLAGS) -o GageComplexTrigger

ComplexTrigger.o: ComplexTrigger.c
	$(CC) $(INCLUDE) $(CFLAGS) -c ComplexTrigger.c

CsSdkMisc.o: ../C_Common/CsSdkMisc.c
	$(CC) $(INCLUDE) $(CFLAGS) -c ../C_Common/CsSdkMisc.c

.PHONY : clean install uninstall

clean:
	rm -rf *.o GageComplexTrigger

install: all
#	cp GageComplexTrigger ../../Bin

uninstall: clean
#	rm -rf ../../Bin/GageComplexTrigger


