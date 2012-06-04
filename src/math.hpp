/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef PIRANHA_MATH_HPP
#define PIRANHA_MATH_HPP

#include <boost/numeric/conversion/cast.hpp>
#include <boost/type_traits/is_complex.hpp>
#include <cmath>
#include <type_traits>

#include "config.hpp"
#include "detail/integer_fwd.hpp"

namespace piranha
{

namespace detail
{

// Default implementation of math::is_zero.
// NOTE: the technique and its motivations are well-described here:
// http://www.gotw.ca/publications/mill17.htm
template <typename T, typename Enable = void>
struct math_is_zero_impl
{
	static bool run(const T &x)
	{
		// NOTE: construct instance from integral constant 0.
		// http://groups.google.com/group/comp.lang.c++.moderated/msg/328440a86dae8088?dmode=source
		return x == T(0);
	}
};

// Handle std::complex types.
template <typename T>
struct math_is_zero_impl<T,typename std::enable_if<boost::is_complex<T>::value>::type>
{
	static bool run(const T &c)
	{
		return math_is_zero_impl<typename T::value_type>::run(c.real()) &&
			math_is_zero_impl<typename T::value_type>::run(c.imag());
	}
};

// Default implementation of math::negate.
template <typename T, typename Enable = void>
struct math_negate_impl
{
	static void run(T &x)
	{
		x = -x;
	}
};

// Default implementation of multiply-accumulate.
template <typename T, typename U, typename V, typename Enable = void>
struct math_multiply_accumulate_impl
{
	template <typename U2, typename V2>
	static void run(T &x, U2 &&y, V2 &&z)
	{
		x += std::forward<U2>(y) * std::forward<V2>(z);
	}
};

#if defined(FP_FAST_FMA) && defined(FP_FAST_FMAF) && defined(FP_FAST_FMAL)

// Implementation of multiply-accumulate for floating-point types, if fast FMA is available.
template <typename T>
struct math_multiply_accumulate_impl<T,T,T,typename std::enable_if<std::is_floating_point<T>::value>::type>
{
	static void run(T &x, const T &y, const T &z)
	{
		x = std::fma(y,z,x);
	}
};

#endif

// Implementation of exponentiation with floating-point base.
template <typename T, typename U>
inline auto float_pow_impl(const T &x, const U &n, typename std::enable_if<
	std::is_integral<U>::value>::type * = piranha_nullptr) -> decltype(std::pow(x,boost::numeric_cast<int>(n)))
{
	return std::pow(x,boost::numeric_cast<int>(n));
}

template <typename T, typename U>
inline auto float_pow_impl(const T &x, const U &n, typename std::enable_if<
	std::is_same<integer,U>::value>::type * = piranha_nullptr) -> decltype(std::pow(x,static_cast<int>(n)))
{
	return std::pow(x,static_cast<int>(n));
}

template <typename T, typename U>
inline auto float_pow_impl(const T &x, const U &y, typename std::enable_if<
	std::is_floating_point<U>::value>::type * = piranha_nullptr) -> decltype(std::pow(x,y))
{
	return std::pow(x,y);
}

}

/// Math namespace.
/**
 * Namespace for general-purpose mathematical functions.
 */
namespace math
{

/// Zero test.
/**
 * Test if value is zero. This function works with all C++ arithmetic types
 * and with piranha's numerical types. For series types, it will return \p true
 * if the series is empty, \p false otherwise. For \p std::complex, the function will
 * return \p true if both the real and imaginary parts are zero, \p false otherwise.
 * 
 * @param[in] x value to be tested.
 * 
 * @return \p true if value is zero, \p false otherwise.
 */
template <typename T>
inline bool is_zero(const T &x)
{
	return piranha::detail::math_is_zero_impl<T>::run(x);
}

/// In-place negation.
/**
 * Negate value in-place. This function works with all C++ arithmetic types,
 * with piranha's numerical types and with series types. For series, piranha::series::negate() is called.
 * 
 * @param[in,out] x value to be negated.
 */
template <typename T>
inline void negate(T &x)
{
	piranha::detail::math_negate_impl<typename std::decay<T>::type>::run(x);
}

/// Multiply-accumulate.
/**
 * Will set \p x to <tt>x + y * z</tt>. Default implementation is equivalent to:
 * @code
 * x += y * z
 * @endcode
 * where \p y and \p z are perfectly forwarded.
 * 
 * In case fast fma functions are available on the host platform, they will be used instead of the above
 * formula for floating-point types.
 * 
 * @param[in,out] x value used for accumulation.
 * @param[in] y first multiplicand.
 * @param[in] z second multiplicand.
 */
template <typename T, typename U, typename V>
inline void multiply_accumulate(T &x, U &&y, V &&z)
{
	piranha::detail::math_multiply_accumulate_impl<typename std::decay<T>::type,
		typename std::decay<U>::type,typename std::decay<V>::type>::run(x,std::forward<U>(y),std::forward<V>(z));
}

/// Default functor for the implementation of piranha::math::pow().
/**
 * This functor should be specialised via the \p std::enable_if mechanism. Default implementation will not define
 * the call operator, and will hence result in a compilation error when used.
 */
template <typename T, typename U, typename Enable = void>
struct pow_impl
{};

/// Specialisation of the piranha::math::pow() functor for floating-point bases.
/**
 * This specialisation is activated when \p T is a floating-point type and \p U is either a floating-point type,
 * an integral type or piranha::integer. The result will be computed via the standard <tt>std::pow()</tt> function.
 */
template <typename T, typename U>
struct pow_impl<T,U,typename std::enable_if<std::is_floating_point<T>::value &&
	(std::is_floating_point<U>::value || std::is_integral<U>::value || std::is_same<U,integer>::value)>::type>
{
	/// Call operator.
	/**
	 * The exponentiation will be computed via <tt>std::pow()</tt>. In case \p U2 is an integral type or piranha::integer,
	 * \p y will be converted to \p int via <tt>boost::numeric_cast()</tt> or <tt>static_cast()</tt>.
	 * 
	 * @param[in] x base.
	 * @param[in] y exponent.
	 * 
	 * @return \p x to the power of \p y.
	 * 
	 * @throws unspecified any exception resulting from numerical conversion failures in <tt>boost::numeric_cast()</tt> or <tt>static_cast()</tt>.
	 */
	auto operator()(const T &x, const U &y) const -> decltype(detail::float_pow_impl(x,y))
	{
		return detail::float_pow_impl(x,y);
	}
};

/// Exponentiation.
/**
 * Return \p x to the power of \p y. The actual implementation of this function is in the piranha::math::pow_impl functor's
 * call operator.
 * 
 * @param[in] x base.
 * @param[in] y exponent.
 * 
 * @return \p x to the power of \p y.
 * 
 * @throws unspecified any exception thrown by the call operator of the piranha::math::pow_impl functor.
 */
// NOTE: here the use of trailing decltype gives a nice and compact compilation error in case the specialisation of the functor
// is missing.
template <typename T, typename U>
inline auto pow(T &&x, U &&y) -> decltype(pow_impl<typename std::decay<T>::type,typename std::decay<U>::type>()(std::forward<T>(x),std::forward<U>(y)))
{
	return pow_impl<typename std::decay<T>::type,typename std::decay<U>::type>()(std::forward<T>(x),std::forward<U>(y));
}

}

}

#endif
