TARGET = GageCsPrf
CC = gcc

all: $(TARGET)

# where the include files are
INCLUDE = -I../../../Include -I../../CsAppSupport -I../../C_Common -I../../../Include/Public

# options for development
CFLAGS = -g -Wall

#options for release
#CFLAGS = -O -Wall

LDFLAGS = -lCsSsm -lCsAppSupport -lpthread

$(TARGET): GageCsPrf.o CsSdkMisc.o CsSdkUtil.o
	$(CC) $^ $(LDFLAGS) -o $@

GageCsPrf.o: GageCsPrf.c
	$(CC) $(INCLUDE) $(CFLAGS) -c $<

CsSdkMisc.o: ../../C_Common/CsSdkMisc.c
	$(CC) $(INCLUDE) $(CFLAGS) -c "$<"

CsSdkUtil.o: ../../C_Common/CsSdkUtil.c
	$(CC) $(INCLUDE) $(CFLAGS) -c "$<"

.PHONY : all clean install uninstall

clean:
	rm -rf *.o $(TARGET)

install: all
#	cp $(TARGET) ../../../Bin

uninstall: clean
#	rm -rf ../../../Bin/$(TARGET)
