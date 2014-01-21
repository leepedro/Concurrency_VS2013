#include <mutex>
#include <thread>
#include <condition_variable>
#include <iostream>

template <typename T>
class BoundedBuffer
{
public:
	BoundedBuffer(std::size_t capacity) : capacity_(capacity) { this->buffer_ = new T[capacity_]; }
	~BoundedBuffer(void) { delete[] this->buffer_; }
	void push(T data);
	T pop(void);

protected:
	T *buffer_;
	std::size_t capacity_ = 0;
	std::size_t front_ = 0;
	std::size_t rear_ = 0;
	std::size_t count_ = 0;
	std::condition_variable not_full_;
	std::condition_variable not_empty_;
	std::mutex lock_;
};

template <typename T>
void BoundedBuffer<T>::push(T data)
{
	std::unique_lock<std::mutex> l(this->lock_);
	this->not_full_.wait(l, [this](){ return this->count_ != this->capacity_; });
	this->buffer_[this->rear_] = data;
	this->rear_ = (this->rear_ + 1) % this->capacity_;
	++this->count_;
	this->not_empty_.notify_one();
}

template <typename T>
T BoundedBuffer<T>::pop(void)
{
	std::unique_lock<std::mutex> l(this->lock_);
	this->not_empty_.wait(l, [this](){ return this->count_ != 0; });
	T result = this->buffer_[this->front_];
	this->front_ = (this->front_ + 1) % this->capacity_;
	--this->count_;
	this->not_full_.notify_one();
	return result;
}

template <typename T>
void Consumer(int id, BoundedBuffer<T> &buffer, std::size_t count)
{
	for (auto n = 0; n != count; ++n)
	{
		T value = buffer.pop();
		std::cout << "Consumer " << id << " fetched " << value << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

template <typename T>
void Producer(int id, BoundedBuffer<T> &buffer, T init_value, T count)
{
	for (T value = init_value, value_end = init_value + count; value != value_end; ++value)
	{
		buffer.push(value);
		std::cout << "Producer " << id << " pushed " << value << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

int main(void)
{
	BoundedBuffer<char> buffer(40);

	std::thread c1(Consumer<char>, 1, std::ref(buffer), 22);
	std::thread c2(Consumer<char>, 2, std::ref(buffer), 15);
	std::thread c3(Consumer<char>, 3, std::ref(buffer), 15);
	std::thread p1(Producer<char>, 1, std::ref(buffer), 'a', static_cast<char>(26));
	std::thread p2(Producer<char>, 2, std::ref(buffer), 'A', static_cast<char>(26));

	c1.join();
	c2.join();
	c3.join();
	p1.join();
	p2.join();

	return 0;
}