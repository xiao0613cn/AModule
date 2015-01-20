#ifndef _SPIN_LOCK_H_
#define _SPIN_LOCK_H_

typedef struct spin_lock {
	long volatile count;
} spin_lock;

static inline void spin_lock_init(spin_lock *lock)
{
	InterlockedExchange(&lock->count, 0);
}

static inline long spin_lock_try_lock(spin_lock *lock)
{
	return (InterlockedCompareExchange(&lock->count, 1, 0) == 0);
}

static inline void spin_lock_lock(spin_lock *lock)
{
	while (!spin_lock_try_lock(lock))
		Sleep(0);
}

static inline void spin_lock_unlock(spin_lock *lock)
{
	InterlockedCompareExchange(&lock->count, 0, 1);
}

static inline void spin_lock_release(spin_lock *lock)
{
}

#endif
