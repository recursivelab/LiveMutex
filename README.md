LiveMutex is the deadlock free mutex class.


It's lock() method is between standard tryLock() and standard lock methods().
The tryLock() method gives up if mutex locking is not possible at this moment.
The standard lock method blocks the thread if mutex locking is not possible at this moment,
but it can cause deadlock problem.

In case that mutex is already locked, the lock() method of this class makes evaluation
does the blocking thread leads to deadlock or not. If yes, it gives up by returning
false that indicates locking failure. This class has tryLock() method to.

The LiveMutexLocker and MutexTryLocker class has purporse of ensuring unlocking mutextes.
The constructor is about locking and destructor about unlocking. Typical use of these class is

LiveMutexLocker locker(liveMutex);

if (locker.hasLocked()) {
    // Some code
} else {
    return ERROR_CODE;
}

where liveMutex is some object of class LiveMutex. Out of locker scope, liveMutex is
automatically unlocked (if it is locked by this object before). In the case of locking failure,
program has to return up to point of the last locking by this thread, unlock related
LiveMutex object and then try again. In the case of locking failure again, program has
to propagate it backward to the next point of LiveMutex locking.

This class supports recursive lockings to. For tryLock() method use

LiveMutexLocker locker(liveMutex, false);

with false as the second argument.


Good luck!
