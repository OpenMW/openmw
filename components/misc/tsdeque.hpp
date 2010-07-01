#ifndef TSDEQUE_H
#define TSDEQUE_H

#include <boost/thread.hpp>

template <typename T>
class TsDeque
{
public:
    void push_back (const T& t)
    {
        boost::mutex::scoped_lock lock(mMutex);
        mDeque.push_back(t);
    }

    bool pop_front (T& t)
    {
        boost::mutex::scoped_lock lock(mMutex);
        if (!mDeque.empty())
        {
            t = mDeque.front();
            mDeque.pop_front();
            return true;
        }
        else
            return false;
    }

protected:
    std::deque<T>        mDeque;
    mutable boost::mutex mMutex;
};

#endif // TSDEQUE_H
