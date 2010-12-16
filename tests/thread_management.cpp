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

#include "../src/thread_management.hpp"

#define BOOST_TEST_MODULE thread_management_test
#include <boost/test/unit_test.hpp>

#include <mutex>
#include <thread>
#include <utility>

#include "../src/runtime_info.hpp"
#include "../src/thread_group.hpp"

std::mutex mutex;

static inline void test_function()
{
	for (unsigned i = 0; i < piranha::runtime_info::hardware_concurrency(); ++i) {
		piranha::thread_management::bind_to_proc(i);
		const std::pair<bool,unsigned> retval = piranha::thread_management::bound_proc();
		// Lock because Boost unit test is not thread-safe.
		std::lock_guard<std::mutex> lock(mutex);
		BOOST_CHECK_EQUAL(retval.first,true);
		BOOST_CHECK_EQUAL(retval.second,i);
	}
}

// Check binding on main thread.
BOOST_AUTO_TEST_CASE(thread_management_main_thread_bind)
{
	test_function();
}

// Check binding on new threads thread.
BOOST_AUTO_TEST_CASE(thread_management_new_threads_bind)
{
	for (unsigned i = 0; i < piranha::runtime_info::hardware_concurrency(); ++i) {
		std::thread t(test_function);
		t.join();
	}
}

// Check thread-safe binding using a thread group.
BOOST_AUTO_TEST_CASE(thread_management_thread_group_bind)
{
	piranha::thread_group tg;
	for (unsigned i = 0; i < piranha::runtime_info::hardware_concurrency(); ++i) {
		tg.create_thread(test_function);
	}
	tg.join_all();
}
