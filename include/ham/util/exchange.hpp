// Copyright (c) 2013-2014 Matthias Noack (ma.noack.pr@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/**
 * @file
 * This is a helper to extract values from std::tuple.
 */

#ifndef ham_util_exchange_hpp
#define ham_util_exchange_hpp

#include <utility>

namespace ham {
namespace util {

template<class T, class U = T>
T exchange(T& obj, U&& new_value)
{
	T old_value = std::move(obj);
	obj = std::forward<U>(new_value);
	return old_value;
}

} // namespace util
} // namespace ham

#endif // ham_util_exchange_hpp
