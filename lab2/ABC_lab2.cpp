#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include<queue>
#include<condition_variable>
using namespace std;

const int NumTasks = 1024 * 1024;
const int NumThreads[] = { 4, 8, 16, 32 };
int pointer = 0;
mutex mtx;
atomic<int> atomic_ptr{0};
void thread_proc_using_mutex(int* Arr, int sleep)
{
	while (pointer < NumTasks)
	{
		mtx.lock();
		if (pointer < NumTasks)
		{
			Arr[pointer] += 1;
			pointer++;
			if (sleep)
				this_thread::sleep_for(chrono::nanoseconds(sleep));
		}
		mtx.unlock();
	}
	return;
}
int getAtomicPtrValue(int sleep)
{
	int old = atomic_ptr.load();
	int newvalue = old;
	do {
		newvalue = old + 1;
		if(sleep)
			this_thread::sleep_for(chrono::nanoseconds(sleep));
	} while (!atomic_ptr.compare_exchange_strong(old, newvalue));
	return newvalue;
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
	subtask_1(Arr, thread_proc_using_mutex, 10); 
	return;
}
///////////////// task 2 //////////////////////////////////////////////////////////
const int ProducerNum[] = { 1,2,4 };
const int ConsumerNum[] = { 1,2,4 };
const int QueueSize[] = { 1, 4, 16 };
const int TaskNum = 1024 * 1024;// 4 * 1024 * 1024

__interface IQueue
{
public:
	void push(int8_t val);
	bool pop(int8_t& val);
	bool empty();
};
class DynamicSizeQueue : public IQueue
{
	queue<uint8_t> q;
	mutex mtx;
public:
	mutex mtx_for_result_sum;
	int active_producers_count = 0;
	int ResultSum = 0;
	DynamicSizeQueue()
	{
	}
	void push(int8_t val)
	{
		lock_guard<mutex> lock(mtx);
		q.push(val);
	}
	bool pop(int8_t& val)
	{
		lock_guard<mutex> lock(mtx);
		if (q.empty())
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
	bool empty() {
		std::lock_guard<std::mutex> lock(mtx);
		return q.empty();
	}
};
void produce(DynamicSizeQueue *q)
{
	for (int i = 0;i < TaskNum;i++) {
		q->push(1);
	}
	q->active_producers_count--;
}
void consume(DynamicSizeQueue *q)
{
	int current_result = 0;
	while(q->active_producers_count > 0 || !q->empty()) {
		int8_t val;
		if (q->pop(val)) {
			current_result += val;
		}
	}
	q->mtx_for_result_sum.lock();
	q->ResultSum += current_result;
	q->mtx_for_result_sum.unlock();
	return;
}
class FixedSizeQueue : public IQueue
{
	size_t size;
	mutex mtx;
	queue<int8_t> q;
	condition_variable cond;
public:
	mutex mtx_for_result_sum;
	mutex del_mtx;
	int active_producers_count;
	int ResultSum = 0;
	FixedSizeQueue(size_t s, int count)
	{
		size = s;
		active_producers_count = count;
	}
	void push(int8_t value) 
	{
		unique_lock<mutex> lock(mtx);
		cond.wait(lock, [this] {return q.size() < size;});
		q.push(value);
		cond.notify_one();
	}
	bool pop(int8_t& value)
	{
		unique_lock<mutex> lock(mtx);
		cond.wait(lock, [this] {return !q.empty();});
		value = q.front();
		if(value!=-1)
			q.pop();
		cond.notify_one();
		return true;
	}
	bool empty() {
		unique_lock<mutex> lock(mtx);
		return q.empty();
	}
};
void produce_fixed_size(FixedSizeQueue *q)
{
	for (int i = 0;i < TaskNum;i++) {
		q->push(1);
	}
	unique_lock<mutex>lock(q->del_mtx);
	q->active_producers_count--;
	if (q->active_producers_count == 0)
		q->push(-1);
}	  
void consume_fixed_size(FixedSizeQueue* q)
{
	int current_result = 0;
	while (true)
	{
		int8_t val;
		q->pop(val);
		if (val == -1) {
			q->mtx_for_result_sum.lock();
			q->ResultSum += current_result;
			q->mtx_for_result_sum.unlock();
			return;
		}
		current_result += val;
	}

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
	for (int k = 0; k < 3; k++) {
		cout << "=========================Fixed Size Queue========================" << endl;
		cout << "======= (Produsers: " << ProducerNum[k] << "\tConsumers: " << ConsumerNum[k] << "\tQueue Size: " << QueueSize[k] << ") =======" << endl;
		thread* producers = new thread[ProducerNum[k]];
		thread* consumers = new thread[ConsumerNum[k]];
		FixedSizeQueue q(QueueSize[k], ProducerNum[k]);

		auto start = chrono::high_resolution_clock::now();

		for (int i = 0;i < ConsumerNum[k];i++) {
			producers[i] = std::thread(produce_fixed_size, &q);
			consumers[i] = std::thread(consume_fixed_size, &q);
		}
		for (int i = 0;i < ConsumerNum[k];i++) {
			producers[i].join();
			consumers[i].join();
		}

		cout << "Sum " << q.ResultSum << std::endl << "Right : " << TaskNum * ProducerNum[k] << endl;

		auto end = chrono::high_resolution_clock::now();
		chrono::duration<float> duration = end - start;
		cout << "Time: " << duration.count() << endl;

	}

}
int main() {
	int *Arr = new int[NumTasks];
	task_1(Arr);
	task_2();
	return 0;
};