#include <atomic>
#include <limits>
#include <thread>
#include <iostream>

std::atomic<bool> Stop = false;

template <typename T>
class AtomicCounter
{
public:
	//AtomicCounter(void) = default;
	AtomicCounter(T value) : value_(value) {}

	void Increment() { this->value_++; }
	void Decrement() { this->value_--; }
	int Get() { return this->value_.load(); }
	void Set(T value) { this->value_ = value; }

protected:
	std::atomic<T> value_;
};

template <typename T>
void IncreaseCounter(AtomicCounter<T> &counter)
{
	while (!::Stop.load())
	{
		if (counter.Get() == std::numeric_limits<T>::max())
		{
			std::cout << std::this_thread::get_id() << " has reached the maximum." << std::endl;
			counter.Set(std::numeric_limits<T>::min());
		}
		else
			counter.Increment();
	}
}

void WaitFor(std::chrono::seconds t)
{
	std::this_thread::sleep_for(t);
	::Stop = true;
}

int main(void)
{
	AtomicCounter<unsigned short> counter(0);
	::Stop = false;
	std::thread th1(IncreaseCounter<unsigned short>, std::ref(counter));
	std::thread th2(IncreaseCounter<unsigned short>, std::ref(counter));
	std::thread th3(IncreaseCounter<unsigned short>, std::ref(counter));
	std::thread th4(WaitFor, std::chrono::seconds(1));
	th1.join();
	th2.join();
	th3.join();
	th4.join();
	std::cout << "Counter = " << counter.Get() << std::endl;
}