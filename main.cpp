#include <iostream>

#include <chrono>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm> // std::sort
#include <numeric>   // std::accumulate
#include <iomanip>   // std::setw

namespace Timer
{
	// Type and other shenanigans	
	#define isOption(Option, ...) constexpr ((std::is_same_v<Option, __VA_ARGS__> || ...))	
	
	template<typename Period> struct __units    { static constexpr const char* value = "?";  };
	template<> struct __units<std::ratio<1>>    { static constexpr const char* value = "s";  };
	template<> struct __units<std::milli> 	    { static constexpr const char* value = "ms"; };
	template<> struct __units<std::micro>       { static constexpr const char* value = "us"; };
	template<> struct __units<std::nano>        { static constexpr const char* value = "ns"; };
	template<> struct __units<std::ratio<60>>   { static constexpr const char* value = "m";  };
	template<> struct __units<std::ratio<3600>> { static constexpr const char* value = "h";  };
	template<typename Duration> std::string units() {return std::string(__units<typename Duration::period>::value);}
	
	template<typename T> struct is_duration : std::false_type {};
	template<typename R, typename P> struct is_duration<std::chrono::duration<R, P>> : std::true_type {};
	template<typename Default, typename... Ts> struct __get_time_t { using type = Default; };
	template<typename Default, typename Head, typename... Tail>
	struct __get_time_t<Default, Head, Tail...> { 
		using type = std::conditional_t<is_duration<Head>::value, Head, typename __get_time_t<Default, Tail...>::type>; };
	template<typename Default, typename... Args> using get_time_t = typename __get_time_t<Default, Args...>::type;



	// Print options
	struct Sort 	  {}; // Sort by time
	struct Percentage {}; // Display percentage of outer timer
	struct Align 	  {}; // Align as columns
	
	constexpr size_t maxNameLength = 45;
	constexpr size_t maxTimeLength = 15;

	using clock = std::chrono::high_resolution_clock;
	
	// Tree structure for timers
	struct Timer
	{
		std::string name = "";
		clock::duration time;
		size_t depth = 0;

		Timer* parent = nullptr;
		std::vector<Timer*> children;

		Timer() {};
		Timer(Timer* p, std::string s) : name(s), depth(p->depth + 1), parent(p) {};
	};
	
	Timer* tree = new Timer();
	Timer* timer = tree;
	
	#define Measure __Measure __measurement
	class __Measure
	{
		std::chrono::time_point<clock> start;

	public:	
		
		__Measure(std::string name = "")
		{
			timer->children.push_back(new Timer(timer, name));
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
		std::sort(timer->children.begin(), timer->children.end(), [](const auto& a, const auto& b) { return a->time > b->time; });
		for(auto child: timer->children) sort(child);
	}
	
	template<typename time_t, typename... Options>
	std::string __string(Timer* timer)
	{
		std::stringstream stream;
		
		if(timer != tree)
		{
			// Timer name and depth
			for(size_t i = 0; i < timer->depth - 1; i++) stream << "| ";
			stream << timer->name + ": ";
			
			// Time measured in time_t
			if isOption(Align, Options) stream << std::right << std::setw(maxNameLength - stream.tellp());
			stream << std::chrono::duration_cast<time_t>(timer->time).count() << units<time_t>();
		
			// Percentage
			double percentage = 100.0 * timer->time / timer->parent->time;
			if isOption(Align, Options) stream << std::left << std::setw(maxNameLength + maxTimeLength - stream.tellp());
			if isOption(Percentage, Options) stream << " " << percentage << "%";	
			stream << std::endl;
		}

		for(auto child: timer->children) stream << __string<time_t, Options...>(child);
		
		return stream.str();
	}

	// Main function that converts the measurements to string
	template<typename... Options>
	std::string string()
	{
		using time_t = get_time_t<std::chrono::milliseconds, Options...>;
		
		if isOption(Sort, Options) sort(tree);

		if isOption(Percentage, Options)
			tree->time = std::accumulate(tree->children.begin(), tree->children.end(), clock::duration::zero(), 
					[](const auto& acc, const auto& child){ return acc + child->time; });
		
		return __string<time_t, Options...>(tree);
	}
	
}

int loop(size_t n = 20000000)
{

	volatile int sum = 0;
	for(size_t i = 0; i < n; i++) sum += i*i;
	return sum;
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

	std::cout << Timer::string<>() << std::endl;
	std::cout << Timer::string<Timer::Sort, Timer::Align>() << std::endl;
	std::cout << Timer::string<std::chrono::microseconds, Timer::Sort, Timer::Percentage, Timer::Align>() << std::endl;
	std::cout << Timer::string<std::chrono::nanoseconds, Timer::Align>() << std::endl; // Will still be sorted
}
