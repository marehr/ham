// Copyright (c) 2013-2014 Matthias Noack (ma.noack.pr@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define HAM_COMM_MPI 1

#include "ham/offload.hpp"
#include <array>
#include <iostream>
#include <sstream>

using namespace ham;

using my_t = std::vector<std::vector<...>>

my_t* deserialise(offload::buffer_ptr<char> data)
	return new my_t(sequan::deserailse(data.get())) // data.get() -> buffer ptr zu bekommen

double inner_product(offload::buffer_ptr<doule> x, offload::buffer_ptr<double> y, size_t n)
{
	double z = 0.0;
	for (size_t i = 0; i < n; ++i)
		z += x[i] * y[i];
	return z;
}

#define t 2

int main(int argc, char* argv[])
{
	// buffer size
	constexpr size_t n = 1024 << 12;

	if (offload::num_nodes() <= t)
	{
		std::cout << "needs at least " << (t+1) << " nodes" << std::endl;
		return 0;
	}

	// allocate host memory
	// std::vector<double> a{n}, b{n};
	std::vector<double> a, b;
	a.resize(n);
	b.resize(n);

	// initialise host memory
	for (size_t i = 0; i < n; ++i) {
		a[i] = i;
		b[i] = n-i;
	}

	// specify an offload target
	offload::node_t target1 = 1; // we simply the first device/node
	offload::node_t target2 = 2; // we simply the first device/node

	// get some information about the target1
	auto target1_description = offload::get_node_description(target1);
	bool is_host = offload::is_host();
	std::stringstream host;
	host << ( is_host ? "host: " : "offload: " );
	host << "(" << offload::num_nodes() << ") ";

	std::cout << host.rdbuf() << "Using target node " << target1 << " with hostname " << target1_description.name() << std::endl;
	auto target2_description = offload::get_node_description(target2);
	std::cout << "HEllo" << std::endl;
	// std::cout << host.rdbuf() << "Using target node " << target2 << " with hostname " << target2_description.name() << std::endl;
	std::cout << "HEllo2" << std::endl;

	// allocate device memory (returns a buffer_ptr<T>)
	auto a_target1 = offload::allocate<double>(target1, n/t);
	auto b_target1 = offload::allocate<double>(target1, n/t);
#if t >= 2
	auto a_target2 = offload::allocate<double>(target2, n/t);
	auto b_target2 = offload::allocate<double>(target2, n/t);
#endif

	// transfer data to the device (the target is implicitly specified by the destination buffer_ptr)
	{
		auto future_a_put1 = offload::put(a.data(), a_target1, n/t); // async
		auto future_b_put1 = offload::put(b.data(), b_target1, n/t); // async
#if t >= 2
		auto future_a_put2 = offload::put(a.data() + n/t, a_target2, n/t); // async
		offload::put(b.data() + n/t, b_target2, n/t); // sync (implicitly returned future performs synchronisation in dtor), alternative: put_sync()
#endif
	}

	// asynchronously offload the call to inner_product
	auto c_future1 = offload::async(target1, f2f(&inner_product, a_target1, b_target1, n/t));
#if t >= 2
	auto c_future2 = offload::async(target2, f2f(&inner_product, a_target2, b_target2, n/t));
#endif

	// synchronise on the result
	double c = c_future1.get()
#if t >= 2
	+ c_future2.get()
#endif
	;

	// we also could have used:
	// double c = offload::async(...).get();
	// double c = offload::sync(...);
	// if we weren't interested in the offload's result, this would also be synchronous, because in this case, the returned future's dtor performs the synchronisation:
	// offload.async(...);

	// output the result
	std::cout << "Result: " << c << std::endl;

	return 0;
}
