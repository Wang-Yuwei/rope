#ifndef REFCOUNTER_H_INCLUDED
#define REFCOUNTER_H_INCLUDED

/*
Mixin class for counting references to an object.  
Derive from this class to make something ref-countable.

NOTE that this is a *mixin* class (inheritance for functionality reuse), 
and does NOT have a virtual destructor.  
Do NOT, I repeat DO NOT use a pointer to a RefCounter to delete an object derived from it.
*/

#include <assert.h>

#include "mutex.h"

template<typename MutexT=Synchronization::NullMutex>
class TRefCounter
{
public:
	size_t AddRef();
	size_t DecRef();
	bool IsUnique() const;
	size_t GetRefCount() const;

protected:
	TRefCounter();
    ~TRefCounter();
    
private:
    MutexT mLock;
    size_t m_refCount;
};

template<typename MutexT>
inline TRefCounter<MutexT>::TRefCounter()
    : m_refCount(0)
{  }

template<typename MutexT>
inline TRefCounter<MutexT>::~TRefCounter()
{
     assert( m_refCount == 0 );
}

//increments the counter and returns it's current value
template<typename MutexT>
inline size_t TRefCounter<MutexT>::AddRef()
{
    Synchronization::TMutexLock<MutexT> lock( mLock );
    return ++m_refCount;
}

//decrements the counter and returns it's current value
template<typename MutexT>
inline size_t TRefCounter<MutexT>::DecRef()
{
    Synchronization::TMutexLock<MutexT> lock( mLock );
    assert(m_refCount>0);
    return --m_refCount;
}

//Is there only one reference to the object?
template<typename MutexT>
inline bool TRefCounter<MutexT>::IsUnique() const
{
    return m_refCount==1;
}

//Returns the value of member 'm_refCount'.
template<typename MutexT>
inline size_t TRefCounter<MutexT>::GetRefCount() const
{
    return m_refCount;
}

typedef TRefCounter<> RefCounter;

#endif