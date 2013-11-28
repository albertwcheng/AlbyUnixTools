all: joina

joina: join_main.cpp AdvGetOptCpp/AdvGetOpt.cpp
	g++ -O3 -std=c++0x -o joina join_main.cpp AdvGetOptCpp/AdvGetOpt.cpp
	
clean:
	rm -rf *.o joina