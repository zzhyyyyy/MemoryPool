// test memory_pool
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include "MemoryPool.cpp"

std::atomic<size_t>New_Time,MemoryPool_Time;
// 测试用例
class P1 
{
    int id_;
};

class P2 
{
    int id_[5];
};

class P3
{
    double id_[1000];
};

class P4
{
    long long id_[2000];
};

//单轮次申请释放次数 线程数 轮次
void BenchmarkMemoryPool(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks); // 线程池
	for (size_t k = 0; k < nworks; ++k) // 创建 nworks 个线程
	{
		vthread[k] = std::thread([&](){
			for (size_t j = 0; j < rounds; ++j)
			{
				auto begin = std::chrono::steady_clock::now();
				for (size_t i = 0; i < ntimes; i++)
				{
                    P1* p1 = newElement<P1>(); // 内存池对外接口
                    deleteElement<P1>(p1);
                    P2* p2 = newElement<P2>();
                    deleteElement<P2>(p2);
                    P3* p3 = newElement<P3>();
                    deleteElement<P3>(p3);
                    P4* p4 = newElement<P4>();
                    deleteElement<P4>(p4);
				}
				auto end = std::chrono::steady_clock::now();
				MemoryPool_Time.fetch_add(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count(),std::memory_order_relaxed);
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%lu个线程并发执行%lu轮次,每轮次newElement&deleteElement %lu次,总计花费:%lu ms\n", nworks, rounds, ntimes, MemoryPool_Time.load()/1000);
}

void BenchmarkNew(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			for (size_t j = 0; j < rounds; ++j)
			{
				auto begin = std::chrono::steady_clock::now();
				for (size_t i = 0; i < ntimes; i++)
				{
                    P1* p1 = new P1;
                    delete p1;
                    P2* p2 = new P2;
                    delete p2;
                    P3* p3 = new P3;
                    delete p3;
                    P4* p4 = new P4;
                    delete p4;
				}
				auto end = std::chrono::steady_clock::now();
				New_Time.fetch_add(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count(),std::memory_order_relaxed);
			}
		});
	}
	for (auto& t : vthread)
	{
		t.join();
	}
	printf("%lu个线程并发执行%lu轮次,每轮次malloc&free %lu次,总计花费:%lu ms\n", nworks, rounds, ntimes, New_Time.load()/1000);
}

int main()
{
    MemoryBucket::initMemoryPool(); // 使用内存池接口前一定要先调用该函数
	BenchmarkMemoryPool(10000, 5, 100); // 测试内存池
	std::cout << "===========================================================================" << std::endl;
	std::cout << "===========================================================================" << std::endl;
	BenchmarkNew(10000, 5, 100); // 测试 new delete
	return 0;
}