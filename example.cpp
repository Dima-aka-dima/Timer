#include <iostream>

#include "timer.hpp" 

void loop(size_t n = 20000000)
{
	volatile int sum = 0;
	for(size_t i = 0; i < n; i++) sum += i*i;
};

int main()
{
	Timer::Start("Outer Timer");
	{
		Timer::Scope("First Loop");
		loop();
		// Timer::Scope("First Loop part 2"); // Will not compile, one timer per scope!

		Timer::Start("First loop 2");
		loop();loop();
		Timer::Stop();
	}


	Timer::Start("Second Loop");	
	loop();

	{
		Timer::Scope("Inside Second Loop 1");
		loop();
	}

	Timer::Start("Inside Second Loop 2");
	loop();
	{
		Timer::Scope("Inside Inside Second Loop 2");
		loop(); loop();
	}
	
	Timer::Stop();
	Timer::Stop();
	
	{
		Timer::Scope("Third Loop");
		loop();
	}
	Timer::Stop();
	
	Timer::Start("Repeating timers");
	for(size_t i = 0; i < 20; i++)
	{
		Timer::Start("Timer in loop, repeating");
		loop(2000); 
		Timer::Stop();

		Timer::Start("Second timer in loop, repeating");
		loop(4000);
		Timer::Stop();
	}
	Timer::Stop();

	std::cout << Timer::string() << std::endl; // Standard way to print
	std::cout << Timer::string<Timer::Align>() << std::endl; // Aligns columns
	std::cout << Timer::string<Timer::Sort, Timer::Align>() << std::endl; // Sorts by time
	std::cout << Timer::string<std::chrono::microseconds, Timer::Percentage, Timer::Align, Timer::Color>() << std::endl; // Will still be sorted
}
