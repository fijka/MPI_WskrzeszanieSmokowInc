SOURCES=$(wildcard *.cpp)
HEADERS=$(SOURCES:.cpp=.h)
FLAGS=-DDEBUG -g -pthread

all: main

main: $(SOURCES) $(HEADERS)
	mpiCC $(SOURCES) $(FLAGS) -o main 

clear: clean

clean:
	rm main a.out

run: main
	mpirun -np 4 ./main
