#include <iostream>

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <typeindex>

namespace Timer
{
	
	#define Measure __Measure _measurement
	
	using clock = std::chrono::high_resolution_clock;
	std::unordered_map<std::type_index, std::string> units = 
	{
		{typeid(std::chrono::nanoseconds),  "ns" },
		{typeid(std::chrono::microseconds), "us" },
		{typeid(std::chrono::milliseconds), "ms" },
		{typeid(std::chrono::seconds),      "s"  },
		{typeid(std::chrono::minutes),      "min"},
		{typeid(std::chrono::hours),        "h"  }
	};
	
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

	Node* __get(std::string name, Node* node)
	{
		if(node->name == name) return node;
		for(auto child: node->children) if(Node* found = __get(name, child)) return found;
		return nullptr;
	}

	clock::duration get(std::string name)
	{
		Node* node =  __get(name, root);
		if(node == nullptr) return clock::duration::zero();
		return node->time;
	}

	std::unordered_map<Node*, size_t> loopCounts;
	void setLoopCount(std::string name, size_t loopCount)
	{
		Node* node = __get(name, root);
		if(node == nullptr) return;
		node->name += " (" + std::to_string(loopCount) + ")";
		loopCounts[node] = loopCount;
	}

	void sort(Node* node)
	{
		std::sort(node->children.begin(), node->children.end(), [](const Node* a, const Node* b) 
				{ return a->time > b->time; });
		for(auto child: node->children) sort(child);
	}

	size_t getDepth(Node* node)
	{
		if(node == root) return 0;
		return getDepth(node->parent) + 1;
	}

	double getPercentage(Node* node)
	{
		if(node->parent == root) return 100.0;
		return 100.0 *node->time / node->parent->time;
	}

	template<typename time_t> 
	std::string getTime(Node* node)
	{
		uint64_t time = std::chrono::duration_cast<time_t>(node->time).count();
		if(loopCounts.find(node) != loopCounts.end()) time /= loopCounts[node];
		return std::to_string(time);
	}
	
	size_t __setNameAlignment(Node* node)
	{
		size_t offset = 0;
		for(auto child: node->children) offset = std::max(offset, __setNameAlignment(child));
		return std::max(node->name.size() + 2*getDepth(node), offset);
	}

	template <typename time_t>
	size_t __setTimeAlignment(Node* node)
	{
		size_t offset = 0;
		for(auto child: node->children) offset = std::max(offset, __setTimeAlignment<time_t>(child));
		return std::max(getTime<time_t>(node).size(), offset);
	}

	size_t nameAlignment = 0;
	size_t timeAlignment = 0;
	
	template <typename time_t>
	void setAlignment()
	{
		nameAlignment = __setNameAlignment(root);
		timeAlignment = __setTimeAlignment<time_t>(root);
	}

	const uint64_t Depth      = 0b0001;
	const uint64_t Sort 	  = 0b0010;
	const uint64_t Percentage = 0b0100;
	const uint64_t Align      = 0b1000;	

	template<typename time_t>
	std::string __string(Node* node, uint64_t flags)
	{
		std::ostringstream stream;
		
		if(flags & Depth) for(size_t i = 0; i < getDepth(node) - 1; i++) stream << "| ";
	
		if(flags & Align) stream << std::left <<\
			std::setw(nameAlignment + ((flags & Depth ? -2*getDepth(node) : 0)) + 3);
		stream << node->name + ": ";

		std::string unit = units[typeid(time_t)];
		if(flags & Align) stream << std::left << std::setw(timeAlignment + unit.size() + 1);
		stream << getTime<time_t>(node) + unit;
		
		if(flags & Percentage) stream << " " << getPercentage(node) << "%";
		
		stream << std::endl;

		for(auto child: node->children) stream << __string<time_t>(child, flags);
		return stream.str();
	}
	
	template<typename time_t = std::chrono::milliseconds>
	std::string string(uint64_t flags = Timer::Depth | Timer::Align | Timer::Percentage | Timer::Sort)
	{
		if (flags & Sort) sort(root); 
		if (flags & Align) setAlignment<time_t>();
		std::ostringstream stream;
		for(auto child: root->children) stream << __string<time_t>(child, flags);
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
	
	Timer::setLoopCount("Third Loop", 10);
	std::cout << Timer::string<std::chrono::nanoseconds>();
	std::cout << Timer::string<std::chrono::nanoseconds>(Timer::Align);
	std::cout << Timer::get("Outer Timer").count() << std::endl;

}
