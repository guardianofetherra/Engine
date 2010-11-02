//----------------------------------------------------------------------------------------------------------------------
// DynamicMemoryHeap.cpp
//
// Copyright (C) 2010 WhiteMoon Dreams, Inc.
// All Rights Reserved
//----------------------------------------------------------------------------------------------------------------------

#include "CorePch.h"
#include "Core/Memory.h"

#include "Core/Threading.h"
#include "Core/Debug.h"

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4530 )  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#endif

#include <hash_map>

#ifdef _MSC_VER
#pragma warning( pop )
#endif

/// Mutex wrapper buffer.  We use this buffer to be able to set aside space for an uninitialized Lunar::LwMutex in which
/// we can properly construct one using placement new, as dlmalloc/nedmalloc are built to deal with more basic C types.
struct MallocMutex
{
    size_t buffer[ ( sizeof( Lunar::LwMutex ) + sizeof( size_t ) - 1 ) / sizeof( size_t ) ];
};

/// Mutex initialize wrapper.
///
/// @param[in] pMutex  Mutex to initialize.
///
/// @return  Zero.
static int MallocMutexInitialize( MallocMutex* pMutex )
{
    new( pMutex->buffer ) Lunar::LwMutex;
    return 0;
}

/// Mutex lock wrapper.
///
/// @param[in] pMutex  Mutex to lock.
///
/// @return  Zero.
static int MallocMutexLock( MallocMutex* pMutex )
{
    reinterpret_cast< Lunar::LwMutex* >( pMutex->buffer )->Lock();
    return 0;
}

/// Mutex unlock wrapper.
///
/// @param[in] pMutex  Mutex to unlock.
///
/// @return  Zero.
static int MallocMutexUnlock( MallocMutex* pMutex )
{
    reinterpret_cast< Lunar::LwMutex* >( pMutex->buffer )->Unlock();
    return 0;
}

/// Mutex try-lock wrapper.
///
/// @param[in] pMutex  Mutex to attempt to lock.
///
/// @return  True if the mutex was locked successfully by this thread, false if it was locked by another thread.
static bool MallocMutexTryLock( MallocMutex* pMutex )
{
    return reinterpret_cast< Lunar::LwMutex* >( pMutex->buffer )->TryLock();
}

/// Get a reference to the global dlmalloc mutex.
///
/// @return  Reference to the global dlmalloc mutex.
static Lunar::LwMutex& GetMallocGlobalMutex()
{
    // Initialize as a local variable to try to ensure it is initialized the first time it is used.
    static Lunar::LwMutex globalMutex;
    return globalMutex;
}

namespace Lunar
{
#if L_ENABLE_MEMORY_TRACKING_VERBOSE
    /// Dynamic memory heap verbose tracking data.
    struct DynamicMemoryHeapVerboseTrackingData
    {
        /// Allocation backtraces for this heap.
        stdext::hash_map< void*, DynamicMemoryHeap::AllocationBacktrace > allocationBacktraceMap;
    };

    static volatile Thread::id_t s_verboseTrackingCurrentThreadId = Thread::INVALID_ID;

    static LwMutex& GetVerboseTrackingMutex()
    {
        static LwMutex verboseTrackingMutex;

        return verboseTrackingMutex;
    }

    static bool ConditionalVerboseTrackingLock()
    {
        Thread::id_t threadId = Thread::GetCurrentId();
        if( s_verboseTrackingCurrentThreadId != threadId )
        {
            GetVerboseTrackingMutex().Lock();
            s_verboseTrackingCurrentThreadId = threadId;

            return true;
        }

        return false;
    }

    static void VerboseTrackingUnlock()
    {
        L_ASSERT( s_verboseTrackingCurrentThreadId == Thread::GetCurrentId() );

        s_verboseTrackingCurrentThreadId = Thread::INVALID_ID;
        GetVerboseTrackingMutex().Unlock();
    }
#endif  // L_ENABLE_MEMORY_TRACKING_VERBOSE
}

#ifndef USE_NEDMALLOC
#define USE_NEDMALLOC 0
#endif

#define MEMORY_HEAP_CLASS_NAME DynamicMemoryHeap

#define USE_LOCKS 2
#define MLOCK_T MallocMutex
#define CURRENT_THREAD Lunar::Thread::GetCurrentId()
#define INITIAL_LOCK( sl ) MallocMutexInitialize( sl )
#define ACQUIRE_LOCK( sl ) MallocMutexLock( sl )
#define RELEASE_LOCK( sl ) MallocMutexUnlock( sl )
#define TRY_LOCK( sl ) MallocMutexTryLock( sl )
#define ACQUIRE_MALLOC_GLOBAL_LOCK() GetMallocGlobalMutex().Lock();
#define RELEASE_MALLOC_GLOBAL_LOCK() GetMallocGlobalMutex().Unlock();

#include "DlMallocMemoryHeap.inl"

namespace Lunar
{
    DynamicMemoryHeap* volatile DynamicMemoryHeap::sm_pGlobalHeapListHead = NULL;
#if L_ENABLE_MEMORY_TRACKING_VERBOSE
    volatile bool DynamicMemoryHeap::sm_bDisableBacktraceTracking = false;
#endif

    /// Acquire a read-only lock on the global dynamic memory heap list.
    ///
    /// @return  Heap at the head of the list.
    ///
    /// @see UnlockReadGlobalHeapList(), GetPreviousHeap, GetNextHeap()
    DynamicMemoryHeap* DynamicMemoryHeap::LockReadGlobalHeapList()
    {
        GetGlobalHeapListLock().LockRead();

        return sm_pGlobalHeapListHead;
    }

    /// Release a previously acquired read-only lock on the global dynamic memory heap.
    ///
    /// @see LockReadGlobalHeapList(), GetPreviousHeap, GetNextHeap()
    void DynamicMemoryHeap::UnlockReadGlobalHeapList()
    {
        GetGlobalHeapListLock().UnlockRead();
    }

#if L_ENABLE_MEMORY_TRACKING
    /// Write memory stats for all dynamic memory heap instances to the output log.
    void DynamicMemoryHeap::LogMemoryStats()
    {
        L_LOG( LOG_DEBUG, L_T( "DynamicMemoryHeap stats:\n" ) );
        L_LOG( LOG_DEBUG, L_T( "Heap name\tActive allocations\tBytes allocated\n" ) );

        ScopeReadLock readLock( GetGlobalHeapListLock() );

        for( DynamicMemoryHeap* pHeap = sm_pGlobalHeapListHead; pHeap != NULL; pHeap = pHeap->GetNextHeap() )
        {
            const tchar_t* pName = NULL;
#if !L_RELEASE
            pName = pHeap->GetName();
#endif
            if( !pName )
            {
                pName = L_T( "(unnamed)" );
            }

            size_t allocationCount = pHeap->GetAllocationCount();
            size_t bytesActual = pHeap->GetBytesActual();

            L_LOG(
                LOG_DEBUG,
                L_T( "%s\t%" ) TPRIuSZ L_T( "\t%" ) TPRIuSZ L_T( "\n" ),
                pName,
                allocationCount,
                bytesActual );
        }

        L_LOG( LOG_DEBUG, L_T( "\n" ) );

#if L_ENABLE_MEMORY_TRACKING_VERBOSE
        bool bLockedTracking = ConditionalVerboseTrackingLock();

        bool bOldDisableBacktraceTracking = sm_bDisableBacktraceTracking;
        sm_bDisableBacktraceTracking = true;

        L_LOG( LOG_DEBUG, L_T( "DynamicMemoryHeap unfreed allocations:\n" ) );

        size_t allocationIndex = 1;

        for( DynamicMemoryHeap* pHeap = sm_pGlobalHeapListHead; pHeap != NULL; pHeap = pHeap->GetNextHeap() )
        {
            const tchar_t* pHeapName = pHeap->GetName();

            DynamicMemoryHeapVerboseTrackingData* pTrackingData = pHeap->m_pVerboseTrackingData;
            if( pTrackingData )
            {
                const stdext::hash_map< void*, AllocationBacktrace >& rAllocationBacktraceMap =
                    pTrackingData->allocationBacktraceMap;

                String symbol;

                stdext::hash_map< void*, AllocationBacktrace >::const_iterator iterEnd = rAllocationBacktraceMap.end();
                stdext::hash_map< void*, AllocationBacktrace >::const_iterator iter;
                for( iter = rAllocationBacktraceMap.begin(); iter != iterEnd; ++iter )
                {
                    L_LOG(
                        LOG_DEBUG,
                        L_T( "%" ) TPRIuSZ L_T( ": 0x%p (%s)\n" ),
                        allocationIndex,
                        iter->first,
                        pHeapName );
                    ++allocationIndex;

                    void* const* ppTraceAddress = iter->second.pAddresses;
                    for( size_t addressIndex = 0;
                         addressIndex < L_ARRAY_COUNT( iter->second.pAddresses );
                         ++addressIndex )
                    {
                        void* pAddress = *ppTraceAddress;
                        ++ppTraceAddress;
                        if( !pAddress )
                        {
                            break;
                        }

                        GetAddressSymbol( symbol, pAddress );
                        L_LOG( LOG_DEBUG, L_T( "- 0x%p: %s\n" ), pAddress, *symbol );
                    }
                }
            }
        }

        L_LOG( LOG_DEBUG, L_T( "\n" ) );

        sm_bDisableBacktraceTracking = bOldDisableBacktraceTracking;

        if( bLockedTracking )
        {
            VerboseTrackingUnlock();
        }
#endif
    }
#endif  // L_ENABLE_MEMORY_TRACKING

    /// Get the read-write lock used for synchronizing access to the global dynamic memory heap list.
    ///
    /// @return  Global heap list read-write lock.
    ReadWriteLock& DynamicMemoryHeap::GetGlobalHeapListLock()
    {
        // Note that the construction of this is not inherently thread-safe, but we can be fairly certain that the main
        // thread will trigger the creation of the lock before any other threads are spawned.
        static ReadWriteLock globalHeapListLock;

        return globalHeapListLock;
    }
}