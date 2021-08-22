TARGET = GageEvents
CC = gcc

all: $(TARGET)

# where the include files are
INCLUDE = -I../../../Include -I../../CsAppSupport -I../../C_Common -I../../../Include/Public

# options for development
CFLAGS = -g -Wall

#options for release
#CFLAGS = -O -Wall

LDFLAGS = -lCsSsm -lCsAppSupport -lpthread

$(TARGET): Events.o Threads.o CsSdkMisc.o
	$(CC) $^ $(LDFLAGS) -o $@

Events.o: Events.c
	$(CC) $(INCLUDE) $(CFLAGS) -c $<

Threads.o: Threads.c
	$(CC) $(INCLUDE) $(CFLAGS) -c $<

CsSdkMisc.o: ../../C_Common/CsSdkMisc.c
	$(CC) $(INCLUDE) $(CFLAGS) -c "$<"

.PHONY : all clean install uninstall

clean:
	rm -rf *.o $(TARGET)

install: all
#	cp $(TARGET) ../../../Bin

uninstall: clean
#	rm -rf ../../../Bin/$(TARGET)
