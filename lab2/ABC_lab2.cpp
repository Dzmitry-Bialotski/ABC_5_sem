#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include<queue>
#include<vector>
#include<condition_variable>
using namespace std;

const int NumTasks = 1024 * 1024;
const int NumThreads[] = { 4, 8, 16, 32 };
mutex mtx;
atomic<int> atomic_ptr{0};

int pointer = 0;
void thread_proc_using_mutex(int* Arr, int sleep)
{
	int local_pointer;
	while (true)
	{
		mtx.lock();
		local_pointer = pointer++;
		mtx.unlock();
		if (local_pointer < NumTasks)
		{
			++Arr[local_pointer];
			if (sleep)
				this_thread::sleep_for(chrono::nanoseconds(sleep));
		}
		else break;
	}
	return;
}

int getAtomicPtrValue(int sleep)
{
	int pointer = ++atomic_ptr;
	if (sleep)
	{
		this_thread::sleep_for(chrono::nanoseconds(sleep));
	}
	return pointer;
}
void thread_proc_using_atomic(int* Arr, int sleep)
{
	int pointer = getAtomicPtrValue(sleep);
	while (pointer < NumTasks)
	{
		Arr[pointer] ++;
		pointer = getAtomicPtrValue(sleep);
	}
	return;
}
string ResultSucceded(int * Arr)
{
	for (int i = 0; i < NumTasks; i++)
	{
		if (Arr[i] != 1)
			return "WRONG";
	}
	return "OK";
}
void subtask_1(int* Arr, void(*func)(int*, int), int sleep = 0)
{
	for (int k = 0; k < 4; k++)
	{
		cout << "Using " << NumThreads[k] << " Threads: \t";
		pointer = 0;
		atomic_ptr = -1;
		for (int i = 0; i < NumTasks; i++)
		{
			Arr[i] = 0;
		}
		auto start = chrono::high_resolution_clock::now();
		thread* threads = new thread[NumThreads[k]];
		for (int i = 0; i < NumThreads[k]; i++) {
			threads[i] = thread(func, Arr, sleep);
		}
		for (int i = 0;i < NumThreads[k]; i++)
		{
			threads[i].join();
		}

		auto end = chrono::high_resolution_clock::now();

		chrono::duration<float> duration = end - start;
		cout << "Time: " << duration.count() << "\t\tResult: " << ResultSucceded(Arr) << endl;
	}
}
void task_1(int* Arr)
{
	std::cout << "==============================Task1==============================" << endl;

	mutex mtx;
	//==========
	std::cout << "====================Using mutex==================== " << endl;
	subtask_1(Arr, thread_proc_using_mutex);
	std::cout << "====================Using mutex(sleep = 10ns)==================== " << endl;
	subtask_1(Arr, thread_proc_using_mutex, 10);
	std::cout << "====================Using atomic==================== " << endl;
	subtask_1(Arr, thread_proc_using_atomic);
	std::cout << "====================Using atomic(sleep = 10ns)==================== " << endl;
	subtask_1(Arr, thread_proc_using_atomic, 10); 
	return;
}
///////////////// task 2 //////////////////////////////////////////////////////////
const int ProducerNum[] = { 1,2,4 };
const int ConsumerNum[] = { 1,2,4 };
const int QueueSize[] = { 1, 4, 16 };
const int TaskNum = 1024 * 1024;// 4 * 1024 * 1024

class IQueue
{
protected:
	size_t size = 0;
	mutex mtx;
	queue<int8_t> q;
	condition_variable cond;
public:
	int active_producers_count = 0;
	int ResultSum = 0;
	virtual void push(int8_t val) = 0;
	virtual bool pop(int8_t& val) = 0;
	virtual bool empty() = 0;
};
class DynamicSizeQueue : public IQueue
{
public:
	DynamicSizeQueue(){}
	void push(int8_t val) override
	{
		lock_guard<mutex> lock(mtx);
		q.push(val);
	}
	bool pop(int8_t& val) override
	{
		lock_guard<mutex> lock(mtx);
		if (this->empty())
		{
			return false;
		}
		else
		{
			val = q.front();
			q.pop();
			return true;
		}
	}
	bool empty() override {
		//std::lock_guard<std::mutex> lock(mtx);
		return q.empty();
	}
};
class FixedSizeQueue : public IQueue
{
	public:
	FixedSizeQueue(size_t s, int count)
	{
		size = s;
		active_producers_count = count;
	}
	void push(int8_t value) override
	{
		unique_lock<mutex> lock(mtx);
		cond.wait(lock, [this] {return q.size() < size;});
		q.push(value);
		cond.notify_one();
	}
	bool pop(int8_t& value) override
	{
		unique_lock<mutex> lock(mtx);
		if (!cond.wait_for(lock, std::chrono::milliseconds(10), [this] {return !q.empty();}))
		{
			return false;
		}
		value = q.front();
		q.pop();
		cond.notify_one();
		return true;
	}
	bool empty() override 
	{
		unique_lock<mutex> lock(mtx);
		return q.empty();
	}
};
mutex del_mtx;
mutex mtx_for_result_sum;
void produce(IQueue* q)
{
	for (int i = 0;i < TaskNum;i++) {
		q->push(1);
	}
	del_mtx.lock();
	q->active_producers_count--;
	del_mtx.unlock();

}
mutex consumer_mtx;
void consume(IQueue* q)
{
	int current_result = 0;
	while (q->active_producers_count > 0 || !q->empty()) {
		int8_t val;
		if (q->pop(val)) {
			current_result += val;
		}
	}
	mtx_for_result_sum.lock();
	q->ResultSum += current_result;
	mtx_for_result_sum.unlock();
	return;
}
void task_2()
{
	cout << "==============================Task2==============================" << endl;
	cout << "=========================Dynamic Size Queue=========================" << endl;
	for (int k = 0; k < 3; k++) {
		cout << "=============== (Produsers: " << ProducerNum[k] << " Consumers: " << ConsumerNum[k] << ") ===============" << endl;
		thread *producers = new thread[ProducerNum[k]];
		thread *consumers = new thread[ConsumerNum[k]];
		DynamicSizeQueue q;
		q.active_producers_count = ProducerNum[k];
		auto start = chrono::high_resolution_clock::now();

		for (int i = 0;i < ConsumerNum[k];i++) {
			producers[i] = std::thread(produce, &q);
			consumers[i] = std::thread(consume, &q);
		}
		for (int i = 0;i < ProducerNum[k];i++) {
			producers[i].join();
			consumers[i].join();
		}

		cout << "Sum " << q.ResultSum << std::endl << "Right : " << TaskNum * ProducerNum[k] << endl;

		auto end = chrono::high_resolution_clock::now();
		chrono::duration<float> duration = end - start;
		cout << "Time: " << duration.count() << endl;
	}
	cout << "=========================Fixed Size Queue========================" << endl;
	for (int k = 0; k < 3; k++) {
		cout << "======= (Produsers: " << ProducerNum[k] << "\tConsumers: " << ConsumerNum[k] << "\tQueue Size: " << QueueSize[k] << ") =======" << endl;
		thread* producers = new thread[ProducerNum[k]];
		thread* consumers = new thread[ConsumerNum[k]];
		FixedSizeQueue q(QueueSize[k], ProducerNum[k]);

		auto start = chrono::high_resolution_clock::now();

		for (int i = 0;i < ConsumerNum[k];i++) {
			producers[i] = std::thread(produce, &q);
			consumers[i] = std::thread(consume, &q);
		}
		for (int i = 0;i < ConsumerNum[k];i++) {
			producers[i].join();
			consumers[i].join();
		}
		auto end = chrono::high_resolution_clock::now();
		chrono::duration<float> duration = end - start;
		cout << "Sum " << q.ResultSum << std::endl << "Right : " << TaskNum * ProducerNum[k] << endl;
		cout << "Time: " << duration.count() << endl;;
	}

}
int main() {
	int *Arr = new int[NumTasks];
	task_1(Arr);
	task_2();
	return 0;
};