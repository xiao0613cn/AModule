#ifndef _SPIN_LOCK_H_
#define _SPIN_LOCK_H_


struct pthread_mutex_helper {
	pthread_mutex_t *mutex;
	pthread_mutex_helper(pthread_mutex_t *m) :mutex(m) {}

	void lock() { pthread_mutex_lock(mutex); }
	bool try_lock() { return !pthread_mutex_trylock(mutex); }
	void unlock() { pthread_mutex_unlock(mutex); }
};

struct spin_lock_t {
	long volatile count;
	spin_lock_t() :count(0) { }

	void lock() { while (!try_lock()) Sleep(0); }
	bool try_lock() { return (InterlockedCompareExchange(&count, 1, 0) == 0); }
	void unlock() { InterlockedCompareExchange(&count, 0, 1); }
};

struct spin_lock_helper {
	long volatile *count;
	spin_lock_helper(long volatile &n) :count(&n) { }
	spin_lock_helper(spin_lock_t &l) :count(&l.count) { }

	void lock() { while (!try_lock()) Sleep(0); }
	bool try_lock() { return (InterlockedCompareExchange(count, 1, 0) == 0); }
	void unlock() { InterlockedCompareExchange(count, 0, 1); }
};

struct null_lock_helper {
	void lock() { }
	bool try_lock() { return true; }
	void unlock() { }
};

#endif
