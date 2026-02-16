main: main.cpp
	g++ -std=c++17 -Wall -Wextra $< -o main

iterator: main_iterator.cpp
	g++ -std=c++17 -Wall -Wextra $< -o main_iterator

cpp14: main_cpp14.cpp
	g++ -std=c++14 -Wall -Wextra $< -o main_cpp14

cpp17: main_cpp17.cpp
	g++ -std=c++17 -Wall -Wextra $< -o main_cpp17 
