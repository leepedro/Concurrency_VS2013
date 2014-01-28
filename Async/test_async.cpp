#include <iostream>
#include <vector>
//#include <algorithm>
#include <numeric>	// std::accumulate
#include <future>

#include "tbb/tick_count.h"

// This is a bad implementation because the number of thread increase exponentially.
template <typename Iterator>
typename std::iterator_traits<Iterator>::value_type
parallel_sum_bad(Iterator begin, Iterator end)
{
	typename Iterator::difference_type length = end - begin;
	if (length < 1000)
		return std::accumulate(begin, end, 0);
	Iterator mid = begin + length / 2;
	auto handle = std::async(std::launch::async, parallel_sum<Iterator>, mid, end);
	auto sum = parallel_sum(begin, mid);
	return sum + handle.get();
}

template <typename Iterator>
typename std::iterator_traits<Iterator>::value_type
dual_sum(Iterator begin, Iterator end)
{
	typename Iterator::difference_type length = end - begin;
	Iterator mid = begin + length / 2;
	auto sum = std::accumulate(begin, mid, 0);
	auto handle = std::async(std::launch::async,
		std::accumulate<Iterator, typename std::iterator_traits<Iterator>::value_type>,
		mid, end, 0);
	return sum + handle.get();
}

// The performance is much better with following implementation.
template <typename Iterator>
typename std::iterator_traits<Iterator>::value_type
quad_sum(Iterator begin, Iterator end)
{
	typename Iterator::difference_type length = end - begin;
	Iterator pt1 = begin + length / 4;
	Iterator pt2 = pt1 + length / 4;
	Iterator pt3 = pt2 + length / 4;
	auto sum1 = std::accumulate(begin, pt1, 0);
	auto handle1 = std::async(std::launch::async,
		std::accumulate<Iterator, typename std::iterator_traits<Iterator>::value_type>,
		pt1, pt2, 0);
	auto handle2 = std::async(std::launch::async,
		std::accumulate<Iterator, typename std::iterator_traits<Iterator>::value_type>,
		pt2, pt3, 0);
	auto handle3 = std::async(std::launch::async,
		std::accumulate<Iterator, typename std::iterator_traits<Iterator>::value_type>,
		pt3, end, 0);
	return sum1 + handle1.get() + handle2.get() + handle3.get();
}

void test_async(void)
{
	//std::vector<unsigned int> v(1000000000, 1);	// 4 GBytes
	std::vector<unsigned short> v(2000000000, 0);	// 4 GBytes

	tbb::tick_count t0 = tbb::tick_count::now();
	auto sum = std::accumulate(v.cbegin(), v.cend(), 0);
	tbb::tick_count t1 = tbb::tick_count::now();
	std::cout << "serial sum == " << sum << " took " << (t1 - t0).seconds() << " seconds."
		<< std::endl;

	t0 = tbb::tick_count::now();
	sum = quad_sum(v.cbegin(), v.cend());
	t1 = tbb::tick_count::now();
	std::cout << "quad sum = " << sum << " took " << (t1 - t0).seconds() << " seconds." <<
		std::endl;

	// NOTE: Following impelmentation is actually slower than the sequential implementation.
	//t0 = tbb::tick_count::now();
	//sum = parallel_sum_bad(v.cbegin(), v.cend());
	//t1 = tbb::tick_count::now();
	//std::cout << "parallel sum = " << sum << " took " << (t1 - t0).seconds() << " seconds." << std::endl;
}

void test_wait_for(void)
{
	auto future = std::async(std::launch::async, [](){ std::this_thread::sleep_for(std::chrono::seconds(3)); return 8; });
	std::cout << "waiting" << std::endl;
	std::future_status status;
	do
	{
		status = future.wait_for(std::chrono::seconds(1));
		switch (status)
		{
		case std::future_status::deferred:
			std::cout << "deferred" << std::endl;
			break;
		case std::future_status::timeout:
			std::cout << "time out" << std::endl;
			break;
		case std::future_status::ready:
			std::cout << "ready" << std::endl;
			break;
		}
	} while (status != std::future_status::ready);
	std::cout << "result is " << future.get() << std::endl;
}

void test_wait_until(void)
{
	auto future = std::async(std::launch::async, [](){ std::this_thread::sleep_for(std::chrono::seconds(3)); return 8; });
	std::cout << "waiting" << std::endl;
	std::future_status status;
	do
	{
		const auto wakeup_time = std::chrono::system_clock::now() + std::chrono::seconds(1);
		status = future.wait_until(wakeup_time);
		switch (status)
		{
		case std::future_status::deferred:
			std::cout << "deferred" << std::endl;
			break;
		case std::future_status::timeout:
			std::cout << "time out" << std::endl;
			break;
		case std::future_status::ready:
			std::cout << "ready" << std::endl;
			break;
		}
	} while (status != std::future_status::ready);
	std::cout << "result is " << future.get() << std::endl;
}

int main(void)
{
	//test_async();
	//test_wait_for();
	test_wait_until();
}