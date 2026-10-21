/*************************************************************************

a.b.c.d:2014-04-25 [Apollo Ju]
	First release

*************************************************************************/

#include "Common.h"


extern  pub_mutex_t g_lockLog_mp4;
extern 	UINT32 g_nDEBUG_LEVELmp4;


void mp4pub_Sleep(UINT32 ms)
{
	#ifndef WIN32DLL
		usleep(ms * 1000);
	#else
		Sleep(ms);
	#endif
}

ULONG mp4pub_GetTickCount()
{
#ifndef WIN32DLL
	ULONG currentTime=0;
	struct timeval current;
	gettimeofday(&current, NULL);
	currentTime = current.tv_sec*1000 + current.tv_usec/1000;
	return currentTime;	
#else
	return GetTickCount();
#endif
}


INT32 mp4pub_SscanfString(const CHAR *pszBuffer , const CHAR *reg, CHAR *szResult, int nResultSize)
{
    memset(szResult, 0, nResultSize) ;
    CHAR buf[512] = {0};
    sprintf(buf, "%s\"", reg);
    CHAR *p = strstr((char*)pszBuffer, buf);
    if(p == NULL) return 0;

    size_t len = strlen(buf);
    p += len;
    char *pEnd = strstr(p, "\"");
    if(pEnd == NULL) return 0;

    INT32 offset = pEnd - p;
    if(offset > 128) return 0;

    INT32 i;
    for(i = 0; i < offset; i++) szResult[i] = p[i];

    mp4pub_TRACE_DEBUG(0x1<<0, "szResult: %s", szResult);
 	return 1;
}

INT32 mp4pub_SscanfInt(const CHAR *pszBuffer , const CHAR *reg , INT32 &nResult)
{
	CHAR * p = strstr((CHAR *)pszBuffer,(CHAR *)reg);
	if(p == NULL) return 0;

	CHAR buf[512] = {0};
	sprintf(buf,"%s%s", reg,"%d");
	sscanf(p, buf, &nResult);

	mp4pub_TRACE_DEBUG(0x1<<0, "SscanfInt]:%s",p);

	return 1;
}

VOID mp4pub_CloseSocket(INT32 *pSocket)
{
	if(pSocket!=NULL && *pSocket>-1){
#ifdef WIN32DLL
		shutdown(*pSocket, SD_BOTH);
		closesocket(*pSocket);
#endif

#ifdef LINUX
		shutdown(*pSocket, SHUT_RDWR);
		close(*pSocket);
#endif
		*pSocket=-1;
	}
}

INT32 mp4pub_CreateThread(PPUB_PTHREAD_START_ROUTINE lpStartAddress, IN VOID* lpParameter, IN INT32 dwCreationFlags, IN CHAR bDetach, OUT VOID **hhThread, OUT pub_threadid* lpThreadId)
{
	int bRet=0;
	void* hThread=0L;
#ifdef WIN32DLL
	hThread=CreateThread(NULL, 0,(PPUB_PTHREAD_START_ROUTINE)lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
	if(hThread==NULL) bRet=-1;
	else if(bDetach) CloseHandle(hThread);
#endif

#ifdef LINUX
	if(pthread_create(lpThreadId, NULL, lpStartAddress, lpParameter) != 0) bRet=-1;
	else if(bDetach) pthread_detach(*lpThreadId);
#endif
	if(hhThread!=NULL) *hhThread=hThread;

	return bRet;
}

void mp4pub_DestroyThread(void** hThread)
{
#ifdef WIN32DLL
	if(*hThread!=NULL) {
		CloseHandle(hThread);
		*hThread=NULL;
	}
#endif
}

VOID mp4pub_TRACE_DEBUG(UINT32 level, const CHAR *format, ...)
{
#if defined(PUB_OUTPUT_LOG)
 pub_mutex_lock(g_lockLog_mp4);
	if((level & g_nDEBUG_LEVELmp4) == 0){
        pub_mutex_unlock(g_lockLog_mp4);
        return;
    }

	static unsigned long gTickout_sep2p=0L;
	static char gFmt_sep2p[1024]={0};
	va_list vlist;
	unsigned long hour=0L, minute=0L, second=0L, ms=0L;
	unsigned long nTmp=0L;
	if(gTickout_sep2p==0L) gTickout_sep2p=mp4pub_GetTickCount();

	ms=(mp4pub_GetTickCount()-gTickout_sep2p);
	nTmp=ms/1000;
	second=nTmp % 60;
	minute=(nTmp/60) % 60;
	hour  =(nTmp/3600) % 24;

    INT32 iLevel = 0;
	while(1)
	{
		if((level>>iLevel)==0)	break;
		iLevel++;
	}
	sprintf(gFmt_sep2p, "%02ld:%02ld:%02ld.%03ld %02d> ", hour, minute,second, ms%1000, iLevel-1);

	va_start(vlist, format);
	//vsnprintf(&fmt[13], 300, format, vlist);
	vsprintf(&gFmt_sep2p[17], format, vlist);
	va_end(vlist);

#if defined(PUB_OUTPUT_LOG_TO_FILE)
	{
		FILE *fp = NULL;
		fp = fopen(PUB_OUTPUT_LOG_TO_FILE,"a+");
		if(fp == NULL) {
			pub_mutex_unlock(g_lockLog_mp4);
			return;
		}
		fprintf(fp,"%s",gFmt_sep2p);
		fclose(fp);
	}
#else
#if defined(WIN32DLL)
	OutputDebugStringA(gFmt_sep2p);
	//printf(gFmt_sep2p);
#else
	printf("%s", gFmt_sep2p);
#endif
#endif
 pub_mutex_unlock(g_lockLog_mp4);
#endif
}


// ===========================================================================
//      SE_Mutex class
// ===========================================================================
#ifdef HAVE_MUTEX
// implemented as inlines in threads.h

	#ifdef HAVE_WIN32_MUTEX
		SE_Mutex::SE_Mutex()
		{
			HANDLE hMutex;
			assert(sizeof(hMutex) == sizeof(mState));

			hMutex = CreateMutex(NULL, FALSE, NULL);
			mState = (void*) hMutex;
		}

		SE_Mutex::SE_Mutex(const char* name)
		{
			// XXX: name not used for now
			HANDLE hMutex;
			assert(sizeof(hMutex) == sizeof(mState));

			hMutex = CreateMutex(NULL, FALSE, NULL);
			mState = (void*) hMutex;
		}

		SE_Mutex::SE_Mutex(int type, const char* name)
		{
			// XXX: type and name not used for now
			HANDLE hMutex;

			assert(sizeof(hMutex) == sizeof(mState));

			hMutex = CreateMutex(NULL, FALSE, NULL);
			mState = (void*) hMutex;
		}

		SE_Mutex::~SE_Mutex()
		{
			CloseHandle((HANDLE) mState);
		}

		status_t SE_Mutex::lock()
		{
			DWORD dwWaitResult;
			dwWaitResult = WaitForSingleObject((HANDLE) mState, INFINITE);
			return dwWaitResult != WAIT_OBJECT_0 ? -1 : NO_ERROR;
		}

		void SE_Mutex::unlock()
		{
			if (!ReleaseMutex((HANDLE) mState)) ;//LOG(LOG_WARN, "thread", "WARNING: bad result from unlocking mutex\n");
		}

		status_t SE_Mutex::tryLock()
		{
			DWORD dwWaitResult;

			dwWaitResult = WaitForSingleObject((HANDLE) mState, 0);
			if (dwWaitResult != WAIT_OBJECT_0 && dwWaitResult != WAIT_TIMEOUT);
				//LOG(LOG_WARN, "thread", "WARNING: bad result from try-locking mutex\n");
			return (dwWaitResult == WAIT_OBJECT_0) ? 0 : -1;
		}


	#elif defined(HAVE_PTHREADS)
		SE_Mutex::SE_Mutex() {
			pthread_mutex_init(&mMutex, NULL);
		}
		SE_Mutex::SE_Mutex(const char* name) {
			pthread_mutex_init(&mMutex, NULL);
		}
		SE_Mutex::SE_Mutex(int type, const char* name) {
			if (type == SHARED) {
				pthread_mutexattr_t attr;
				pthread_mutexattr_init(&attr);
				pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
				pthread_mutex_init(&mMutex, &attr);
				pthread_mutexattr_destroy(&attr);
			} else {
				pthread_mutex_init(&mMutex, NULL);
			}
		}
		SE_Mutex::~SE_Mutex() {
			pthread_mutex_destroy(&mMutex);
		}
		status_t SE_Mutex::lock() {
			return -pthread_mutex_lock(&mMutex);
		}
		void SE_Mutex::unlock() {
			pthread_mutex_unlock(&mMutex);
		}
		status_t SE_Mutex::tryLock() {
			return -pthread_mutex_trylock(&mMutex);
		}
	#else// HAVE_PTHREADS
		#error "Somebody forgot to implement threads for this platform."
	#endif
#endif

