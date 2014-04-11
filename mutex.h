#ifndef MUTEX_H_INCLUDED
#define MUTEX_H_INCLUDED

#ifdef WIN32
#include "Windows.h"

//unfortunatly the Microsoft compiler\headers arn't standard compliant
//see: http://x42.deja.com/getdoc.xp?AN=520752890

#ifdef _MSC_VER
	#ifdef max 
	#undef max
	#endif
#endif


namespace Synchronization
{
	class Win32Mutex
	{
		public:
			Win32Mutex() { 
				InitializeCriticalSection( &mCriticalSection ); 
			}
			~Win32Mutex() {
				DeleteCriticalSection( &mCriticalSection );
			}

			void Lock() {
				EnterCriticalSection(  &mCriticalSection );
			}

			void Unlock() {
				LeaveCriticalSection(  &mCriticalSection );
			}

		private:
			Win32Mutex(const Win32Mutex &);
			Win32Mutex& operator=(const Win32Mutex &);
			
			CRITICAL_SECTION mCriticalSection;
	};
	
	typedef Win32Mutex Mutex;
}

#else
#include <pthread.h>
namespace Synchronization
{
	class PThreadMutex
	{
		public:
			PThreadMutex() { 
				pthread_mutex_init( &mCriticalSection, 0 ); 
			}
			~PThreadMutex() {
				pthread_mutex_destroy( &mCriticalSection );
			}

			void Lock() {
				pthread_mutex_lock(  &mCriticalSection );
			}

			void Unlock() {
				pthread_mutex_unlock(  &mCriticalSection );
			}

		private:
			PThreadMutex(const PThreadMutex &);
			PThreadMutex& operator=(const PThreadMutex &);
			
			pthread_mutex_t mCriticalSection;
	};
	
	typedef PThreadMutex Mutex;
}
#endif

namespace Synchronization
{
	class NullMutex
	{
		public:
			NullMutex() { }
			~NullMutex() { }
			void Lock() { }
			void Unlock() { }
		private:
			NullMutex(const NullMutex&);
			NullMutex& operator=(const NullMutex&);
	};

	template< typename MutexType >
	class TMutexLock
	{
		public:
			TMutexLock( MutexType& mutex ) 
				: mMutex( mutex )
			{
				mMutex.Lock();
			}

			~TMutexLock() {
				mMutex.Unlock();
			}

		private:
			MutexType& mMutex;
	};
	
	typedef TMutexLock<Mutex> MutexLock;
	
}
#endif