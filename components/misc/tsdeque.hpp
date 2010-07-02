#ifndef TSDEQUE_H
#define TSDEQUE_H

#include <boost/thread.hpp>

//
// Adapted from http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
//
template<typename Data>
class TsDeque
{
private:
    std::deque<Data> the_queue;
    mutable boost::mutex the_mutex;
    boost::condition_variable the_condition_variable;

public:
    void push_back(Data const& data)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        the_queue.push_back(data);
        lock.unlock();
        the_condition_variable.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_queue.empty();
    }

    bool try_pop_front(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        if(the_queue.empty())
            return false;
       
        popped_value=the_queue.front();
        the_queue.pop_front();
        return true;
    }

    void wait_and_pop_front(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        while(the_queue.empty())
        {
            the_condition_variable.wait(lock);
        }
        
        popped_value=the_queue.front();
        the_queue.pop_front();
    }
};

#endif // TSDEQUE_H
