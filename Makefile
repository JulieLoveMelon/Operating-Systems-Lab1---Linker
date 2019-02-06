linker:linker.o
	g++ linker.o -o linker

linker.o:Linker.cpp
	g++ -std=c++11 Linker.cpp -c -o linker.o

clean:
	rm linker
	rm *.o
