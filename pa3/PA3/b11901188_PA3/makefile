routing.o:routing.cpp
	g++ -std=c++11 -O3 -c routing.cpp
pa3.o:pa3.cpp
	g++ -std=c++11 -O3 -c pa3.cpp
pa3: pa3.o routing.o
	g++ -std=c++11 -O3 -o pa3 routing.o pa3.o
	
clean:
	rm -f F_M.o pa3.o routing.o pa3
	
