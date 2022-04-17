CC=g++
CFLAGS=-I
CFLAGS+=-Wall
CFLAGS+=-std=c++17
FILES=Logger.cpp
FILES+=Automobile.cpp
FILES+=TravelSimulator.cpp
FILES1=LogServer.cpp
LIBS=-lpthread

travel: $(FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

server: $(FILES1)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f *.o travel server
	
all: travel server
