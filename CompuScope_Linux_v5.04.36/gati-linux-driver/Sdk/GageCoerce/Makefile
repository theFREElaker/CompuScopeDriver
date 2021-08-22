all: GageCoerce

CC = gcc

# where the include files are
INCLUDE = -I../../Include -I../CsAppSupport -I../C_Common -I../../Include/Public


# options for development
CFLAGS = -g -Wall

#options for release
#CFLAGS = -O -Wall

LDFLAGS = -lCsSsm -lCsAppSupport

GageCoerce: Coerce.o CsSdkMisc.o
	$(CC) Coerce.o CsSdkMisc.o $(LDFLAGS) -o GageCoerce

Coerce.o: Coerce.c
	$(CC) $(INCLUDE) $(CFLAGS) -c Coerce.c

CsSdkMisc.o: ../C_Common/CsSdkMisc.c
	$(CC) $(INCLUDE) $(CFLAGS) -c ../C_Common/CsSdkMisc.c

.PHONY : clean install uninstall

clean:
	rm -rf *.o GageCoerce

install: all
#	cp GageCoerce ../../Bin

uninstall: clean
#	rm -rf ../../Bin/GageCoerce

