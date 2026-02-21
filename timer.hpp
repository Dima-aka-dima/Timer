#include <chrono>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm> // std::sort
#include <numeric>   // std::accumulate
#include <iomanip>   // std::setw

namespace Timer
{
	// Type shenanigans	
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
	
	const std::string RESET   = "\033[0m";	  // Default
	const std::string DIM     = "\033[2m";    // Tree pipes |
	const std::string CYAN    = "\033[36m";   // Time values
	const std::string GREEN   = "\033[32m";   // Low percentages
	const std::string YELLOW  = "\033[33m";   // Mid percentages
	const std::string RED     = "\033[31m";   // High percentages

	const std::string& percentageColor(double percentage)
	{
		if(percentage < 100.0*1.0/3.0) return GREEN;
		if(percentage < 100.0*2.0/3.0) return YELLOW;
		return RED;
	}

	// Print options
	struct Sort 	  {}; // Sort by time
	struct Percentage {}; // Display percentage of outer timer
	struct Align 	  {}; // Align as columns
	struct Color      {}; // Color the output
	
	// TODO:
	// struct Units      {}; // Automatic units

	size_t maxNameLength = 0;
	size_t maxDepth = 0;
	constexpr size_t maxTimeLength = 10;


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

	// Main measurement functions
	std::vector<std::chrono::time_point<clock>> starts;
	void Start(std::string name = "")
	{
		maxNameLength = std::max(name.size(), maxNameLength);
		timer->children.push_back(new Timer(timer, name));
		timer = timer->children.back();

		starts.push_back(clock::now());
	}

	void Stop()
	{
		auto duration = clock::now() - starts.back();
		starts.pop_back();
			
		maxDepth = std::max(timer->depth, maxDepth);
		timer->time = duration;
		timer = timer->parent;

	}

	// Measuring in scope. Construction (destruction)
	// corresponds to starting (stopping) the measuremsent
	#define Scope __Scope __measurement
	class __Scope
	{
	public:	
		__Scope(std::string name = "") { Start(name); }
		~__Scope() { Stop(); }
	};
	


	void sort(Timer* timer)
	{
		std::sort(timer->children.begin(), timer->children.end(), [](const auto& a, const auto& b) { return a->time > b->time; });
		for(auto child: timer->children) sort(child);
	}
	
	// Converts one measurement to string 
	template<typename time_t, typename... Options>
	std::string string(Timer* timer)
	{
		std::stringstream stream;
		
		if(timer != tree)
		{
			// Timer depth
			if isOption(Color, Options) stream << DIM;
			for(size_t i = 0; i < timer->depth - 1; i++) stream << "| ";
			stream << RESET;
		
			// Timer name
			size_t depthLength = 3; if isOption(Color, Options) depthLength++;
			if isOption(Align, Options) stream << std::left << std::setw(maxNameLength + depthLength*maxDepth - stream.tellp());
			stream << timer->name + ": ";
			
			// Time measured in time_t
			if isOption(Color, Options) stream << CYAN;
			if isOption(Align, Options) stream << std::right << std::setw(maxTimeLength);
			stream << std::chrono::duration_cast<time_t>(timer->time).count();
			stream << RESET << units<time_t>();
		
			// Percentage
			double percentage = 100.0 * timer->time / timer->parent->time;
			if isOption(Color, Options) stream << percentageColor(percentage);
			if isOption(Percentage, Options) stream << "\t\t" << percentage << "%";	
			stream << RESET;

			stream << std::endl;
		}

		for(auto child: timer->children) stream << string<time_t, Options...>(child);
		
		return stream.str();
	}

	// Main function that converts measurements to string
	template<typename... Options>
	std::string string()
	{
		if(not starts.empty()) return "\033[31mError: Not all timers have stopped"; 

		using time_t = get_time_t<std::chrono::milliseconds, Options...>;
		
		if isOption(Sort, Options) sort(tree);

		if isOption(Percentage, Options)
			tree->time = std::accumulate(tree->children.begin(), tree->children.end(), clock::duration::zero(), 
					[](const auto& acc, const auto& child){ return acc + child->time; });
		
		return string<time_t, Options...>(tree);
	}
	
}
