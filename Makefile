CXX = x86_64-w64-mingw32-g++
CC = x86_64-w64-mingw32-gcc
CXXFLAGS = -Wall -O2
LDFLAGS = -static -mwindows
LDLIBS = -lopengl32 -lglu32 -lcomctl32

celtic_knots.scr: saver.cpp cknot.cpp cknot.hpp spline.hpp scrnsave.o
	$(CXX) $(CXXFLAGS) -o $@ saver.cpp cknot.cpp scrnsave.o $(LDFLAGS) $(LDLIBS)

scrnsave.o: third_party/scrnsave.c
	$(CC) -O2 -c -o $@ $<

clean:
	rm -f celtic_knots.scr scrnsave.o
