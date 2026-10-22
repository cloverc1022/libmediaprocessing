#include <jni.h>

#include "sem4_native.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1GetSdkVer(JNIEnv *env, jclass thiz, jbyteArray chDesc,
                                           jint nDescMaxSize) {
    if (chDesc == NULL) {
        return sem4_getSdkVer(NULL, 0);
    }
    if (nDescMaxSize < 128) {
        return sem4_getSdkVer(NULL, 0);
    }
    jbyte *jchDesc = env->GetByteArrayElements(chDesc, NULL);
    jint nVer = sem4_getSdkVer((char *) jchDesc, nDescMaxSize);
    if (jchDesc != NULL) {
        env->ReleaseByteArrayElements(chDesc, jchDesc, 0);
    }
    return nVer;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1Init(JNIEnv *env, jclass thiz, jstring strAndroidPackageName,
                                      jint nAndroidBuildVersion) {
    if (strAndroidPackageName == NULL) {
        return API_ER_NULL;
    }
    char *name = (char *) env->GetStringUTFChars(strAndroidPackageName, 0);
    jint nRet = sem4_init(name, nAndroidBuildVersion);
    if (name) {
        env->ReleaseStringUTFChars(strAndroidPackageName, name);
    }
    return nRet;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1Open(JNIEnv *env, jclass thiz, jintArray ppHandle,
                                      jstring filename, jint nWidth, jint nHeigh, jint nVideoType,
                                      jint nAudioSampleRate, jint nAudioChannel, jint nAudioType,
                                      jint nRotateVideo) {
    if (ppHandle == NULL || filename == NULL) {
        return API_ER_NULL;
    }
    jint *jppHandle = env->GetIntArrayElements(ppHandle, NULL);
    char *jfilename = (char *) env->GetStringUTFChars(filename, 0);
    int handle = 0;
    jint nRet = sem4_open(&handle, jfilename, nWidth, nHeigh, nVideoType, nAudioSampleRate,
                          nAudioChannel, nAudioType, nRotateVideo);
    if (jppHandle != NULL) {
        *jppHandle = handle;
    }
    if (jfilename) {
        env->ReleaseStringUTFChars(filename, jfilename);
    }
    if (jppHandle != NULL) {
        env->ReleaseIntArrayElements(ppHandle, jppHandle, 0);
    }
    return nRet;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1Close(JNIEnv *env, jclass thiz, jintArray ppHandle) {
    if (ppHandle == NULL) {
        return API_ER_NULL;
    }
    jint *jppHandle = env->GetIntArrayElements(ppHandle, NULL);
    if (jppHandle != NULL) {
        sem4_close(jppHandle);
        env->ReleaseIntArrayElements(ppHandle, jppHandle, 0);
    }
    return 1;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1AppendAACHead(JNIEnv *env, jclass thiz, jint pHandle,
                                               jbyteArray bytAACDataRecording,
                                               jint nAudioSampleRate, jbyteArray bytAudioData,
                                               jint iSrcPos, jint nAudioDataSize) {
    jbyte *aac = env->GetByteArrayElements(bytAACDataRecording, 0);
    jbyte *audio = env->GetByteArrayElements(bytAudioData, 0);
    jint nRet = sem4_appendAacHead(pHandle, (char *) aac, nAudioSampleRate, (char *) audio, iSrcPos,
                                   nAudioDataSize);
    env->ReleaseByteArrayElements(bytAudioData, audio, 0);
    env->ReleaseByteArrayElements(bytAACDataRecording, aac, 0);
    return nRet;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1AddVideoFrame(JNIEnv *env, jclass thiz, jint pHandle,
                                               jbyteArray pData, jint iSrcPos, jint nDataLen,
                                               jlong nTimestamp, jint keyframe) {
    jbyte *jpData = env->GetByteArrayElements(pData, 0);
    jint nRet =
            sem4_addVideoFrame(pHandle, (const char *) jpData, iSrcPos, nDataLen, nTimestamp, keyframe);
    env->ReleaseByteArrayElements(pData, jpData, 0);
    return nRet;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_SEMP4_1AddAudioFrame(JNIEnv *env, jclass thiz, jint pHandle,
                                               jbyteArray pData, jint iSrcPos, jint nDataLen,
                                               jlong nTimestamp) {
    jbyte *jpData = env->GetByteArrayElements(pData, 0);
    jint nRet = sem4_addAudioFrame(pHandle, (const char *) jpData, iSrcPos, nDataLen, nTimestamp);
    env->ReleaseByteArrayElements(pData, jpData, 0);
    return nRet;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_utility_SE_1MP4_TS2MP4(JNIEnv *env, jclass thiz, jstring tsFilelistBytime,
                                 jint nVideoType, jint nAudioType, jint nAudioSampleRate,
                                 jint nAudioChannel, jstring outFilename) {
    if (tsFilelistBytime == NULL || outFilename == NULL) {
        return API_ER_NULL;
    }
    const char *ts = env->GetStringUTFChars(tsFilelistBytime, 0);
    const char *out = env->GetStringUTFChars(outFilename, 0);
    if (ts == NULL || out == NULL) {
        if (out) {
            env->ReleaseStringUTFChars(outFilename, out);
        }
        if (ts) {
            env->ReleaseStringUTFChars(tsFilelistBytime, ts);
        }
        return API_ER_NULL;
    }
    jint nRet = sem4_ts2mp4(ts, nVideoType, nAudioType, nAudioSampleRate, nAudioChannel, out);
    env->ReleaseStringUTFChars(outFilename, out);
    env->ReleaseStringUTFChars(tsFilelistBytime, ts);
    return nRet;
}
