CXX    ?= clang++
CFLAGS  = -Wall -std=c++11
LIBS    = `pkg-config --cflags --libs gtkmm-3.0`
LIBS   += `pkg-config --cflags --libs cairomm-1.0`
SOURCE  = $(SRC)*.cpp
BINARY  = jpgd

all:
	$(CXX) $(SOURCE) $(LIBS) $(CFLAGS) -o $(BINARY) -O3
debug:
	$(CXX) $(SOURCE) $(LIBS) $(CFLAGS) -o $(BINARY) -DDEBUG -g
clean:
	rm -f $(BINARY)
	rm -f *.o

