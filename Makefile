
example: example.cpp timer.hpp
	g++ -std=c++17 -Wall -Wextra --pedantic $< -o $@ 

clean:
	rm -f example main_iterator main_cpp14 main_cpp17

iterator: versions/main_iterator.cpp
	g++ -std=c++17 -Wall -Wextra $< -o main_iterator

cpp14: versions/main_cpp14.cpp
	g++ -std=c++14 -Wall -Wextra $< -o main_cpp14

cpp17: versions/main_cpp17.cpp
	g++ -std=c++17 -Wall -Wextra $< -o main_cpp17 
