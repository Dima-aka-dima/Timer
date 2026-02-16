#include <iostream>

#include <chrono>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm> // std::sort
#include <numeric>   // std::accumulate
#include <iomanip>   // std::setw
#include <typeindex> // std::type_index

namespace Timer
{
	using clock = std::chrono::high_resolution_clock;

	std::unordered_map<std::type_index, std::string> units = 
	{
		{typeid(std::chrono::nanoseconds),  "ns" }, {typeid(std::chrono::microseconds), "us"},
		{typeid(std::chrono::milliseconds), "ms" }, {typeid(std::chrono::seconds),      "s" },
		{typeid(std::chrono::minutes),      "min"}, {typeid(std::chrono::hours),        "h" }
	};

	// Tree structure for timers
	struct Timer
	{
		std::string name;
		clock::duration time;
		size_t depth = 0;

		Timer* parent = nullptr;
		std::vector<Timer*> children;

		Timer() {};
		Timer(Timer* p, std::string s) : name(s), depth(p->depth + 1), parent(p) {};
	};
	
	Timer* tree = new Timer();
	Timer* timer = tree;

	// Main measurement class. Construction (destruction)
	// corresponds to starting (stopping) the measuremsent
	#define Measure __Measure _measurement
	class __Measure
	{
		std::chrono::time_point<clock> start;

	public:	
		
		__Measure(std::string s = "")
		{
			timer->children.push_back(new Timer(timer, s));
			timer = timer->children.back();

			start = clock::now();
		}

		~__Measure()
		{
			auto duration = clock::now() - start;
			
			timer->time = duration;
			timer = timer->parent;
		}
	};

	void sort(Timer* timer)
	{
		std::sort(timer->children.begin(), timer->children.end(), [](const Timer* a, const Timer* b) { return a->time > b->time; });
		for(auto child: timer->children) sort(child);
	}

	// Print options
	const uint64_t Sort 	  = 0b001; // Sort by time
	const uint64_t Percentage = 0b010; // Display percentage of outer timer
	const uint64_t Align      = 0b100; // Align as columns
	
	constexpr size_t maxNameLength = 45;
	constexpr size_t maxTimeLength = 15;

	// Converts one measurement to string 
	template<typename time_t>
	std::string __string(Timer* timer, uint64_t flags)
	{
		std::stringstream stream;
		
		if(timer != tree)
		{
			for(size_t i = 0; i < timer->depth - 1; i++) stream << "| ";
			stream << timer->name + ": ";

			if (flags & Align) stream << std::right << std::setw(maxNameLength - stream.tellp());
			stream << std::chrono::duration_cast<time_t>(timer->time).count() << units[typeid(time_t)];
			
			double percentage = 100.0 * timer->time / timer->parent->time;
			if (flags & Align) stream << std::left << std::setw(maxNameLength + maxTimeLength - stream.tellp());
			if (flags & Percentage) stream << " " << percentage << "%";	
			stream << std::endl;
		}

		for(auto child: timer->children) stream << __string<time_t>(child, flags);
		
		return stream.str();
	}
	
	// Main function that converts measurements to string
	template<typename time_t = std::chrono::milliseconds>
	std::string string(uint64_t flags = 0)
	{
		if (flags & Sort) sort(tree); 
		
		if (flags & Percentage)
			tree->time = std::accumulate(tree->children.begin(), tree->children.end(), clock::duration::zero(), 
					[](const auto& acc, const auto& child){ return acc + child->time; });
		return __string<time_t>(tree, flags);
	}
}

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
				loop(10);
			}
		}
	}
	{
		Timer::Measure("Third Loop");
		loop();
		loop();
	}
	}
	
	std::cout << Timer::string() << std::endl;
	std::cout << Timer::string(Timer::Align | Timer::Percentage) << std::endl;
	std::cout << Timer::string<std::chrono::microseconds>(Timer::Sort | Timer::Align | Timer::Percentage) << std::endl;

}
