CXX = g++
CFLAGS = -g3 -ggdb3 -O0

all: severity_test1
	make -C ..

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

severity_test1: severity_test1.o ../clusters.o
	$(CXX) -o severity_test1 severity_test1.o ../clusters.o

clean:
	rm *.o
	rm severity_test1

