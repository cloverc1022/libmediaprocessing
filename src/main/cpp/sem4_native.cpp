#include "sem4_native.h"

#include <pthread.h>
#include <cstring>

#include "mp4_manager.h"
#include "include/mp4.h"

#ifndef LOGI
#define LOGI(...)
#endif

#define MAX_NUM_MP4 64

static int g_nMP4Init = 0;
static MP4 *g_pMP4[MAX_NUM_MP4];
static pthread_mutex_t g_mutexMP4 = PTHREAD_MUTEX_INITIALIZER;

static int newHandle() {
    int i = 0, nNewHandle = -1;
    pthread_mutex_lock(&g_mutexMP4);
    if (g_nMP4Init == 0) {
        g_nMP4Init++;
        memset(g_pMP4, 0, sizeof(g_pMP4));
    } else {
        g_nMP4Init++;
    }
    for (i = 0; i < MAX_NUM_MP4; i++) {
        if (g_pMP4[i] == NULL) {
            nNewHandle = i;
            break;
        }
    }
    if (nNewHandle > -1) {
        g_pMP4[nNewHandle] = new MP4();
    }
    pthread_mutex_unlock(&g_mutexMP4);
    return nNewHandle;
}

static MP4 *lockHandle(int nHandle) {
    pthread_mutex_lock(&g_mutexMP4);
    if (nHandle < 0 || nHandle >= MAX_NUM_MP4) {
        return NULL;
    }
    return g_pMP4[nHandle];
}

static void unlockHandle() {
    pthread_mutex_unlock(&g_mutexMP4);
}

static void deleteHandle(int nHandle) {
    int i = 0;
    if (nHandle < 0 || nHandle >= MAX_NUM_MP4) {
        return;
    }
    pthread_mutex_lock(&g_mutexMP4);
    if (g_nMP4Init > 0) {
        g_nMP4Init--;
    }
    if (g_nMP4Init <= 0) {
        g_nMP4Init = 0;
        for (i = 0; i < MAX_NUM_MP4; i++) {
            if (g_pMP4[i] != NULL) {
                delete g_pMP4[i];
                g_pMP4[i] = NULL;
            }
        }
    }
    if (g_pMP4[nHandle] != NULL) {
        delete g_pMP4[nHandle];
        g_pMP4[nHandle] = NULL;
    }
    pthread_mutex_unlock(&g_mutexMP4);
}

int sem4_getSdkVer(char *desc, int descMaxSize) {
    return MP4::SEMP4_GetSdkVer(desc, descMaxSize);
}

int sem4_init(const char *packageName, int buildVersion) {
    if (packageName == NULL) {
        return API_ER_NULL;
    }
    return SEMP4_Init((char *) packageName, buildVersion);
}

int sem4_open(int *outHandle, const char *filename, int width, int height, int videoType,
              int audioSampleRate, int audioChannel, int audioType, int rotateVideo) {
    if (outHandle == NULL || filename == NULL) {
        return API_ER_NULL;
    }
#if defined(LINUX_ANDROID) || defined(BUILD_FOR_OHOS)
    int nHandle = newHandle();
    *outHandle = nHandle;
    if (nHandle < 0) {
        return API_ER_NULL;
    }
    MP4 *pMp4 = lockHandle(nHandle);
    int nRet = API_ER_NULL;
    if (pMp4 != NULL) {
        nRet = pMp4->OpenMp4File((char *) filename, width, height, videoType, audioSampleRate,
                                 audioChannel, audioType, rotateVideo);
    }
    unlockHandle();
    LOGI("SEMP4_Open native g_nMP4Init=%d, handle=%d\n", g_nMP4Init, *outHandle);
    return nRet;
#else
    (void) filename;
    (void) width;
    (void) height;
    (void) videoType;
    (void) audioSampleRate;
    (void) audioChannel;
    (void) audioType;
    (void) rotateVideo;
    return API_ER_NULL;
#endif
}

int sem4_close(int *handle) {
    if (handle == NULL) {
        return API_ER_NULL;
    }
#if defined(LINUX_ANDROID) || defined(BUILD_FOR_OHOS)
    MP4 *pMp4 = lockHandle(*handle);
    if (pMp4 != NULL) {
        pMp4->CloseMp4File();
    }
    unlockHandle();
    deleteHandle(*handle);
#endif
    return 1;
}

int sem4_appendAacHead(int handle, char *aacDataRecording, int audioSampleRate, char *audioData,
                       int srcPos, int audioDataSize) {
    if (handle < 0 || handle >= MAX_NUM_MP4) {
        return API_ER_NULL;
    }
    if (aacDataRecording == NULL || audioData == NULL) {
        return API_ER_NULL;
    }
    MP4 *pMp4 = lockHandle(handle);
    int nRet = 0;
    if (pMp4 != NULL) {
        nRet = SEMP4_AppendAACHead((UCHAR *) pMp4, (CHAR *) aacDataRecording, audioSampleRate,
                                   (CHAR *) audioData, srcPos, audioDataSize);
    }
    unlockHandle();
    return nRet;
}

int sem4_addVideoFrame(int handle, const char *data, int srcPos, int dataLen, int64_t timestamp,
                       int keyframe) {
    if (handle < 0 || handle >= MAX_NUM_MP4) {
        return API_ER_NULL;
    }
    MP4 *pMp4 = lockHandle(handle);
    int nRet = 0;
    if (pMp4 != NULL) {
        nRet = pMp4->AddVideoFrame((UCHAR *) data, srcPos, dataLen, timestamp, keyframe);
    }
    unlockHandle();
    return nRet;
}

int sem4_addAudioFrame(int handle, const char *data, int srcPos, int dataLen, int64_t timestamp) {
    if (handle < 0 || handle >= MAX_NUM_MP4) {
        return API_ER_NULL;
    }
    MP4 *pMp4 = lockHandle(handle);
    int nRet = 0;
    if (pMp4 != NULL) {
        nRet = pMp4->AddAudioFrame((UCHAR *) data, srcPos, dataLen, timestamp);
    }
    unlockHandle();
    return nRet;
}

int sem4_ts2mp4(const char *tsFilelistBytime, int videoType, int audioType, int audioSampleRate,
                int audioChannel, const char *outFilename) {
    if (tsFilelistBytime == NULL || outFilename == NULL) {
        return API_ER_NULL;
    }
    return TS2MP4((CHAR *) tsFilelistBytime, videoType, audioType, audioSampleRate, audioChannel,
                  (CHAR *) outFilename, 0);
}
