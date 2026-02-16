Timer namespace that can be used to measure execution time in scope.

### Example Usage
Measuring
```cpp
{Measure("Timer name") // Timer starts
	// ...
} // Timer stops
```
Printing 
```cpp
std::cout << Timer::string() << std::endl;
std::cout << Timer::string<std::chrono::microseconds, Timer::Sort, Timer::Percentage, Timer::Align>() << std::endl;
```
Example output
```
Outer Timer: 403ms
| First Loop: 57ms
| Second Loop: 287ms
| | Inside Second Loop 1: 58ms
| | Inside Second Loop 2: 172ms
| | | Inside Inside Second Loop 2: 116ms
| Third Loop: 58ms

Outer Timer:                           403084us             100%
| Second Loop:                         287175us             71.2446%
| | Inside Second Loop 2:              172330us             60.0088%
| | | Inside Inside Second Loop 2:     116392us             67.5402%
| | Inside Second Loop 1:               58009us             20.2%
| Third Loop:                           58310us             14.4662%
| First Loop:                           57564us             14.2811%
```
For more options see `example.cpp` that can be compiled with `make example`.
