#ifndef REFCOUNTEDOBJPTR_H
#define REFCOUNTEDOBJPTR_H

/*
a pointer to an object that supports reference counting.  
Increments reference count on auqisition, decrements it on release.
Deletes object if ref count reaches zero.
*/

#include "mutex.h"

template<typename T, typename MutexT=Synchronization::NullMutex>
class RefCountedObjPtr
{
public:
    explicit RefCountedObjPtr(T* ptr = 0);
    RefCountedObjPtr(const RefCountedObjPtr<T, MutexT>& rhs);
    ~RefCountedObjPtr();
    T& operator*() const;
    T* operator->() const;
    RefCountedObjPtr<T, MutexT>& operator=(const RefCountedObjPtr<T, MutexT>& rhs);
    RefCountedObjPtr<T, MutexT>& operator=(T* ptr);
	bool operator==(const RefCountedObjPtr<T>& rhs)const;
    bool operator==(const T* rhs)const;
    bool operator<(const RefCountedObjPtr<T>& rhs)const;
	bool operator!=(const RefCountedObjPtr<T>& rhs)const;
    T* GetPtr() const;

	operator bool () const;
private:
    mutable MutexT mLock;
	T* m_ptr;

	//const - modifies body of pointed to object only
	void Acquire()const;
	void Release();
};

//Construct from a raw pointer
template<typename T, typename MutexT>
inline RefCountedObjPtr<T, MutexT>::RefCountedObjPtr(T* ptr) 
    : m_ptr(ptr)
{
    Acquire();
}

//Copy constructor.
template<typename T, typename MutexT>
inline RefCountedObjPtr<T, MutexT>::RefCountedObjPtr(const RefCountedObjPtr<T, MutexT>& rhs) 
    : m_ptr(rhs.m_ptr)
{
    Acquire();		
}

//Destructor method.
template<typename T, typename MutexT>
inline RefCountedObjPtr<T, MutexT>::~RefCountedObjPtr()
{
    Release();
}

template<typename T, typename MutexT>
inline void RefCountedObjPtr<T, MutexT>::Acquire()const
{
    Synchronization::TMutexLock<MutexT> lock( mLock );
    if (m_ptr){
        m_ptr->AddRef();
    }
}

template<typename T, typename MutexT>
inline T& RefCountedObjPtr<T, MutexT>::operator*() const
{
    assert(m_ptr);
    return *m_ptr; 
}

template<typename T, typename MutexT>
inline T* RefCountedObjPtr<T, MutexT>::operator->() const
{
    assert(m_ptr);
    return m_ptr; 
}

template<typename T, typename MutexT>
inline RefCountedObjPtr<T, MutexT>& RefCountedObjPtr<T, MutexT>::operator=(const RefCountedObjPtr<T, MutexT>& rhs)
{
	rhs.Acquire();
	Release();
    m_ptr = rhs.m_ptr;

    return *this;
}

template<typename T, typename MutexT>
inline RefCountedObjPtr<T, MutexT>& RefCountedObjPtr<T, MutexT>::operator=(T* ptr)
{
    RefCountedObjPtr temp(ptr);
    return *this = temp;
}


template<typename T, typename MutexT>
inline void RefCountedObjPtr<T, MutexT>::Release()
{
    Synchronization::TMutexLock<MutexT> lock( mLock );
    if (m_ptr){
        if (m_ptr->DecRef()==0){
    	   delete m_ptr;		   
        }
#ifdef DEBUG
		m_ptr = 0xdeadbeef;
#endif
    }
}

template<typename T, typename MutexT>
inline bool RefCountedObjPtr<T, MutexT>::operator==(const RefCountedObjPtr<T>& rhs)const
{
	return m_ptr==rhs.m_ptr;
}

template<typename T, typename MutexT>
inline bool RefCountedObjPtr<T, MutexT>::operator==(const T* rhs)const
{
	return m_ptr==rhs;
}

template<typename T, typename MutexT>
inline bool RefCountedObjPtr<T, MutexT>::operator<(const RefCountedObjPtr<T>& rhs)const
{
	return m_ptr<rhs.m_ptr;
}

template<typename T, typename MutexT>
inline bool RefCountedObjPtr<T, MutexT>::operator!=(const RefCountedObjPtr<T>& rhs)const
{
	return m_ptr!=rhs.m_ptr;
}

//Returns the value of member 'm_ptr'.
template<typename T, typename MutexT>
inline T* RefCountedObjPtr<T, MutexT>::GetPtr() const
{
    return m_ptr;
}

template<typename T, typename MutexT>
inline RefCountedObjPtr<T, MutexT>::operator bool () const
{
	return m_ptr!=0;
}
#endif
