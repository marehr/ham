// Copyright (c) 2013-2014 Matthias Noack (ma.noack.pr@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>

#include "ham_iostream.hpp"

#include <cereal/types/vector.hpp>

using namespace ham;

double inner_product(offload::buffer_ptr<double> x, offload::buffer_ptr<double> y, size_t n)
{
    double z = 0.0;
    for (size_t i = 0; i < n; ++i)
        z += x[i] * y[i];
    return z;
}

void print_vector(auto && dna_vector)
{
  std::cout << "{";
  for (auto && dna_string: dna_vector) {
      std::cout << "{";
      for (auto && dna: dna_string)
      {
          std::cout << dna << ",";
      }
      std::cout << "}," << std::endl;
  }
  std::cout << "}" << std::endl;
}

void offload_print(offload::buffer_ptr<char> target_buf_ptr, size_t target_buf_size)
{
    std::vector<std::vector<unsigned>> dna_vector;
    ham_istream istream{target_buf_ptr.get(), target_buf_size};
    istream(dna_vector);

    std::cout << "On node: " << std::endl;
    print_vector(dna_vector);
}

int main([[gnu::unused]]int argc, [[gnu::unused]]char* argv[])
{
    // buffer size
    constexpr size_t n = 1024;

    // allocate host memory
    std::array<double, n> a, b;

    // initialise host memory
    for (size_t i = 0; i < n; ++i) {
        a[i] = i;
        b[i] = n-i;
    }

    // specify an offload target
    offload::node_t target = 1; // we simply the first device/node

    // get some information about the target
    auto target_description = offload::get_node_description(target);
    std::cout << "Using target node " << target << " with hostname " << target_description.name() << std::endl;

    // allocate device memory (returns a buffer_ptr<T>)
    auto a_target = offload::allocate<double>(target, n);
    auto b_target = offload::allocate<double>(target, n);

    std::vector<std::vector<unsigned>> dna_vector{{4294967295u,2}, {3,4}, {5,6}, {7,8,9}, {10, 11, 12}};
    ham_ostream ostream{};
    ostream(dna_vector);

    ham_ostream::buffer_ptr_t dna_vector_target;
    ham_ostream::future_t dna_vector_future;
    std::tie(dna_vector_target, dna_vector_future) = ostream.offload(target);

    // synchronise
    dna_vector_future.get();

    // asynchronously offload the call to inner_product
    offload::async(target, f2f(&offload_print, dna_vector_target, n));

    // char* remote_buffer;
    // size_t remote_buffer_size;
    // // on host
    // {
    //     ham_ostream ostream{};
    //     std::vector<std::vector<unsigned>> dna_vector{{4294967295u,2}, {3,4}, {5,6}, {7,8,9}, {10, 11, 12}};
    //     ostream(dna_vector);
    //
    //     // allocate data
    //     remote_buffer_size = ostream.byte_size();
    //     remote_buffer = new char[ostream.byte_size()];
    //
    //     // transfer data
    //     memcpy(remote_buffer, ostream.begin(), ostream.byte_size());
    // }
    //
    // // on target
    // {
    //     std::vector<std::vector<unsigned>> dna_vector;
    //     ham_istream istream{remote_buffer, remote_buffer_size};
    //     istream(dna_vector);
    //
    //     print_vector(dna_vector);
    // }
    // delete remote_buffer;

    // {
    //     std::vector<std::vector<unsigned>> dna_vector;
    //     std::ifstream file( "out.bin", std::ios::binary);
    //     cereal::BinaryInputArchive archive_in( file );
    //     archive_in(dna_vector);
    //
    //     std::cout << "{";
    //     for (auto && dna_string: dna_vector) {
    //         std::cout << "{";
    //         for (auto && dna: dna_string)
    //         {
    //             std::cout << dna << ",";
    //         }
    //         std::cout << "}," << std::endl;
    //     }
    //     std::cout << "}" << std::endl;
    // }

    // transfer data to the device (the target is implicitly specified by the destination buffer_ptr)
    auto future_a_put = offload::put(a.data(), a_target, n); // async
    offload::put(b.data(), b_target, n); // sync (implicitly returned future performs synchronisation in dtor), alternative: put_sync()

    // synchronise
    future_a_put.get();

    // asynchronously offload the call to inner_product
    auto c_future = offload::async(target, f2f(&inner_product, a_target, b_target, n));

    // synchronise on the result
    double c = c_future.get();

    // we also could have used:
    // double c = offload::async(...).get();
    // double c = offload::sync(...);
    // if we weren't interested in the offload's result, this would also be synchronous, because in this case, the returned future's dtor performs the synchronisation:
    // offload.async(...);

    // output the result
    std::cout << "Result: " << c << std::endl;

    return 0;
}
