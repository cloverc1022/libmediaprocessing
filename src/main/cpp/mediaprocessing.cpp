#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <pthread.h>
#include <iostream>
#include <cstring>

#include "mp4_manager.h"
#include "include/mp4.h"

#define API_ER_ANDROID_NULL        -100
#ifndef LOGI
#define LOGI(...)
#endif

#define MAX_NUM_MP4            64
//#define HANDLE_PLACEHOLDER	0x01

static int g_nMP4Init = 0;
static MP4 *g_pMP4[MAX_NUM_MP4];
static pthread_mutex_t g_mutexMP4 = PTHREAD_MUTEX_INITIALIZER;

//==================================================================
int newHandle() {
    int i = 0, nNewHandle = -1;

    pthread_mutex_lock(&g_mutexMP4);
    if (g_nMP4Init == 0) {
        g_nMP4Init++;
        memset(g_pMP4, 0, sizeof(g_pMP4));
    } else g_nMP4Init++;

    for (i = 0; i < MAX_NUM_MP4; i++) {
        if (g_pMP4[i] == NULL) {
            //g_pMP4[i]=HANDLE_PLACEHOLDER;
            nNewHandle = i;
            break;
        }
    }
    if (nNewHandle > -1) g_pMP4[nNewHandle] = new MP4();
    pthread_mutex_unlock(&g_mutexMP4);

    return nNewHandle;
}

/**
 * 取会话指针并持有 g_mutexMP4。无论返回是否为 NULL，都必须调用 unlockHandle()（STAB-03）。
 * 写帧/关闭互斥后，句柄复用也不会指向已删除会话。
 */
static MP4 *lockHandle(int nHandle) {
    pthread_mutex_lock(&g_mutexMP4);
    if (nHandle < 0 || nHandle >= MAX_NUM_MP4) return NULL;
    return g_pMP4[nHandle];
}

static void unlockHandle() {
    pthread_mutex_unlock(&g_mutexMP4);
}

void deleteHandle(int nHandle) {
    int i = 0;
    if (nHandle < 0 || nHandle >= MAX_NUM_MP4) return;

    pthread_mutex_lock(&g_mutexMP4);
    if (g_nMP4Init > 0) g_nMP4Init--;
    if (g_nMP4Init <= 0) {
        g_nMP4Init = 0;

        for (i = 0; i < MAX_NUM_MP4; i++) {
            //if(g_pMP4[i]!=NULL && g_pMP4[i]!=HANDLE_PLACEHOLDER) {
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

extern "C"
JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1GetSdkVer(JNIEnv *env, jclass thiz,
                                           jbyteArray chDesc,
                                           jint nDescMaxSize) {
    jint nVer = 0;
    if (chDesc == NULL) nVer = MP4::SEMP4_GetSdkVer(NULL, 0);
    else {
        if (nDescMaxSize < 128) nVer = MP4::SEMP4_GetSdkVer(NULL, 0);
        else {
            jbyte *jchDesc = NULL;
            if (chDesc != NULL) jchDesc = (jbyte *) env->GetByteArrayElements(chDesc, NULL);
            nVer = MP4::SEMP4_GetSdkVer((char *) jchDesc, nDescMaxSize);
            if (jchDesc != NULL) env->ReleaseByteArrayElements(chDesc, jchDesc, 0);
        }
    }
    return nVer;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1Init(JNIEnv *env, jclass thiz,
                                      jstring strAndroidPackageName,
                                      jint nAndroidBuildVersion) {
    jint nRet = 0;
    if (strAndroidPackageName == NULL) return API_ER_ANDROID_NULL;
    char *jstrAndroidPackageName = (char *) env->GetStringUTFChars(strAndroidPackageName, 0);
    nRet = SEMP4_Init(jstrAndroidPackageName, nAndroidBuildVersion);
    if (jstrAndroidPackageName)
        env->ReleaseStringUTFChars(strAndroidPackageName, jstrAndroidPackageName);

    return nRet;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1Open(JNIEnv *env, jclass thiz,
                                      jintArray ppHandle,
                                      jstring filename, jint nWidth,
                                      jint nHeigh, jint nVideoType,
                                      jint nAudioSampleRate,
                                      jint nAudioChannel,
                                      jint nAudioType,
                                      jint nRotateVideo) {
    jint nRet = 0, nHandle = -1;
    if (ppHandle == NULL || filename == NULL) return API_ER_ANDROID_NULL;
    jint *jppHandle = (jint *) env->GetIntArrayElements(ppHandle, NULL);

#ifdef LINUX_ANDROID
    nHandle=newHandle();
        *jppHandle=nHandle;
        if(nHandle<0) nRet=API_ER_ANDROID_NULL;
        else {
            char *jfilename=(char*)env->GetStringUTFChars(filename,0);
            MP4 *pMp4 = lockHandle(nHandle);
            if (pMp4 != NULL) {
                nRet = pMp4->OpenMp4File(jfilename, nWidth, nHeigh, nVideoType, nAudioSampleRate,nAudioChannel,nAudioType, nRotateVideo);
            } else {
                nRet = API_ER_ANDROID_NULL;
            }
            unlockHandle();
            if(jfilename)env->ReleaseStringUTFChars(filename, jfilename);
        }
        LOGI("SEMP4_Open,jni] g_nMP4Init=%d, *jppHandle=%d\n", g_nMP4Init, *jppHandle);
#endif
    if (jppHandle != NULL)env->ReleaseIntArrayElements(ppHandle, jppHandle, 0);

    return nRet;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1Close(JNIEnv *env, jclass thiz,
                                       jintArray ppHandle) {
    if (ppHandle == NULL) return API_ER_ANDROID_NULL;
    jint *jppHandle = (jint *) env->GetIntArrayElements(ppHandle, NULL);
#ifdef LINUX_ANDROID
    {
        // Close 与写帧同一把锁：等在飞写帧结束后再删对象（STAB-03）
        MP4 *pMp4 = lockHandle(*jppHandle);
        if (pMp4 != NULL) {
            pMp4->CloseMp4File();
        }
        unlockHandle();
        deleteHandle(*jppHandle);
    }
#endif
    if (jppHandle != NULL)env->ReleaseIntArrayElements(ppHandle, jppHandle, 0);
    return 1;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1AppendAACHead(JNIEnv *env, jclass thiz,
                                               jint pHandle,
                                               jbyteArray bytAACDataRecording,
                                               jint nAudioSampleRate,
                                               jbyteArray bytAudioData,
                                               jint iSrcPos,
                                               jint nAudioDataSize) {
    jint nRet = 0;
    if (pHandle < 0 || pHandle >= MAX_NUM_MP4) return API_ER_ANDROID_NULL;
    if (bytAACDataRecording == NULL || bytAudioData == NULL) return API_ER_ANDROID_NULL;

    jbyte *jbytAACDataRecording = (jbyte *) env->GetByteArrayElements(bytAACDataRecording, 0);
    jbyte *jbytAudioData = (jbyte *) env->GetByteArrayElements(bytAudioData, 0);

    MP4 *pMp4 = lockHandle(pHandle);
    if (pMp4 != NULL) {
        nRet = SEMP4_AppendAACHead((UCHAR *) pMp4, (CHAR *) jbytAACDataRecording,
                                   nAudioSampleRate, (CHAR *) jbytAudioData, iSrcPos,
                                   nAudioDataSize);
    }
    unlockHandle();

    env->ReleaseByteArrayElements(bytAudioData, jbytAudioData, 0);
    env->ReleaseByteArrayElements(bytAACDataRecording, jbytAACDataRecording, 0);
    return nRet;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1AddVideoFrame(JNIEnv *env, jclass thiz,
                                               jint pHandle,
                                               jbyteArray pData,
                                               jint iSrcPos,
                                               jint nDataLen,
                                               jlong nTimestamp,
                                               jint keyframe) {
    jint nRet = 0;
    if (pHandle < 0 || pHandle >= MAX_NUM_MP4) return API_ER_ANDROID_NULL;

    jbyte *jpData = (jbyte *) env->GetByteArrayElements(pData, 0);
    MP4 *pMp4 = lockHandle(pHandle);
    if (pMp4 != NULL) {
        nRet = pMp4->AddVideoFrame((UCHAR *) jpData, iSrcPos, nDataLen, nTimestamp,
                                   keyframe);
    }
    unlockHandle();
    env->ReleaseByteArrayElements(pData, jpData, 0);
    return nRet;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1AddAudioFrame(JNIEnv *env, jclass thiz,
                                               jint pHandle,
                                               jbyteArray pData,
                                               jint iSrcPos,
                                               jint nDataLen,
                                               jlong nTimestamp) {
    jint nRet = 0;
    if (pHandle < 0 || pHandle >= MAX_NUM_MP4) return API_ER_ANDROID_NULL;

    jbyte *jpData = (jbyte *) env->GetByteArrayElements(pData, 0);
    MP4 *pMp4 = lockHandle(pHandle);
    if (pMp4 != NULL) {
        nRet = pMp4->AddAudioFrame((UCHAR *) jpData, iSrcPos, nDataLen, nTimestamp);
    }
    unlockHandle();
    env->ReleaseByteArrayElements(pData, jpData, 0);
    return nRet;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_TS2MP4(JNIEnv *env, jclass thiz,
                                 jstring tsFilelistBytime,
                                 jint nVideoType, jint nAudioType,
                                 jint nAudioSampleRate,
                                 jint nAudioChannel,
                                 jstring outFilename) {
    jint nRet = 0;
    if (tsFilelistBytime == NULL || outFilename == NULL) return API_ER_ANDROID_NULL;

    const char *jtsFilelistBytime = env->GetStringUTFChars(tsFilelistBytime, 0);
    const char *joutFilename = env->GetStringUTFChars(outFilename, 0);
    if (jtsFilelistBytime == NULL || joutFilename == NULL) {
        if (joutFilename)env->ReleaseStringUTFChars(outFilename, joutFilename);
        if (jtsFilelistBytime)env->ReleaseStringUTFChars(tsFilelistBytime, jtsFilelistBytime);
        return API_ER_ANDROID_NULL;
    }

    nRet = TS2MP4((CHAR *) jtsFilelistBytime, nVideoType, nAudioType, nAudioSampleRate,
                  nAudioChannel, (CHAR *) joutFilename, 0);

    if (joutFilename)env->ReleaseStringUTFChars(outFilename, joutFilename);
    if (jtsFilelistBytime)env->ReleaseStringUTFChars(tsFilelistBytime, jtsFilelistBytime);

    return nRet;
}