
#pragma once

#include <sstream>

#include <cereal/archives/binary.hpp>

#define HAM_COMM_MPI 1
#include <ham/offload.hpp>
#undef HAM_COMM_MPI

struct ham_istream;

template <typename type_t = void>
struct ham_target_type
{
    using char_t = char;
    using buffer_ptr_t = ham::offload::buffer_ptr<char_t>;

    char_t* buffer_ptr;
    size_t byte_size;

    type_t get()
    {
        type_t result{};

        if(byte_size > 0)
        {
            ham_istream istream{buffer_ptr, byte_size};
            istream(result);
        }

        {
            buffer_ptr_t remote_ptr(buffer_ptr, ham::offload::this_node());
            ham::delete_buffer<char_t>{remote_ptr}();
        }
        buffer_ptr = nullptr;
        byte_size = 0;

        return std::move(result);
    }
};

struct ham_ostream
{
    struct my_stringbuf: std::stringbuf
    {
        using std::stringbuf::pbase;
        using std::stringbuf::pptr;
    };

    using char_t = char;
    using buffer_ptr_t = ham::offload::buffer_ptr<char_t>;
    using future_t = ham::offload::future<void>;

    std::ostringstream ostream{std::ios::binary};
    cereal::BinaryOutputArchive oarchive{ostream};

    template <typename ...args_t>
    void operator() (args_t && ...args)
    {
        oarchive(std::forward<args_t>(args)...);
        assert(ostream.tellp() == std::distance(begin(), end()));
    }

    char* begin()
    {
        return reinterpret_cast<ham_ostream::my_stringbuf*>(ostream.rdbuf())->pbase();
    }

    char* end()
    {
        return reinterpret_cast<ham_ostream::my_stringbuf*>(ostream.rdbuf())->pptr();
    }

    size_t byte_size()
    {
        return std::distance(begin(), end())*sizeof(char);
    }

    std::tuple<buffer_ptr_t, future_t> offload(ham::offload::node_t target)
    {
        auto rem_buf = ham::offload::allocate<char_t>(target, byte_size());
        auto future = ham::offload::put(begin(), rem_buf, byte_size());
        return std::make_tuple(std::move(rem_buf), std::move(future));
    }

    template <typename ...args_t>
    ham_target_type<std::remove_reference_t<args_t>...> put(ham::offload::node_t target_node, args_t && ...args)
    {
        auto && [target, future] = put_async(target_node, std::forward<args_t>(args)...);
        future.get();
        return std::move(target);
    }

    template <typename ...args_t>
    std::tuple<ham_target_type<std::remove_reference_t<args_t>...>, future_t> put_async(ham::offload::node_t target_node, args_t && ...args)
    {
        oarchive(std::forward<args_t>(args)...);

        auto buffer_ptr = ham::offload::allocate<char_t>(target_node, byte_size());
        ham_target_type<std::remove_reference_t<args_t>...> target{buffer_ptr.get(), byte_size()};

        auto future = ham::offload::put(begin(), buffer_ptr, byte_size());
        return std::make_tuple(std::move(target), std::move(future));
    }
};

struct ham_istream
{
    std::istringstream istream{std::ios::binary};
    cereal::BinaryInputArchive iarchive{istream};

    using char_t = ham_ostream::char_t;
    using buffer_ptr_t = ham_ostream::buffer_ptr_t;

    explicit ham_istream(buffer_ptr_t buf_ptr, size_t buf_size)
      : ham_istream(buf_ptr.get(), buf_size)
    {}

    explicit ham_istream(char_t* buf_ptr, size_t buf_size)
    {
        istream.rdbuf()->pubsetbuf(buf_ptr, buf_size);
    }

    template <typename ...args_t>
    void operator() (args_t & ...args)
    {
        iarchive(args...);
    }
};
