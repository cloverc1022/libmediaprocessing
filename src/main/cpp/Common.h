#if !defined(__COMMON____H)
#define __COMMON____H


#include "SEP2P_Type.h"

typedef pthread_mutex_t				pub_mutex_t;
//void my_mutex_init(pub_mutex_t *p_mutex);
#define pub_mutex_init(theMutex)		pthread_mutex_init(&theMutex, NULL)
#define pub_mutex_lock(theMutex)		pthread_mutex_lock(&theMutex)
#define pub_mutex_unlock(theMutex)		pthread_mutex_unlock(&theMutex)
#define pub_mutex_destroy(theMutex)

typedef pthread_t						pub_threadid;
typedef sem_t							pub_event_t;
#define  pub_ResetEvent(e)				sem_init(&e, 0, 0);
#define  pub_SetEvent(e)				sem_post(&e)
//int WaitForSingleObject(sem_t *mutex, unsigned long TimeOut_ms);
#define	 pub_WaitForSingleObject(e,timeout_ms) WaitForSingleObject(&e, timeout_ms)
#define  pub_WAIT_OBJECT_0			0
char *pub_strupr(char *s);
#define pub_stricmp					strcasecmp

typedef	void*		    handle_thread;
#define pub_thread_join(hHandle, threadid) pthread_join(threadid, NULL)


#ifndef SAFE_DELETE
#define SAFE_DELETE(p)\
{\
    if((p) != NULL)\
    {\
        delete (p) ;\
        (p) = NULL ;\
    }\
}
#endif

#ifndef SAFE_DELETE_ARR
#define SAFE_DELETE_ARR(p)\
{\
    if((p) != NULL)\
    {\
        delete [](p);\
        (p) = NULL ;\
    }\
}
#endif


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void mp4pub_Sleep(UINT32 ms);
ULONG mp4pub_GetTickCount();

INT32 mp4pub_SscanfString(const char *pszBuffer, const char *reg, char *szResult, int nResultSize);
INT32 mp4pub_SscanfInt(const char *pszBuffer, const char *reg, int &nResult);

VOID mp4pub_CloseSocket(INT32 *pSocket);
typedef handle_thread (WINAPI * PPUB_PTHREAD_START_ROUTINE ) ( VOID *lpThreadParameter ) ;
INT32 mp4pub_CreateThread(PPUB_PTHREAD_START_ROUTINE lpStartAddress, IN VOID *lpParameter, IN
                          INT32 dwCreationFlags, IN CHAR bDetach, OUT VOID **hhThread, OUT
                          pub_threadid *lpThreadId);
VOID mp4pub_DestroyThread(VOID **hThread);


VOID mp4pub_TRACE_DEBUG(UINT32 level, const CHAR *format, ...);

#ifdef __cplusplus
}
#endif //__cplusplus


// ===========================================================================
//      SE_Mutex class
// ===========================================================================
class SE_Mutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    SE_Mutex();

    SE_Mutex(const char *name);

    SE_Mutex(int type, const char *name = NULL);

    ~SE_Mutex();

    // lock or unlock the mutex
    status_t lock();

    void unlock();

    // lock if possible; returns 0 on success, error otherwise
    status_t tryLock();

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
    class Autolock {
    public:
        inline Autolock(SE_Mutex &mutex) : mLock(mutex) { mLock.lock(); }

        inline Autolock(SE_Mutex *mutex) : mLock(*mutex) { mLock.lock(); }

        inline ~Autolock() { mLock.unlock(); }

    private:
        SE_Mutex &mLock;
    };

private:
    //friend class Condition;

    // A mutex cannot be copied
    SE_Mutex(const SE_Mutex &);

    SE_Mutex &operator=(const SE_Mutex &);

#if defined(HAVE_PTHREADS)
    pthread_mutex_t mMutex;
#else

    void _init();

    void *mState;
#endif
};


#endif  //__COMMON____H