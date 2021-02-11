#ifndef CUSTOMINDEXARRAY_H
#define CUSTOMINDEXARRAY_H

#include <epidemiology/utils/uncertain_value.h>
#include "epidemiology/utils/tensor_helpers.h"
#include "epidemiology/utils/ScalarType.h"
#include "epidemiology/utils/eigen.h"

#include <vector>
#include <array>
#include <numeric>

namespace
{

// Some metaprogramming to get the index of a given type in a parameter pack.
// Taken from https://stackoverflow.com/questions/26169198/how-to-get-the-index-of-a-type-in-a-variadic-type-pack
template <typename T, typename... Ts>
struct Index;

template <typename T, typename... Ts>
struct Index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {
};

template <typename T, typename U, typename... Ts>
struct Index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + Index<T, Ts...>::value> {
};

template <typename T, typename... Ts>
constexpr std::size_t Index_v = Index<T, Ts...>::value;

//calculate the product of a size_t parameter pack
template <size_t...>
struct product;

template <>
struct product<> {
    static constexpr size_t value = 1;
};

template <size_t i, size_t... tail>
struct product<i, tail...> {
    static constexpr size_t value = i * product<tail...>::value;
};

} // namespace

namespace epi
{

/**
 * @brief A class template for an array with custom indices
 *
 * This class stores an array of elements that can be queried using
 * a variadic number of custom index types. Each index type is associated
 * with a category, or dimension into a multidimensional array.
 *
 * Each custom index type must have a nested Count element that is castable
 * to an integer. Typically it is assumed, that custom indices are enums.
 *
 * Example:
 *
 * enum class AgeGroup
 * {
 *    Young,
 *    Old,
 *    Count = 2
 * };
 *
 * enum class Gender
 * {
 *    Female,
 *    Male,
 *    Diverse,
 *    Count = 3
 * };
 *
 * CustomIndexArray<size_t, AgeGroup, Gender> populations;
 *
 * Here, populations represents a 2x3 size_t array (though the data is stored contigously).
 * An element can be accessed using a flat row-major index or by using categories:
 *
 * auto x = populations.get(4);
 * auto y = populations.get(AgeGroup::Old, Gender::Male);
 * assert(x == y);
 *
 * @tparam Typ the type stored in the array
 * @tparam Categories The custom Index types (enums)
 *
 */

template <class Typ, class... Categories>
class CustomIndexArray
{
public:

    using Type = Typ;

    // This type can be used by other classes to refer to a concrete compartment
    using Index = std::tuple<Categories...>;

    /**
     * @brief Populations default constructor
     */
    CustomIndexArray() = default;

    /**
     * @brief get_num_compartments returns the number of compartments
     *
     * This corresponds to the product of the category sizes
     *
     * @return number of compartments
     */
    static size_t constexpr size()
    {
        return siz;
    }

    /**
     * @brief get_compartments returns an Eigen copy of the vector of populations. This can be used
     * as initial conditions for the ODE solver
     * @return Eigen::VectorXd  of populations
     */
    auto const& get_array() const
    {
        return m_y;
    }

    /**
     * @brief returns the entry of the array given a flat index index
     * @param index a flat index
     * @return the value at the index
     */
    Type& operator[](size_t index) {
        return m_y[index];
    }

    /**
     * @brief returns the entry of the array given a flat index index
     * @param index a flat index
     * @return the value at the index
     */
    Type const& operator[](size_t index) const {
        return m_y[index];
    }

    /**
     * @brief get returns the element given a custom index
     * @param cats the custsom indices for each category
     * @return the value at the index
     */
    Type& get(Categories... cats)
    {
        return m_y[get_flat_index(cats...)];
    }

    /**
     * @brief get returns the element given a custom index
     * @param cats the custom indices for each category
     * @return the value at the index
     */
    Type const& get(Categories... cats) const
    {
        return m_y[get_flat_index(cats...)];
    }

    /**
     * @brief set sets the scalar value of the element given a custom index
     * @param indices the custom indices for each category
     * @param value the new value
     */
    void set(Type const& value, Categories... cats)
    {
        m_y[get_flat_index(cats...)] = value;
    }

    /**
     * @brief set sets the scalar value of the element given a custom index
     * @param indices the custom indices for each category
     * @param value the new value
     */
    void set(ScalarType value, Categories... cats)
    {
        m_y[get_flat_index(cats...)] = value;
    }


    /**
     * @brief get_flat_index returns the flat index into the stored array, given the
     * indices of each category
     * @param indices the custom indices for each category
     * @return a flat index into the data structure storing the compartment populations
     */
    static size_t get_flat_index(Categories... cats)
    {
        return flatten_index({static_cast<size_t>(cats)...}, dimensions);
    }


    // An array storying the size of each category
    static std::array<size_t, sizeof...(Categories)> dimensions;
    static size_t constexpr siz = product<static_cast<size_t>(Categories::Count)...>::value;

protected:
    // An array containing the elements
    Eigen::Matrix<Type, siz, 1> m_y{};
};

// initialize array storying the size of each category
template <class Type, class... Categories>
std::array<size_t, sizeof...(Categories)> CustomIndexArray<Type, Categories...>::dimensions = {
    static_cast<size_t>(Categories::Count)...};

} // namespace epi

#endif // CUSTOMINDEXARRAY_H
