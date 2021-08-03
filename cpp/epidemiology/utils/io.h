/**
 * This file describes the framework for IO. The framework is format-independent, 
 * all the format specific implementations are in the epidemiology_io library.
 * 
 * Main functions and types:
 * -------------------------
 * - functions serialize and deserialize: 
 *      Main entry points to the framework to write and read values, respectively. The functions expect an IOContext
 *      (see Concepts below) that stores the serialized data. (De-)serialization can be customized by providing a
 *      (de-)serialize_internal overload or a (de-)serialize member function for the type. See the section "Implementing 
 *      serialization for a new type" or the documentation for `serialize` and `deserialize`.
 * - IOStatus and IOResult: 
 *      Used for error handling, see section "Error Handling" below.
 * 
 * Concepts: 
 * ---------
 * 1. IOContext
 * Stores data that describes serialized objects of any type in some unspecified format and provides structured
 * access to the data for deserialization. Implementations of this concept may store the data in any format
 * they want including binary. The data may also be written directly to disk. The context also keeps track
 * of errors. An IOContext object `io` allows the following operations:
 * - `io.create_object("Type")`: 
 *      Returns an IOObject for the type called `"Type"`. The IOObject (see below) allows adding data that describes 
 *      the object to be serialized. The function must return something that can be assigned to a local
 *      variable, e.g., a temporary or copyable function. IOObject may store references to the context internally,
 *      so the lifetime of the local IOObject may not exceed the lifetime of the IOContext that created it.
 * - `io.expect_object("Type")`: 
 *      Returns an IOObject for the type called `"Type"`.
 *      The IOObject (see below) provides access to the data needed for deserialization. 
 * - `io.flags()`:
 *      Returns the flags that determine the behavior of serialization; see IOFlags.
 * - `io.error()`:
 *      Returns an IOStatus object to check if there were any errors during serialization.
 *      Usually it is not necessary to check this manually but can be used to report the error faster and 
 *      avoid expensive operations that would be wasted anyway.
 * - `io.set_error(s)` with some IOStatus object: 
 *      Stores an error that was generated outside of the IOContext, e.g., if a value that was deserialized 
 *      is outside an allowed range.
 * 2. IOObject
 * Gives structured access to serialized data. During serialization, data can be added with `add_...` operations.
 * During deserialization, data can be retrieved with `expect_...` operations. Data must be retrieved in the same order
 * as it was added since, e.g., binary format does not allow lookup by key. The following operations are supported 
 * for an IOObject `obj`:
 * - `obj.add_element("Name", t)`: 
 *      Stores an object `t` in the IOObject under the key "Name". If `t` is of basic type (i.e., int, string), IOObject 
 *      is expected to handle it directly. Otherwise, the object uses `epi::serialize` to get the data for `t`.
 * - `obj.add_list("Name", b, e)`:
 *      Stores the elements in the range represented by iterators `b` and `e` under the key "Name". The individual elements are not named.
 *      The elements are either handled directly by the IOObject or using `epi::serialize` just like `add_element`.
 * - `obj.add_optional("Name", p)`:
 *      Stores the element pointed to by pointer `p` under the key "Name". The pointer may be null. Otherwise identical to add_element.
 * - `obj.expect_element("Name", Tag<T>{})`:
 *      If an object of type T can be found under the key "Name" and can be deserialized, returns the object. Otherwise returns
 *      an error. Analogously to serialization, the IOObject is expected to handle basic types directly and use `epi::deserialize`
 *      otherwise.
 * - `obj.expect_list("Name", Tag<T>{})`:
 *      If a list of objects of type T can be found under the key "Name" and can be deserialized, returns a range that can be 
 *      iterated over. Otherwise returns an error.
 * - `obj.expect_optional("Name", Tag<T>{})`:
 *      Returns boost::optional<T> if an optional value of type T can be found under the key "Name". The optional may contain a 
 *      value or it may be empty. Otherwise returns an error. Note that for some formats a wrong key is indistinguishable from 
 *      an empty optional, so make sure to provide the correct key.
 *  
 * Error handling:
 * ---------------
 * Errors are handled by returning error codes. The type IOStatus contains an error code and an optional string with additional 
 * information. The type IOResult contains either a value or an IOStatus that describes an error. Operations that can fail return 
 * an IOResult<T> where T is the type of the value that is produced by the operation if it is succesful. Except where necessary
 * because of dependencies, the framework does not throw nor catch any exceptions. IOContext and IOObject implementations are
 * expected to store errors. During serialization, `add_...` operations fail without returning errors, but the error is stored 
 * in the IOObject and subsequent calls are usually no-ops. During deserialization, the values produced must usually be used or 
 * inspected, so `expect_...` operations return an IOResult. The `apply` utility function provides a simple way to inspect the result 
 * of multiple `expect_...` operations and use the values if all are succesful. See the documentation of `IOStatus`, `IOResult`
 * and `apply` below for more details.
 * 
 * Implementing serialization for a new type:
 * ------------------------------------------
 * Serialization of a new type T can be customized by providing _either_ member functions `serialize` and `deserialize` _or_ free functions 
 * `serialize_internal` and `deserialize_internal`.
 * 
 * The `void serialize(IOContext& io)` member function takes an IO context and uses `create_object` and `add_...` operations
 *  to add data. The static `IOResult<T> deserialize(IOContext& io)` member function takes an IO context and uses `expect_...` 
 * operations to retrieve the data. The `apply` utility function can be used to inspect the result of the `expect_...`
 * operations and construct the object of type T.
 * E.g.:
 * ```
 * struct Foo {
 *   int i;
 *   template<class IOContext>
 *   void serialize(IOContext& io) {
 *     auto obj = io.create_object("Foo");
 *     obj.add_element("i", i);
 *   }
 *   template<class IOContext>
 *   static IOResult<Foo> deserialize(IOContext& io) {
 *     auto obj = io.expect_object("Foo");
 *     auto i_result = obj.expect_element("i", epi::Tag<int>{});
 *     return epi::apply(io, [](auto&& i) { return Foo{i}; }, i_result);
 *   }
 * };
 * ```
 * 
 * The free functions `serialize_internal` and `deserialize_internal` must be found with argument dependent lookup (ADL). 
 * They can be used if no member function should or can be added to the type. See the code below for examples where 
 * this was done for, e.g., Eigen3 matrices and STL containers.
 * 
 * Adding a new serialization format:
 * ----------------------------------
 * Implement concepts IOContext and IOObject that provide the operations listed above. Your implemenation should handle
 * all built in types as well as std::string. It may handle other types (e.g., STL containers) as well if it can do so
 * more efficiently than the provided general free functions. See the JsonSerializer implementation in the epidemiology_io
 * library for an example.
 */

#ifndef EPI_UTILS_IO_H
#define EPI_UTILS_IO_H

#include "epidemiology/utils/metaprogramming.h"
#include "epidemiology/utils/eigen_util.h"
#include "boost/outcome/result.hpp"
#include "boost/outcome/try.hpp"
#include <tuple>
#include <iostream>

namespace epi
{

/**
 * code to indicate the result of an operation.
 * convertible to std::error_code.
 * see https://www.boost.org/doc/libs/1_75_0/libs/outcome/doc/html/motivation/plug_error_code.html
 */
enum class StatusCode
{
    OK           = 0, //must be 0 because std::error_code stores ints and expects 0 to be no error
    UnknownError = 1, //errors must be non-zero
    OutOfRange,
    InvalidValue,
    InvalidFileFormat,
    KeyNotFound,
    InvalidType,
    FileNotFound,
};

/**
 * flags to determine the behavior of the serialization process.
 */
enum IOFlags
{
    /**
     * default behavior.
     */
    IOF_None = 0,

    /**
     * Don't serialize distributions for types that contain both a specific value and a distribution 
     * from which new values can be sampled, e.g. UncertainValue.
     */
    IOF_OmitDistributions = 1 << 0,

    /**
     * Don't serialize the current value for types that contain both a specific value and a distribution 
     * from which new values can be sampled, e.g., UncertainValue.
     */
    IOF_OmitValues = 1 << 1,
};

} // namespace epi

namespace std
{

//make enum compatible with std::error_code
template <>
struct is_error_code_enum<epi::StatusCode> : true_type {
};

} // namespace std

namespace epi
{

namespace detail
{
    /**
     * category that describes StatusCode in std::error_code
     */
    class StatusCodeCategory : public std::error_category
    {
    public:
        /**
         * name of the status
         */
        virtual const char* name() const noexcept override final
        {
            return "StatusCode";
        }

        /**
         * convert enum to string message.
         */
        virtual std::string message(int c) const override final
        {
            switch (static_cast<StatusCode>(c)) {
            case StatusCode::OK:
                return "No error";
            case StatusCode::OutOfRange:
                return "Invalid range";
            case StatusCode::InvalidValue:
                return "Invalid value";
            case StatusCode::InvalidFileFormat:
                return "Invalid file format";
            case StatusCode::KeyNotFound:
                return "Key not found";
            case StatusCode::InvalidType:
                return "Invalid type";
            case StatusCode::FileNotFound:
                return "File not found";
            default:
                return "Unknown Error";
            }
        }

        /**
         * convert to standard error code to make it comparable.
         */
        virtual std::error_condition default_error_condition(int c) const noexcept override final
        {
            switch (static_cast<StatusCode>(c)) {
            case StatusCode::OK:
                return std::error_condition();
            case StatusCode::OutOfRange:
                return std::make_error_condition(std::errc::argument_out_of_domain);
            case StatusCode::InvalidValue:
            case StatusCode::InvalidFileFormat:
            case StatusCode::InvalidType:
            case StatusCode::KeyNotFound:
                return std::make_error_condition(std::errc::invalid_argument);
            case StatusCode::FileNotFound:
                return std::make_error_condition(std::errc::no_such_file_or_directory);
            default:
                return std::error_condition(c, *this);
            }
        }
    };
} // namespace detail

/**
 * singleton StatusCodeCategory instance.
 */
inline const detail::StatusCodeCategory& status_code_category()
{
    static detail::StatusCodeCategory c;
    return c;
}

/**
 * Convert StatusCode to std::error_code.
 * Expected customization point of std::error_code.
 */
inline std::error_code make_error_code(StatusCode e)
{
    return {static_cast<int>(e), status_code_category()};
}

/**
 * IOStatus represents the result of an operation.
 * Consists of an error code and additional information in a string message.
 * The error code may be a value from epi::StatusCode or from a dependency. 
 * The category of the error code can be inspected to see where the 
 * error code originated; see https://en.cppreference.com/w/cpp/error/error_code. 
 */
class IOStatus
{
public:
    /**
     * default constructor, represents success
     */
    IOStatus() = default;

    /**
     * constructor with error code and message
     * @param ec error code
     * @param msg optional message with additional information about the error, default empty.
     */
    IOStatus(std::error_code ec, const std::string& msg = {})
        : m_ec(ec)
        , m_msg(msg)
    {
    }

    /**
     * equality comparison operators.
     * @{
     */
    bool operator==(const IOStatus& other) const
    {
        return other.m_ec == m_ec;
    }
    bool operator!=(const IOStatus& other) const
    {
        return !(*this == other);
    }
    /**@}*/

    /**
     * get the error code.
     */
    const std::error_code& code() const
    {
        return m_ec;
    }

    /**
     * get the message that contains additional information about errors.
     */
    const std::string& message() const
    {
        return m_msg;
    }

    /**
     * Check if the status represents success.
     * `is_ok()` is Equivalent to `!bool(code())`.
     * @return true if status is success, false otherwise.
     * @{
     */
    bool is_ok() const
    {
        return !(bool)m_ec;
    }
    operator bool() const
    {
        return is_ok();
    }
    /**@}*/

    /**
     * Check if the status represents failure.
     * @return true if status is failure, false otherwise.
     * @{
     */
    bool is_error() const
    {
        return !is_ok();
    }

    /**
     * Get a string that combines the error code and the message.
     */
    std::string formatted_message() const
    {
        if (is_error()) {
            return m_ec.message() + ": " + m_msg;
        }
        return {};
    }

private:
    std::error_code m_ec = {};
    std::string m_msg    = {};
};

/**
 * gtest printer for IOStatus.
 */
inline void PrintTo(const IOStatus& status, std::ostream* os)
{
    *os << status.formatted_message();
}

/**
 * Convert IOStatus to std::error_code.
 * Expected customization point of std::error_code.
 */
inline const std::error_code& make_error_code(const IOStatus& status)
{
    return status.code();
}

/**
 * Value-or-error type for operations that return a value but can fail.
 * Can also be used for functions that return void so that all
 * IO functions have compatible signatures.
 * e.g.
 * `IOResult<int> parse_int(const std::string& s);`
 * `IOResult<void> mkdir(const std::string& path);`
 * 
 * Create IOResult objects with:
 * - `success(t)`, `failure(e)`: Create objects that store a T or IOStatus and are implicitly convertible to IOResult.
 *                               This is the easiest way to return from a function that returns an IOResult.
 * - constructors: IOResult can normally be constructed directly from T or IOStatus.
 *                 If T is convertible to an error code (e.g. T = int), these constructors are disabled.
 *                 There are constructors IOResult(Tag<T>{}, t) and IOResult(Tag<IOStatus>{}, e) that always work.
 * 
 * Inspect the result with:
 * - `operator bool()`: true if result represents success.
 * - `value()`: returns a reference to the value.
 * - `error()`: returns a reference to the error.
 * `value()`/`error()` assert (terminate in debug mode) if 
 * the result is not succesful/not an error.
 * 
 * When nesting functions that return IOResult, it is also possible to unpack the value
 * and forward errors using the macro BOOST_OUTCOME_TRY. 
 * The statement `BOOST_OUTCOME_TRY(x, try_get_x());` is equivalent to the statements
 * ```
 * auto result = try_get_x();
 * if (!result) {
 *   return result.as_failure();
 * }
 * auto&& x = result.value();
 * ``` 
 * This way, code the branches that are added for error handling are not visible in 
 * your code, the logic looks completely linear, e.g.:
 * ```
 * extern void use_int(int i);
 * IOResult<void> parse_and_use_int(const std::string& s)
 * {
 *   BOOST_OUTCOME_TRY(i, parse_int(s));
 *   use_int(i);
 *   return success();
 * }
 * ```
 * The variable name can be omitted for operations that return IOResult<void>.
 * 
 * @see https://www.boost.org/doc/libs/1_75_0/libs/outcome/doc/html/index.html
 * @tparam the type produced by an opertion that can fail.
 */
template <class T>
using IOResult = boost::outcome_v2::unchecked<T, IOStatus>;

/**
 * Create an object that is implicitly convertible to a succesful IOResult<void>.
 * Use `return success()` to conveniently return a successful result from a function.
 */
inline auto success()
{
    return boost::outcome_v2::success();
}

/**
 * Create an object that is implicitly convertible to a succesful IOResult.
 * Use `return success(t)` to conveniently return a successful result from a function.
 * @param t a value that is convertible to the value type of the IOResult
 */
template <class T>
auto success(T&& t)
{
    return boost::outcome_v2::success(std::forward<T>(t));
}

/**
 * Create an object that is implicitly convertible to an error IOResult<T>.
 * Use `return failure(s)` to conveniently return an error from a function.
 * @param s the status that contains the error.
 */
inline auto failure(const IOStatus& s)
{
    return boost::outcome_v2::failure(s);
}

/**
 * Create an object that is implicitly convertible to an error IOResult<T>.
 * Use `return failure(c, msg)` to conveniently return an error from a function.
 * @param c an error code.
 * @param msg a string that contains more information about the error.
 */
inline auto failure(std::error_code c, const std::string& msg = "")
{
    return failure(IOStatus{c, msg});
}

/**
 * Type that is used for overload resolution.
 * Use as dummy arguments to resolve overloads that would otherwise only
 * differ by return type and don't have any other arguments that allow resolution.
 * e.g.
 * ```
 * int foo(Tag<int>);
 * double foo(Tag<double>);
 * ```
 */
template <class T>
using Tag = boost::outcome_v2::in_place_type_t<T>;

//forward declaration only, see below
template <class IOContext, class T>
void serialize(IOContext& io, const T& t);

//forward declaration only, see below
template <class IOContext, class T>
IOResult<T> deserialize(IOContext& io, Tag<T> tag);

//utility for apply function implementation
namespace details
{
    //multiple nested IOResults are redundant and difficult to use.
    //so transform IOResult<IOResult<T>> into IOResult<T>.
    template <class T>
    struct FlattenIOResult {
        using Type = IOResult<T>;
    };
    template <class T>
    struct FlattenIOResult<IOResult<T>> {
        using Type = typename FlattenIOResult<T>::Type;
    };
    template <class T>
    using FlattenIOResultT = typename FlattenIOResult<T>::Type;

    //check if a type is an IOResult or something else
    template <class T>
    struct IsIOResult : std::false_type {
    };
    template <class T>
    struct IsIOResult<IOResult<T>> : std::true_type {
    };

    //if F(T...) returns an IOResult<U>, apply returns that directly
    //if F(T...) returns a different type U, apply returns IOResult<U>
    template <class F, class... T>
    using ApplyResultT = FlattenIOResultT<std::result_of_t<F(T...)>>;

    //evaluates the function f using the values of the given IOResults as arguments, assumes all IOResults are succesful
    //overload for functions that do internal validation, so return an IOResult
    template <class F, class... T, std::enable_if_t<IsIOResult<std::result_of_t<F(T...)>>::value, void*> = nullptr>
    ApplyResultT<F, T...> eval(F f, const IOResult<T>&... rs)
    {
        return f(rs.value()...);
    }
    //overload for functions that can't fail because all values are acceptable, so return some other type
    template <class F, class... T, std::enable_if_t<!IsIOResult<std::result_of_t<F(T...)>>::value, void*> = nullptr>
    ApplyResultT<F, T...> eval(F f, const IOResult<T>&... rs)
    {
        return success(f(rs.value()...));
    }
} // namespace details

/**
 * Evaluate a function with zero or more unpacked IOResults as arguments.
 * Returns an IOResult that contains the result of `f(rs.value()...)` if all IOResults `rs`
 * contain a value. If any IOResult contains an error, that error is returned instead.
 * The function f may return an object of any type U. It can also return IOResult<U> 
 * (e.g. to validate the values contained in the arguments). In either case, apply returns 
 * IOResult<U> and never any nested IOResult<IOResult<U>>. If apply returns an error, it is also
 * stored in the given IO context so that the context is informed of e.g. validation errors
 * that cannot be checked simply from the types and the format of the file.
 * @tparam IOContext a type with a set_error(const IOStatus&) member function.
 * @tparam F a type that has a function call operator with signature either `U F(T&&...)` or `IOResult<U> F(T&&...)` for any `U`.
 * @tparam T zero ore more types of values contained in the IOResult arguments.
 * @param io an IOContext that is notified of errors.
 * @param f the function that is called with the values contained in `rs` as arguments.
 * @param rs zero or more IOResults from previous operations.
 * @return the result of f(rs.value()...) if successful, the first error encountered otherwise.
 */
template <class IOContext, class F, class... T>
details::ApplyResultT<F, T...> apply(IOContext& io, F f, const IOResult<T>&... rs)
{
    //store the status of each argument in an array to check them.
    //it would be possible to write a recursive template that checks one argument
    //after the other without copying each status. This would probably be slightly faster
    //and easier to optimize for the compiler. but gdb crashes trying to resolve the
    //very long symbol in case of many arguments. Successful results are very cheap to copy
    //and slightly worse performance in the case of an error is probably acceptable.

    //check for errors in the arguments
    IOStatus status[] = {(rs ? IOStatus{} : rs.error())...};
    auto iter_err     = std::find_if(std::begin(status), std::end(status), [](auto& s) {
        return s.is_error();
    });

    //evaluate f if all succesful
    auto result =
        iter_err == std::end(status) ? details::eval(f, rs...) : details::ApplyResultT<F, T...>(failure(*iter_err));

    if (!result) {
        //save error in context
        //if the error was generated by the applied function, the context hasn't seen it yet.
        //this allows the context to fail fast and not continue to parse more.
        io.set_error(result.error());
    }
    return result;
}

//utility for (de-)serializing tuple-like objects
namespace details
{
    template <size_t Idx>
    std::string make_tuple_element_name()
    {
        return "Element" + std::to_string(Idx);
    }

    //recursive tuple serialization for each tuple element
    //store one tuple element after the other in the IOObject 
    template <size_t Idx, class IOObj, class Tup>
    std::enable_if_t<(Idx >= std::tuple_size<Tup>::value)> serialize_tuple_element(IOObj&, const Tup&)
    {
        //end of recursion, no more elements to serialize
    }
    template <size_t Idx, class IOObj, class Tup>
    std::enable_if_t<(Idx < std::tuple_size<Tup>::value)> serialize_tuple_element(IOObj& obj, const Tup& tup)
    {
        //serialize one element, then recurse
        obj.add_element(make_tuple_element_name<Idx>(), std::get<Idx>(tup));
        serialize_tuple_element<Idx + 1>(obj, tup);
    }

    //recursive tuple deserialization for each tuple element
    //read one tuple element after the other from the IOObject
    //argument pack rs contains the elements that have been read already
    template <class IOObj, class Tup, class... Ts>
    std::enable_if_t<(sizeof...(Ts) == std::tuple_size<Tup>::value), IOResult<Tup>>
    deserialize_tuple_element(IOObj& o, Tag<Tup>, const IOResult<Ts>&... rs)
    {
        //end of recursion
        //number of arguments in rs is the same as the size of the tuple
        //no more elements to read, so finalize the object
        return epi::apply(
            o,
            [](const Ts&... ts) {
                return Tup(ts...);
            },
            rs...);
    }
    template <class IOObj, class Tup, class... Ts>
    std::enable_if_t<(sizeof...(Ts) < std::tuple_size<Tup>::value), IOResult<Tup>>
    deserialize_tuple_element(IOObj& obj, Tag<Tup> tag, const IOResult<Ts>&... rs)
    {
        //get the next element of the tuple from the IO object
        const size_t Idx = sizeof...(Ts);
        auto r           = obj.expect_element(make_tuple_element_name<Idx>(), Tag<std::tuple_element_t<Idx, Tup>>{});
        //recurse, append the new element to the pack of arguments
        return deserialize_tuple_element(obj, tag, rs..., r);
    }

    //detect tuple-like types, e.g. std::tuple or std::pair
    template <class Tup>
    using tuple_size_value_t = decltype(std::tuple_size<Tup>::value);
} // namespace details

/**
 * serialize a tuple-like object, e.g. std::tuple or std::pair.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam Tup the tuple-like type to be serialized, i.e. anything that supports tuple_size and tuple_element.
 * @param io an IO context.
 * @param tup a tuple-like object to be serialized.
 */
template <class IOContext, class Tup,
          class = std::enable_if_t<is_expression_valid<details::tuple_size_value_t, Tup>::value>>
void serialize_internal(IOContext& io, const Tup& tup)
{
    auto obj = io.create_object("Tuple");
    details::serialize_tuple_element<0>(obj, tup);
}

/**
 * deserialize a tuple-like object, e.g. std::tuple or std::pair.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam Tup the tuple-like type to be deserialized, i.e. anything that supports tuple_size and tuple_element.
 * @param io an IO context.
 * @param tag define the type of the object to be deserialized.
 * @return a restored tuple
 */
template <class IOContext, class Tup,
          class = std::enable_if_t<is_expression_valid<details::tuple_size_value_t, Tup>::value>>
IOResult<Tup> deserialize_internal(IOContext& io, Tag<Tup> tag)
{
    auto obj = io.expect_object("Tuple");
    return details::deserialize_tuple_element(obj, tag);
}

/**
 * serialize an Eigen matrix expression.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam M the type of Eigen matrix expression to be deserialized.
 * @param io an IO context.
 * @param mat the matrix expression to be serialized.
 */
template <class IOContext, class M>
void serialize_internal(IOContext& io, const Eigen::EigenBase<M>& mat)
{
    auto obj = io.create_object("Matrix");
    obj.add_element("Rows", mat.rows());
    obj.add_element("Columns", mat.cols());
    obj.add_list("Elements", begin(static_cast<const M&>(mat)), end(static_cast<const M&>(mat)));
}

/**
 * deserialize an Eigen matrix.
 * It is possible to serialize an unevaluated expression, e.g. Eigen::MatrixXd::Constant(r, c, v).
 * But it is (at least currently) not possible to deserialize it. Only matrices that own their memory
 * can be deserialized. 
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam M the type of Eigen matrix expression to be deserialized.
 * @param io an IO context.
 * @param tag defines the type of the matrix to be serialized.
 */
template <class IOContext, class M, std::enable_if_t<std::is_base_of<Eigen::EigenBase<M>, M>::value, void*> = nullptr>
IOResult<M> deserialize_internal(IOContext& io, Tag<M> /*tag*/)
{
    auto obj      = io.expect_object("Matrix");
    auto rows     = obj.expect_element("Rows", Tag<Eigen::Index>{});
    auto cols     = obj.expect_element("Columns", Tag<Eigen::Index>{});
    auto elements = obj.expect_list("Elements", Tag<typename M::Scalar>{});
    return epi::apply(
        io,
        [](auto&& r, auto&& c, auto&& v) {
            auto m = M{r, c};
            for (auto i = Eigen::Index(0); i < r; ++i) {
                for (auto j = Eigen::Index(0); j < c; ++j) {
                    m(i, j) = v[i * c + j];
                }
            }
            return m;
        },
        rows, cols, elements);
}

/**
 * serialize an enum value as its underlying type.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam E an enum type to be serialized.
 * @param io an IO context
 * @param e an enum value to be serialized.
 */
template <class IOContext, class E, std::enable_if_t<std::is_enum<E>::value, void*> = nullptr>
void serialize_internal(IOContext& io, E e)
{
    epi::serialize(io, std::underlying_type_t<E>(e));
}

/**
 * deserialize an enum value from its underlying type.
 * It is impossible to validate the range of the enum type, 
 * validate after if necessary.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam E an enum type to be deserialized.
 * @param io an IO context
 * @param tag defines the type of the enum to be deserialized
 * @return an enum value if succesful, an error otherwise.
 */
template <class IOContext, class E, std::enable_if_t<std::is_enum<E>::value, void*> = nullptr>
IOResult<E> deserialize_internal(IOContext& io, Tag<E> /*tag*/)
{
    BOOST_OUTCOME_TRY(i, epi::deserialize(io, epi::Tag<std::underlying_type_t<E>>{}));
    return success(E(i));
}

//detect a serialize member function
template <class IOContext, class T>
using serialize_t = decltype(std::declval<T>().serialize(std::declval<IOContext&>()));

/**
 * serialize an object that has a serialize(io) member function.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam T the Type of the object to be serialized. Must have a serialize member function.
 * @param io an IO context
 * @param t the object to be serialized.
 */
template <class IOContext, class T,
          std::enable_if_t<is_expression_valid<serialize_t, IOContext, T>::value, void*> = nullptr>
void serialize_internal(IOContext& io, const T& t)
{
    t.serialize(io);
}

//detect a static deserialize member function
template <class IOContext, class T>
using deserialize_t = decltype(T::deserialize(std::declval<IOContext&>()));

/**
 * deserialize an object that has a deserialize(io) static member function.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam T the type of the object to be deserialized. Must have a serialize member function.
 * @param io an io context.
 * @param tag defines the type of the object for overload resolution.
 * @return the restored object if succesful, an error otherwise.
 */
template <class IOContext, class T,
          std::enable_if_t<is_expression_valid<deserialize_t, IOContext, T>::value, void*> = nullptr>
IOResult<T> deserialize_internal(IOContext& io, Tag<T> /*tag*/)
{
    return T::deserialize(io);
}

//utilities for (de-)serializing STL containers
namespace details
{
    //detect stl container.
    //don't check all requirements, since there are too many. Instead assume that if begin and end
    //iterators are available, the other requirements are met as well, as they should be in any
    //proper implementation.
    template <class C>
    using compare_iterators_t = decltype(std::declval<const C&>().begin() != std::declval<const C&>().end());
} // namespace details

/**
 * Is std::true_type if C is a STL compatible container.
 * Is std::false_type otherwise.
 * See https://en.cppreference.com/w/cpp/named_req/Container.
 * @tparam C any type.
 */
template <class C>
using is_container = is_expression_valid<details::compare_iterators_t, C>;

/**
 * serialize an STL compatible container.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam Container the container type to be serialized. A container is anything with begin and end iterators.
 * @param io an IO context.
 * @param container a container to be serialized.
 */
template <
    class IOContext, class Container,
    std::enable_if_t<(is_container<Container>::value && !is_expression_valid<serialize_t, IOContext, Container>::value),
                     void*> = nullptr>
void serialize_internal(IOContext& io, const Container& container)
{
    auto obj = io.create_object("List");
    obj.add_list("Items", container.begin(), container.end());
}

/**
 * deserialize an STL compatible container.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam Container the container type to be deserialized. A container is anything with begin and end iterators.
 * @param io an IO context.
 * @param tag defines the type of the container to be serialized for overload resolution.
 * @return restored container if successful, error otherwise.
 */
template <
    class IOContext, class Container,
    std::enable_if_t<(is_container<Container>::value && !is_expression_valid<serialize_t, IOContext, Container>::value),
                     void*> = nullptr>
IOResult<Container> deserialize_internal(IOContext& io, Tag<Container> /*tag*/)
{
    auto obj = io.expect_object("List");
    auto i   = obj.expect_list("Items", Tag<typename Container::value_type>{});
    return apply(
        io,
        [](auto&& i_) {
            return Container(i_.begin(), i_.end());
        },
        i);
}

/**
 * Save data that describes an object in a format determined by the given context.
 * There must be provided for the type T either a free function `serialize_internal(io, t)`
 * that can be found using argument dependent lookup (ADL) or a member function `t.serialize(io)`.
 * The `serialize_internal` function or `serialize` member function provide the data that describes 
 * the object to the io context. The context stores the data in some unspecified format so that the 
 * objects can be reconstructed from it. The context also keeps track of any IO errors.
 * `serialize_internal` overloads are already provided for many common types, e.g. STL containers 
 * or Eigen Matrices.
 * `serialize` and `deserialize` are the main entry point into this IO framework,
 * but there may be more convenient functions provided for specific IO contexts.
 * These functions are not expected to use ADL, so should be called namespace qualified.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam T any serializable type, i.e., that has a `serialize` member function or `serialize_internal` overload
 * @param io io context that stores data from the object t in some unspecified format.
 * @param t the object to be stored.
 */
template <class IOContext, class T>
void serialize(IOContext& io, const T& t)
{
    using epi::serialize_internal;
    serialize_internal(io, t);
}

/**
 * Restores an object from the data stored in an IO context.
 * There must be provided for the type T either a free function `deserialize_internal(io, tag)`
 * that can be found using argument dependent lookup (ADL) or a static member function `T::deserialize(io)`.
 * The `deserialize_internal` function or `deserialize` member function retrieve the data needed to
 * restore the object from the IO context. The context provides the data if it can and keeps track of errors.
 * `deserialize_internal` overloads are already provided for many common types, e.g. STL containers 
 * or Eigen Matrices. 
 * `serialize` and `deserialize` are the main entry points into this IO framework,
 * but there may be more convenient functions provided for specific IO contexts.
 * These functions are not expected to use ADL, so should be called namespace qualified.
 * @tparam IOContext a type that models the IOContext concept.
 * @tparam T any deserializable type, i.e., that has a `deserialize` member function or `deserialize_internal` overload
 * @param io IO context that contains the data for an object of type T
 * @param tag specifies the type to be restored from the data, for overload resolution only.
 * @return the restored T if succesful, an error code otherwise.
 */
template <class IOContext, class T>
IOResult<T> deserialize(IOContext& io, Tag<T> tag)
{
    using epi::deserialize_internal;
    return deserialize_internal(io, tag);
}

} // namespace epi

#endif