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

#ifndef PIRANHA_SUBSTITUTABLE_SERIES_HPP
#define PIRANHA_SUBSTITUTABLE_SERIES_HPP

#include <string>
#include <type_traits>
#include <utility>

#include "forwarding.hpp"
#include "math.hpp"
#include "serialization.hpp"
#include "series.hpp"
#include "symbol_set.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

struct substitutable_series_tag {};

}

template <typename Series, typename Derived>
class substitutable_series: public Series, detail::substitutable_series_tag
{
		typedef Series base;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
		// Detect subs term.
		template <typename Term, typename T>
		struct subs_term_score
		{
			static const unsigned value = static_cast<unsigned>(has_subs<typename Term::cf_type,T>::value) +
				(static_cast<unsigned>(key_has_subs<typename Term::key_type,T>::value) << 1u);
		};
		// Case 1: subs only on cf.
		template <typename T, typename Term>
		using ret_type_1 = decltype(math::subs(std::declval<typename Term::cf_type const &>(),std::declval<std::string const &>(), \
				std::declval<T const &>()) * std::declval<Derived const &>());
		template <typename T, typename Term, typename std::enable_if<subs_term_score<Term,T>::value == 1u,int>::type = 0>
		static ret_type_1<T,Term> subs_impl(const Term &t, const std::string &name, const T &x, const symbol_set &s_set)
		{
			Derived tmp;
			tmp.set_symbol_set(s_set);
			tmp.insert(Term(typename Term::cf_type(1),t.m_key));
			return math::subs(t.m_cf,name,x) * tmp;
		}
		// Case 2: subs only on key.
		template <typename T, typename Term>
		using k_subs_type = typename decltype(std::declval<const typename Term::key_type &>().subs(std::declval<const std::string &>(),
			std::declval<const T &>(),std::declval<const symbol_set &>()))::value_type::first_type;
		template <typename T, typename Term>
		using ret_type_2_ = decltype(std::declval<Derived const &>() * std::declval<const k_subs_type<T,Term> &>());
		template <typename T, typename Term>
		using ret_type_2 = typename std::enable_if<is_addable_in_place<ret_type_2_<T,Term>>::value,ret_type_2_<T,Term>>::type;
		template <typename T, typename Term, typename std::enable_if<subs_term_score<Term,T>::value == 2u,int>::type = 0>
		static ret_type_2<T,Term> subs_impl(const Term &t, const std::string &name, const T &x, const symbol_set &s_set)
		{
			ret_type_2<T,Term> retval;
			retval.set_symbol_set(s_set);
			auto ksubs = t.m_key.subs(name,x,s_set);
			for (auto &p: ksubs) {
				Derived tmp;
				tmp.set_symbol_set(s_set);
				tmp.insert(Term{t.m_cf,std::move(p.second)});
				// NOTE: possible use of multadd here in the future.
				retval += tmp * p.first;
			}
			return retval;
		}
		// Initial definition of the subs type.
		template <typename T>
		using subs_type_ = decltype(subs_impl(std::declval<typename Series::term_type const &>(),std::declval<std::string const &>(),
			std::declval<const T &>(),std::declval<symbol_set const &>()));
		// Enable conditionally based on the common requirements in the subs() method.
		template <typename T>
		using subs_type = typename std::enable_if<std::is_constructible<subs_type_<T>,int>::value && is_addable_in_place<subs_type_<T>>::value,
			subs_type_<T>>::type;
	public:
		/// Defaulted default constructor.
		substitutable_series() = default;
		/// Defaulted copy constructor.
		substitutable_series(const substitutable_series &) = default;
		/// Defaulted move constructor.
		substitutable_series(substitutable_series &&) = default;
		PIRANHA_FORWARDING_CTOR(substitutable_series,base)
		/// Defaulted copy assignment operator.
		substitutable_series &operator=(const substitutable_series &) = default;
		/// Defaulted move assignment operator.
		substitutable_series &operator=(substitutable_series &&) = default;
		/// Trivial destructor.
		~substitutable_series()
		{
			PIRANHA_TT_CHECK(is_series,substitutable_series);
			PIRANHA_TT_CHECK(is_series,Derived);
			PIRANHA_TT_CHECK(std::is_base_of,substitutable_series,Derived);
		}
		PIRANHA_FORWARDING_ASSIGNMENT(substitutable_series,base)
		template <typename T>
		subs_type<T> subs(const std::string &name, const T &x) const
		{
			subs_type<T> retval(0);
			for (const auto &t: this->m_container) {
				retval += subs_impl(t,name,x,this->m_symbol_set);
			}
			return retval;
		}
};

namespace detail
{

// Enabler for the specialisation of subs functor for subs series.
template <typename Series, typename T>
using subs_impl_subs_series_enabler = typename std::enable_if<
	std::is_base_of<substitutable_series_tag,Series>::value &&
	true_tt<decltype(std::declval<const Series &>().subs(std::declval<const std::string &>(),std::declval<const T &>()))>::value
>::type;

}

namespace math
{

template <typename Series, typename T>
struct subs_impl<Series,T,detail::subs_impl_subs_series_enabler<Series,T>>
{
	auto operator()(const Series &s, const std::string &name, const T &x) const -> decltype(s.subs(name,x))
	{
		return s.subs(name,x);
	}
};

}

}

#endif
