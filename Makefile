
CC=clang
#CXX=clang++
CXX=g++

#PROFILER=-pg
DEBUG=-g3 -ggdb3

SOURCES=graph.cpp clusters.cpp gtk_fragmap.cpp filelistview.cpp
OBJECTS=$(SOURCES:.cpp=.o)


CFLAGS=$(DEBUG) -O2 $(shell pkg-config --cflags gtk+-2.0) $(PROFILER)
LIBS=$(shell pkg-config --libs gtk+-2.0) $(PROFILER)

all: graph

#clusters.o: clusters.cpp
#	$(CXX) $(CFLAGS) -c clusters.cpp

#graph.o: graph.cpp
#	$(CXX) $(CFLAGS) -c graph.cpp

#gtk_fragmap.o: gtk_fragmap.cpp
#	$(CXX) $(CFLAGS) -c gtk_fragmap.cpp

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<



graph: $(OBJECTS)
	$(CXX)  -o graph $(OBJECTS) $(LIBS)

clean:
	rm -f *.o
	rm -f graph gmon.out
