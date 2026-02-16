#include <iostream>

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include <numeric>
#include <iomanip>

namespace Timer
{
	
	#define Measure __Measure _
	#define isOption(Option, ...) constexpr ((std::is_same_v<Option, __VA_ARGS__> || ...))	
	
	using clock = std::chrono::high_resolution_clock;
	
	template<typename Period> struct __units    { static constexpr const char* value = "?";  };
	template<> struct __units<std::ratio<1>>    { static constexpr const char* value = "s";  };
	template<> struct __units<std::milli> 	    { static constexpr const char* value = "ms"; };
	template<> struct __units<std::micro>       { static constexpr const char* value = "us"; };
	template<> struct __units<std::nano>        { static constexpr const char* value = "ns"; };
	template<> struct __units<std::ratio<60>>   { static constexpr const char* value = "m";  };
	template<> struct __units<std::ratio<3600>> { static constexpr const char* value = "h";  };
	template<typename Duration>
	std::string units() {return std::string(__units<typename Duration::period>::value);}
	
	template<typename T> struct is_duration : std::false_type {};
	template<typename R, typename P> struct is_duration<std::chrono::duration<R, P>> : std::true_type {};
	template<typename Default, typename... Ts> struct __get_time_t { using type = Default; };
	template<typename Default, typename Head, typename... Tail>
	struct __get_time_t<Default, Head, Tail...> 
		{ using type = std::conditional_t<is_duration<Head>::value, Head, typename __get_time_t<Default, Tail...>::type>; };
	template<typename Default, typename... Args>
	using get_time_t = typename __get_time_t<Default, Args...>::type;
	
	struct Sort {};
	struct Percentage {};
	struct Align {};
	
	struct Node
	{
		std::string name;
		clock::duration time;

		Node* parent = nullptr;
		std::vector<Node*> children;

		Node() {};
		Node(Node* p) : parent(p) {};
	};
	
	Node* root = new Node();
	Node* timer = root;

	class __Measure
	{
		std::chrono::time_point<clock> start;

	public:	
		
		__Measure(std::string s = "")
		{
			timer->children.push_back(new Node(timer));
			timer = timer->children.back();
			timer->name = s;			

			start = clock::now();
		}

		~__Measure()
		{
			auto duration = clock::now() - start;
			
			timer->time = duration;
			timer = timer->parent;
		}
	};

	void sort(Node* node)
	{
		std::sort(node->children.begin(), node->children.end(), [](const auto& a, const auto& b) 
				{ return a->time > b->time; });
		for(auto child: node->children) sort(child);
	}

	std::unordered_map<Node*, double> percentages;
	void setPercentages(Node* node)
	{
		if(node == root)
			node->time = std::accumulate(node->children.begin(), node->children.end(), clock::duration::zero(), 
					[](const auto& acc, const auto& child){ return acc + child->time; });

		for(auto child: node->children)
		{
			percentages[child] = 100.0 * child->time / node->time;
			setPercentages(child);
		}
	}

	std::unordered_map<Node*, size_t> alignments;
	void setAlignments(Node* node)
	{
		size_t offset = 0;
		for(auto child: node->children) offset = std::max(child->name.size(), offset);
		alignments[node] = offset;
		for(auto child: node->children) setAlignments(child);
	}

	template<typename time_t, typename... Options>
	std::string __string(Node* node, size_t depth = 0)
	{
		std::ostringstream stream;
		
		if(node != root)
		{
			for(size_t i = 0; i < depth - 1; i++) stream << "| ";

			std::string unit = units<time_t>();

			if isOption(Align, Options) stream << std::left << std::setw(alignments[node->parent] + 2);
			stream << node->name + ": " << std::chrono::duration_cast<time_t>(node->time).count() << unit;
			
			if isOption(Percentage, Options) stream << " " << percentages[node] << "%";	
			stream << std::endl;
		}
		
		for(auto child: node->children) 
			stream << __string<time_t, Options...>(child, depth + 1);
		
		return stream.str();
	}

	template<typename... Options>
	std::string string()
	{
		using time_t = get_time_t<std::chrono::milliseconds, Options...>;
		
		if isOption(Sort, Options) sort(root);
		if isOption(Percentage, Options) setPercentages(root);
		if isOption(Align, Options) setAlignments(root);
		return __string<time_t, Options...>(root);
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

	std::cout << Timer::string<Timer::Sort, Timer::Percentage, Timer::Align>();
}
