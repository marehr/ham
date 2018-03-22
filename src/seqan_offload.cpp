// Copyright (c) 2013-2014 Matthias Noack (ma.noack.pr@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <functional>

#include <seqan3/core/platform.hpp>
#include <seqan3/alphabet/nucleotide/dna4.hpp>
#include <seqan3/range/view/complement.hpp>
#include "ham_iostream.hpp"

#include <cereal/types/vector.hpp>

using namespace seqan3;
using namespace seqan3::literal;

void print_vector(auto && dna_vector)
{
  std::cout << "{" << std::endl;
  for (auto && dna_string: dna_vector) {
      std::cout << "    {";
      for (auto && dna: dna_string)
      {
          std::cout << dna << ",";
      }
      std::cout << "}," << std::endl;
  }
  std::cout << "}" << std::endl;
}

// namespace cereal
// {
// template<class Archive, seqan3::alphabet_concept alphabet_t>
// void save(Archive & archive, const alphabet_t & alphabet)
// {
//     archive( alphabet.to_rank() );
// }
//
// template<class Archive, seqan3::alphabet_concept alphabet_t>
// void load(Archive & archive, alphabet_t & alphabet)
// {
//     underlying_rank_t<alphabet_t> _rank;
//     archive( _rank );
//     alphabet.assign_rank(_rank);
// }
// } // namespace cereal

template <auto function_ptr, typename ...args_t>
auto ham_target_invoke(ham::offload::node_t target_node, args_t && ...args)
{
    using function_t = ham::function<decltype(function_ptr), function_ptr>;
    return ham::offload::async(target_node, function_t{std::forward<args_t...>(args...)});
}

template <auto function_ptr, typename ...args_t>
auto target_invoke(ham::offload::node_t target, args_t && ...args)
{
    using function_t = ham::function<decltype(function_ptr), function_ptr>;

    // workaround <=gcc7.2 bug
    auto apply = [&target](auto && arg)
    {
        ham_ostream ostream{};
        return ostream.put(target, arg);
    };
    // ham_ostream ostream{};
    // auto && [arg_target, arg_future] = ostream.put_async(target, args...);
    // arg_future.get();

    return ham::offload::async(target, function_t{apply(std::forward<args_t>(args))...});
}

size_t print_offload(ham_target_type<std::vector<dna4_vector>> texts_target, ham_target_type<std::vector<dna4_vector>> texts2_target)
{
    std::vector<dna4_vector> texts = texts_target.get();
    print_vector(texts);

    std::vector<dna4_vector> texts2 = texts2_target.get();
    print_vector(texts2);

    return 15 + 3;
}

// auto future = target_invoke<&complement_offload>(target, texts);
// auto complement_texts = future.get().get();
// print_vector(complement_texts);
// ham_target_type<std::vector<dna4_vector>>
auto
complement_offload(ham_target_type<std::vector<dna4_vector>> texts_target)
{
    std::vector<dna4_vector> texts = texts_target.get();
    std::vector<dna4_vector> complement_texts{};

    for(auto && text: texts)
    {
        auto complement_text_view = text | view::complement;
        complement_texts.emplace_back(std::begin(complement_text_view), std::end(complement_text_view));
    }

    print_vector(complement_texts);

    ham_ostream ostream{};
    auto && [target, future] = ostream.put_async(ham::offload::node_t{0}, complement_texts);
    future.get();
    return 1+4;
}

// auto future1 = ham_target_invoke<&pod_offload>(target, 'B');
// auto result1 = future1.get();
// std::cout << "result1: " << result1 << std::endl;
auto pod_offload(char test)
{
    ham::offload::node_t this_node = ham::offload::this_node();
    char hello[6] = "HELLO";
    hello[0] = test;
    auto hello_target = ham::offload::allocate<char>(this_node, sizeof(hello)); // "HELLO\0"
    ham::offload::put((char*)hello, hello_target, sizeof(hello));
    return 5;
}

struct dna_statistics {
    unsigned dna_a{0}, dna_c{0}, dna_g{0}, dna_t{0};
};

dna_statistics
calculate_statistics_offload(
    ham_target_type<std::vector<dna4_vector>> texts_target
) {
    dna_statistics stats{};
    std::vector<dna4_vector> texts = texts_target.get();
    for(auto && text: texts){
        for(auto && chr: text) {
            stats.dna_a += chr == dna4::A;
            stats.dna_c += chr == dna4::C;
            stats.dna_g += chr == dna4::G;
            stats.dna_t += chr == dna4::T;
        }
    }
    return stats;
}

int main([[gnu::unused]]int argc, [[gnu::unused]]char* argv[])
{
    std::vector<dna4_vector> texts = {
        "ACGT"_dna4, "CGTA"_dna4, "ACGATATA"_dna4
    };

    ham::offload::node_t target = 1;
    auto target_description = ham::offload::get_node_description(target);

    std::cout << "Using target node " << target << " with hostname "
              << target_description.name() << std::endl;

    auto future = target_invoke<&calculate_statistics_offload>(target, texts);
    dna_statistics stats = future.get();
    std::cout << "A: " << stats.dna_a << "\nC: " << stats.dna_c <<
               "\nG: " << stats.dna_g << "\nT: " << stats.dna_t << std::endl;

    return 0;
}
