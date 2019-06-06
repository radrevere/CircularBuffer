CC=g++
CFLAGS=-I -Wall -W -Wextra -Wpedantic -std=c++14
DEPS = packetdefs.h logger.h
OBJ = BufferTest.o SingleBlockPool.o CircularBuffer.o

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

BufferTest: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
