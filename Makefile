
CC=clang
#CXX=clang++
CXX=g++

#PROFILER=-pg
DEBUG=-g3 -ggdb3

SOURCES=graph.cpp clusters.cpp gtk_fragmap.cpp filelistview.cpp fragdb.cpp
OBJECTS_common=clusters.o
OBJECTS_graph=graph.o gtk_fragmap.o filelistview.o
OBJECTS_fragdb=fragdb.o


CFLAGS=$(DEBUG) -O2 $(shell pkg-config --cflags gtk+-2.0) $(PROFILER)
CFLAGS+=$(shell pkg-config --cflags sqlite3)
LIBS_graph=$(shell pkg-config --libs gtk+-2.0)
LIBS_fragdb=$(shell pkg-config --libs sqlite3)

all: graph fragdb

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

fragdb: $(OBJECTS_common) $(OBJECTS_fragdb)
	$(CXX) -o fragdb $(OBJECTS_common) $(OBJECTS_fragdb) $(LIBS_fragdb) $(PROFILER)

graph: $(OBJECTS_common) $(OBJECTS_graph)
	$(CXX)  -o graph $(OBJECTS_common) $(OBJECTS_graph) $(LIBS_graph) $(PROFILER)

clean:
	rm -f *.o
	rm -f graph fragdb gmon.out
