#ifndef __ZMQ_YPIPE_HPP_INCLUDED__
#define __ZMQ_YPIPE_HPP_INCLUDED__

#include "atomic_ptr.hpp"
#include "yqueue.hpp"


template<typename T,int N>
class ypipe_t
{
public:
    inline ypipe_t()
    {
        queue.push();
        r = w = f = &queue.back();
        c.set(&queue.back());
    }
    inline virtual ~ypipe_t(){}

    inline void write(const T&value_,bool incomplete_)
    {
        queue.back() = value_;
        queue.push();

        if(!incomplete_)
        {
            f = &queue.back();
            printf("1 f:%p,w:%p\n",f,w);
        }else{
            printf("0 f:%p,w:%p\n",f,w);
        }
    }

    inline bool unwrite(T *value)
    {
        if(f == &queue.back())
            return false;
        queue.unpush();
        *value_ = queue.back();
        return true;
    }

    inline bool flush()
    {
        if(w==f)
            return true;
        
        if(c.cas(w,f) != w)
        {
            c.set(f);
            w=f;
            return false;

        }else{

            w=f;
            return true;
        }
    }

    inline bool check_read()
    {
        if(&queue.front()!=r && r)
            return true;

        r = c.cas(&queue.front(),NULL);

        if(&queue.front()==r || !r)
            return false;

        return true;
    }

    inline bool read(T *value_)
    {
        if(!check_read())
            return false;
        
        *value_ = queue.front();
        queue.pop();
        return true;
    }

    inline bool probe(bool (*fn)(T &))
    {
        bool rc = check_read();
        //zmq_assert(rc);

        return (*fn)(queue.front());
    }


protected:
    yqueue_t<T,N>queue;

    T *w; //指向第一个未刷新的元素，只被写线程使用
    T *r; //指向第一个还没预提取的元素，只被读线程使用
    T *f; //指向下一轮要被刷新的一批元素中的第一个

    atomic_ptr_t<T> c;  //读写线程共享的指针，指向每一轮刷新的起点。当c为空时读线程休眠

    ypipe_t(const ypipe_t &);
    const ypipe_t &operator(const ypipe_t &);

};

#endif