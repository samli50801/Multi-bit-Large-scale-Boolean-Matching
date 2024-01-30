# sample Makefile
RES = ./src/main.cpp
EXE = ./bin/bmatch
all:
	g++ -std=c++11 -Ofast $(RES) -o $(EXE) cadical-master/build/libcadical.a libabc.a -lm -ldl -lreadline -lpthread
clean:
	rm $(EXE)
