#include <iostream>

#include "timer.hpp" 

void loop(size_t n = 20000000)
{
	volatile int sum = 0;
	for(size_t i = 0; i < n; i++) sum += i*i;
};

int main()
{
	{Timer::Measure("Outer Timer");
	{
		Timer::Measure("First Loop");
		loop();
		// Timer::Measure("First Loop part 2"); // Will not compile, one timer per scope!
	}

	{
		Timer::Measure("Second Loop");	
		loop();

		{
			Timer::Measure("Inside Second Loop 1");
			loop();
		}

		{
			Timer::Measure("Inside Second Loop 2");
			loop();
			{
				Timer::Measure("Inside Inside Second Loop 2");
				loop(); loop();
			}
		}
	}
	{
		Timer::Measure("Third Loop");
		loop();
	}
	}

	std::cout << Timer::string<>() << std::endl; // Standard way to print
	std::cout << Timer::string<Timer::Align>() << std::endl; // Aligns columns
	std::cout << Timer::string<Timer::Sort, Timer::Align>() << std::endl; // Sorts by time
	std::cout << Timer::string<std::chrono::microseconds, Timer::Percentage, Timer::Align>() << std::endl; // Will still be sorted
}
