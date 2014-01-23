#include <mutex>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <atomic>

std::mutex mutex_for_cout;

template <typename T>
class BoundedBuffer
{
public:
	BoundedBuffer(std::size_t capacity) : capacity_(capacity) { this->buffer_ = new T[capacity_]; }
	~BoundedBuffer(void) { delete[] this->buffer_; }
	
	void push(T data);
	bool try_pop(T &result, const std::chrono::seconds &wait_time = std::chrono::seconds(3));

protected:
	T *buffer_;
	std::size_t capacity_ = 0;

	std::atomic_size_t front_ = 0;
	std::atomic_size_t rear_ = 0;
	std::atomic_size_t count_ = 0;

	std::condition_variable not_full_;
	std::condition_variable not_empty_;
	std::mutex mutex_;
};

template <typename T>
void BoundedBuffer<T>::push(T data)
{
	std::unique_lock<std::mutex> lock(this->mutex_);
	this->not_full_.wait(lock, [this](){ return this->count_.load() != this->capacity_; });
	this->buffer_[this->rear_.load()] = data;
	this->rear_ = (this->rear_.load() + 1) % this->capacity_;
	++this->count_;
	this->not_empty_.notify_one();
}

// Try to pop during the wait time, and give up if buffer is still empty at the end of the wait time.
template <typename T>
bool BoundedBuffer<T>::try_pop(T &result, const std::chrono::seconds &wait_time)
{
	std::unique_lock<std::mutex> lock(this->mutex_);
	//this->not_empty_.wait(lock, [this](){ return this->count_.load() != 0; });
	auto now = std::chrono::system_clock::now();
	if (this->not_empty_.wait_until(lock, now + wait_time, [this](){ return this->count_.load() != 0; }))
	{
		result = this->buffer_[this->front_.load()];
		this->front_ = (this->front_.load() + 1) % this->capacity_;
		--this->count_;
		this->not_full_.notify_one();
		return true;
	}
	else
		return false;	// timed out.
}

template <typename T>
void Consumer(int id, BoundedBuffer<T> &buffer, std::size_t count)
{
	for (auto n = 0; n != count; ++n)
	{
		T value;
		if (buffer.try_pop(value))
		{
			std::lock_guard<std::mutex> lock(::mutex_for_cout);
			std::cout << "Consumer " << id << " fetched " << value << std::endl;
		}
		// If failed to pop, give it up for this iteration.
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

template <typename T>
void Producer(int id, BoundedBuffer<T> &buffer, T init_value, T count)
{
	for (T value = init_value, value_end = init_value + count; value != value_end; ++value)
	{
		buffer.push(value);
		{
			std::lock_guard<std::mutex> lock(::mutex_for_cout);
			std::cout << "Producer " << id << " pushed " << value << std::endl;
		}		
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

int main(void)
{
	BoundedBuffer<char> buffer(40);

	std::thread c1(Consumer<char>, 1, std::ref(buffer), 22);
	std::thread c2(Consumer<char>, 2, std::ref(buffer), 15);
	std::thread c3(Consumer<char>, 3, std::ref(buffer), 16);
	std::thread p1(Producer<char>, 1, std::ref(buffer), 'a', static_cast<char>(26));
	std::thread p2(Producer<char>, 2, std::ref(buffer), 'A', static_cast<char>(26));

	c1.join();
	c2.join();
	c3.join();
	p1.join();
	p2.join();

	return 0;
}