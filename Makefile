all: joina cljoina

joina: join_main.cpp AdvGetOptCpp/AdvGetOpt.cpp
	g++ -O3 -std=c++0x -o joina join_main.cpp AdvGetOptCpp/AdvGetOpt.cpp

cljoina: cljoin_main.cpp AdvGetOptCpp/AdvGetOpt.cpp
	g++ -O3 -std=c++0x -o cljoina cljoin_main.cpp AdvGetOptCpp/AdvGetOpt.cpp

clean:
	rm -rf *.o joina cljoina