main: main.cpp
	g++ -std=c++14 -Wall -Wextra $< -o $@

cpp17: main_cpp17.cpp
	g++ -std=c++17 -Wall -Wextra $< -o main_cpp17 
