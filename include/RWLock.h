#ifndef RWLOCK_H
#define RWLOCK_H

#include <mutex>

class RWLock
{
public:
	void getReadLock()
	{
		std::unique_lock<std::mutex> lock(m);
		while (writerUsed == true)
		{
			cv.wait(lock);
		}
		readerCount++;
	}
	void getWriteLock()
	{
		std::unique_lock<std::mutex> lock(m);
		while (readerCount != 0 || writerUsed == true)
		{
			cv.wait(lock);
		}
		writerUsed = true;
	}
	void unlockReader()
	{
		std::unique_lock<std::mutex> lock(m);
		readerCount--;
		if (readerCount == 0)
		{
			cv.notify_all();
		}
	}
	void unlockWriter()
	{
		std::unique_lock<std::mutex> lock(m);
		writerUsed = false;
		cv.notify_all();
	}
 
private:
	int readerCount = 0;
	bool writerUsed = false;
	std::mutex m;
	std::condition_variable cv;
};


#endif