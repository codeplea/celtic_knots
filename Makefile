CC=g++
CFLAGS=-Wall -O2

saver:
	$(CC) $(CFLAGS) -o celtic_knots.scr saver.cpp cknot.cpp -mwindows -lopengl32 -lscrnsave

