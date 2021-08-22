all: GageMultipleRecord

CC = gcc

# where the include files are
INCLUDE = -I../../Include -I../CsAppSupport -I../C_Common -I../../Include/Public


# options for development
CFLAGS = -g -Wall

#options for release
#CFLAGS = -O -Wall

LDFLAGS = -lCsSsm -lCsAppSupport

GageMultipleRecord: MultipleRecord.o CsSdkMisc.o
	$(CC) MultipleRecord.o CsSdkMisc.o $(LDFLAGS) -o GageMultipleRecord

MultipleRecord.o: MultipleRecord.c
	$(CC) $(INCLUDE) $(CFLAGS) -c MultipleRecord.c

CsSdkMisc.o: ../C_Common/CsSdkMisc.c
	$(CC) $(INCLUDE) $(CFLAGS) -c ../C_Common/CsSdkMisc.c

.PHONY : clean install uninstall

clean:
	rm -rf *.o GageMultipleRecord

install: all
#	cp GageMultipleRecord ../../Bin

uninstall: clean
#	rm -rf ../../Bin/GageMultipleRecord

