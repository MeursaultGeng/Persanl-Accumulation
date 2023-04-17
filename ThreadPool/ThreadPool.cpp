#include "threadpool.h"
#include <random>
#include <iostream>

std::random_device rd;  //true random number generator
std::mt19937 mt(rd());   //to generate calculated random number
std::uniform_int_distribution<int> dist(-1000, 1000);
auto rnd = std::bind(dist, mt);

void simulate_hard_computation()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
}

void multiply(const int a, const int b)
{
	simulate_hard_computation();
	const int res = a * b;
	std::cout << a << " * " << b << " = " << res << std::endl;
}

void multiply_output(int &out, const int a, const int b)
{
	simulate_hard_computation();
	out = a * b;
	std::cout << a << " * " << b << " = " << out << std::endl;
}

int multiply_return(const int a, const int b)
{
	simulate_hard_computation();
	const int res = a * b;
	std::cout << a << " * " << b << " = " << res << std::endl;
	return res;
}

void example()
{
	ThreadPool pool(3);    //to create 3 thread pool
	pool.init();

	//to submit multiply operator, totally 30
	for (int i = 1; i <= 3; ++i)
		for (int j = 1; j <= 10; ++j)
			pool.submit(multiply, i, j);

	//use the output value refered by ref to submit function
	int output_ref;
	auto future1 = pool.submit(multiply_output, std::ref(output_ref), 5, 6);

	//wait for multiply_output to finish
	future1.wait();
	std::cout << "Last operation result is equals to " << output_ref << std::endl;

	//to use return value to submit function
	auto future2 = pool.submit(multiply_return, 5, 3);

	//wait for multiply_return function to finish
	int res = future2.get();
	std::cout << "Last operation result is equals to " << res << std::endl;

	//to shut down thread pool
	pool.shutdown();
}

int main()
{
	example();
	
	return 0;
}

