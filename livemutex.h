/***********************************************************************************
*                                                                                  *
*  Copyright (c) 2017 by Nedeljko Stefanovic                                       *
*                                                                                  *
*  Permission is hereby granted, free of charge, to any person obtaining a copy    *
*  of this software and associated documentation files (the "Software"), to deal   *
*  in the Software without restriction, including without limitation the rights    *
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*  copies of the Software, and to permit persons to whom the Software is           *
*  furnished to do so, subject to the following conditions:                        *
*                                                                                  *
*  The above copyright notice and this permission notice shall be included in all  *
*  copies or substantial portions of the Software.                                 *
*                                                                                  *
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*  SOFTWARE.                                                                       *
*                                                                                  *
***********************************************************************************/



#ifndef LIVEMUTEX_H
#define LIVEMUTEX_H


#include <atomic>
#include <set>


class LiveMutex
{
    std::atomic<bool> locked;

    struct ThreadData
    {
        std::atomic<bool> locked;
        unsigned int lockedMutextes;
        LiveMutex *blockedByMutex;

        ThreadData() :
            locked(false),
            lockedMutextes(0),
            blockedByMutex(nullptr)
        {
        }
    };

    static ThreadData& threadData()
    {
        thread_local static ThreadData result;

        return result;
    }

    ThreadData* lockedByThread;
    unsigned int numberOfLocks;

public:
    LiveMutex()
    {
        locked = false;
        lockedByThread = nullptr;
        numberOfLocks = 0;
    }

    bool isLocked() const
    {
        return numberOfLocks>0;
    }

    unsigned int numOfLocks() const
    {
        return numberOfLocks;
    }

    bool tryLock()
    {
//      The mutex attribute can be temporary locked inside this methods only.
//      For this reason, thread can not lock multiple such attributes simultanely.
//      Therefore, deadlock is not possible about such attributes.
        while (threadData().locked.exchange(true)) {
        }

        while (locked.exchange(true)) {
        }

        if (lockedByThread!=nullptr) {
            if (lockedByThread==&threadData()) {
                ++numberOfLocks;
                threadData().locked = false;
                locked = false;

                return true;
            }

            locked = false;
            threadData().locked = false;

            return false;
        }

        lockedByThread = &threadData();
        locked = false;
        threadData().blockedByMutex = nullptr;

        return true;
    }

private:
    /**
     * @brief lock
     * @return
     * Returns true if mutex is locked succesfully.
     * Returns false if mutex is not
     */

    bool lock()
    {
        while (threadData().locked.exchange(true)) {
        }

        while (true) {
            while (locked.exchange(true)) {
            };

            if (lockedByThread==nullptr) {
                lockedByThread = &threadData();
                ++numberOfLocks;
                ++threadData().lockedMutextes;
                locked = false;
                threadData().locked = false;

                return true;
            } else if (lockedByThread==&threadData()) {
//              Recursive lock.
                ++numberOfLocks;
                ++threadData().lockedMutextes;
                locked = false;
                threadData().locked = false;

                return true;
            }

            if (threadData().lockedMutextes==0) {
//              In this case blocking current thread can not create deadlock.
                while (true) {
                    locked = false;

                    while (lockedByThread!=nullptr) {
                    }

                    threadData().blockedByMutex = this;

                    while (locked.exchange(true)) {
                    }

                    lockedByThread = &threadData();
                    ++threadData().lockedMutextes;
                    ++numberOfLocks;
                    locked = false;

                    return true;
                }
            }

//          This part detects deadlock danger in case of blocking this thread.
//          If such danger is present, then all other relevant threads are already blocked.
//          Mutex can be unlocked by the same thread what locked it.
//          For this reason, in such case relevant changes are not possible and danger will be detected.
//          Otherwise, locking will be attempted again in the loop.
            LiveMutex *currentMutex = this;
            ThreadData *currentThread = &threadData();

            std::set<ThreadData*> threads;

            while (true) {
                ThreadData *nextThread = lockedByThread;

//              Deadlock danger is detected. Both of mutex locking and thread blocking is impossible.
                if (nextThread==nullptr || threads.count(nextThread)>0) {
                    currentMutex->locked = false;
                    currentThread->locked = false;
                    threadData().locked = false;

                    return false;
                }

                threads.insert(nextThread);

                while (nextThread->locked.exchange(true)) {
                }

                if (currentThread!=&threadData()) {
                    currentThread->locked = false;
                }

                currentThread = nextThread;

                LiveMutex *nextMutex = currentThread->blockedByMutex;

                if (nextMutex==nullptr) {
                    currentMutex->locked = false;
                    currentThread->locked = false;
                    threadData().locked = false;

                    return false;
                }

                while (nextMutex->locked.exchange(true)) {
                }

                currentMutex->locked = false;
                currentMutex = nextMutex;

                if (currentMutex->lockedByThread==nullptr) {
                    currentMutex->locked = false;
                    currentThread->locked = false;
                    threadData().locked = false;

                    return false;
                }
            }
//          If deadlock danger is not detected, try again.
        }
    }

    void unlock()
    {
//      The mutex attribute can be temporary locked inside this methods only.
//      For this reason, thread can not lock multiple such attributes simultanely.
//      Therefore, deadlock is not possible about such attributes.
        while (locked.exchange(true)) {
        };

        lockedByThread = nullptr;
        --numberOfLocks;

        if (numberOfLocks==0) {
            --threadData().lockedMutextes;
        }

        locked = false;
    }

    friend class LiveMutexLocker;
};

class LiveMutexLocker
{
    LiveMutex &liveMutex;
    bool locked;

public:
    LiveMutexLocker(LiveMutex &liveMutex, bool tryBlock = true) :
        liveMutex(liveMutex)
    {
        if (tryBlock) {
            locked = liveMutex.lock();
        } else {
            locked = liveMutex.tryLock();
        }
    }

    ~LiveMutexLocker()
    {
        if (locked) {
            liveMutex.unlock();
        }
    }

    bool hasLocked() const
    {
        return locked;
    }
};

#endif // LIVEMUTEX_H
