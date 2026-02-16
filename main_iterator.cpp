#include <iostream>

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <stack>

namespace Timer
{
	// Type and other shenanigans	
	#define Measure __Measure __measure
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
	std::string units() { return std::string(__units<typename Duration::period>::value); }
	
	template<typename T> struct is_duration : std::false_type {};
	template<typename R, typename P> struct is_duration<std::chrono::duration<R, P>> : std::true_type {};
	template<typename Default, typename... Ts> struct __get_time_t { using type = Default; };
	template<typename Default, typename Head, typename... Tail>
	struct __get_time_t<Default, Head, Tail...> 
		{ using type = std::conditional_t<is_duration<Head>::value, Head, typename __get_time_t<Default, Tail...>::type>; };
	template<typename Default, typename... Args>
	using get_time_t = typename __get_time_t<Default, Args...>::type;


	
	// Print options
	struct Sort 	  {}; // Sort by time
	struct Percentage {}; // Display percentage of outer timer
	struct Align 	  {}; // Align as columns


	// Tree structure for timers
	struct Node
	{
		std::string name;
		clock::duration time;
		size_t depth = 0;

		Node* parent = nullptr;
		std::vector<Node*> children;

		Node() {};
		Node(Node* p) : parent(p) { depth = parent->depth + 1; };


	class iterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = Node;
		using difference_type = std::ptrdiff_t;
		using pointer = Node*;
		using reference = Node&;
		
	private:
		std::stack<Node*> stack;
		Node* current;
		
	public:
		iterator(Node* node = nullptr) : current(node) { if (node) stack.push(node); }
		reference operator*() const { return *current; }
		pointer operator->() const { return current; }
		
		iterator& operator++()
		{
			if (stack.empty()) { current = nullptr; return *this; }
			current = stack.top(); stack.pop();
			
			// Push children in reverse order for left-to-right traversal
			for (auto it = current->children.rbegin(); it != current->children.rend(); ++it) stack.push(*it);
			return *this;
		}
		
		iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
		bool operator==(const iterator& other) const { return current == other.current; }
		bool operator!=(const iterator& other) const { return !(*this == other); }
	};
	
	iterator begin() { return iterator(this); }
	iterator end() { return iterator(nullptr); }
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
		std::sort(node->children.begin(), node->children.end(), [](const auto& a, const auto& b) { return a->time > b->time; });
		for(auto child: node->children) sort(child);
	}
	
	template<typename time_t, typename... Options>
	std::string __string(Node* node)
	{
		if (node == root) return "";

		std::stringstream stream;
	
		for(size_t i = 0; i < node->depth - 1; i++) stream << "| ";

		stream << node->name + ": ";
		
		std::string unit = units<time_t>();
		if isOption(Align, Options) stream << std::right << std::setw(60 - stream.tellp());
		stream << std::chrono::duration_cast<time_t>(node->time).count() << unit;
	
		double percentage = 100.0*node->time / node->parent->time;
		if isOption(Align, Options) stream << std::left << std::setw(60 - stream.tellp() + 20);
		if isOption(Percentage, Options) stream << " " << percentage << "%";	
	
		stream << std::endl;
		
		return stream.str();
	}

	// Main function that converts the measurements to string
	template<typename... Options>
	std::string string()
	{
		using time_t = get_time_t<std::chrono::milliseconds, Options...>;
		
		if isOption(Sort, Options) 	sort(root);

		if isOption(Percentage, Options)
			root->time = std::accumulate(root->children.begin(), root->children.end(), clock::duration::zero(), 
					[](const auto& acc, const auto& child){ return acc + child->time; });
		
		std::ostringstream stream;
		std::for_each(root->begin(), root->end(), [&](Node& node) { stream << __string<time_t, Options...>(&node); });
		return stream.str();
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

	std::cout << Timer::string<Timer::Align>() << std::endl;
	std::cout << Timer::string<Timer::Sort, Timer::Align>() << std::endl;
	std::cout << Timer::string<std::chrono::microseconds, Timer::Sort, Timer::Percentage, Timer::Align>() << std::endl;
	std::cout << Timer::string<std::chrono::nanoseconds, Timer::Align>() << std::endl;
}
