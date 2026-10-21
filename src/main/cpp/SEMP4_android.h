#include <jni.h>

#ifndef _Included_com_utility_SE_MP4
#define _Included_com_utility_SE_MP4


//#define JNI_DEBUG
#ifdef LINUX_ANDROID
	#ifdef JNI_DEBUG
		#ifndef LOG_TAG
			#define LOG_TAG "JNI_DEBUG"
		#endif

		#include <android/log.h>
		#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
		#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
		#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

	#else
		#define LOGE(...) 
		#define LOGI(...) 
		#define LOGD(...) 
	#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4_GetSdkVer
 * Signature: ([BI)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4_1GetSdkVer
  (JNIEnv *env, jclass clazThis, jbyteArray chDesc, jint nDescMaxSize);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4_Init
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4_1Init
  (JNIEnv *env, jclass clazThis, jstring strAndroidPackageName, jint nAndroidBuildVersion);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4_Open
 * Signature: ([ILjava/lang/String;IIII)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4_1Open
  (JNIEnv *env, jclass clazThis, jintArray ppHandle, jstring filename, jint nWidth, jint nHeigh, jint nVideoType, jint nAudioSampleRate, jint nAudioChannel, jint nAudioType,jint nRotateVideo);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4_Close
 * Signature: ([I)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4_1Close
  (JNIEnv *env, jclass clazThis, jintArray ppHandle);


/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4_AppendAACHead
 * Signature: (I[BI[BI)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4_1AppendAACHead
  (JNIEnv *env, jclass clazThis, jint pHandle, jbyteArray bytAACDataRecording, jint nAudioSampleRate, jbyteArray bytAudioData,jint iSrcPos, jint nAudioDataSize);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4_AddVideoFrame
 * Signature: (I[BIJI)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4_1AddVideoFrame
  (JNIEnv *env, jclass clazThis, jint pHandle, jbyteArray pData,jint iSrcPos, jint nDataLen, jlong nTimestamp, jint keyframe);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4_AddAudioFrame
 * Signature: (I[BIJ)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4_1AddAudioFrame
  (JNIEnv *env, jclass clazThis, jint pHandle, jbyteArray pData,jint iSrcPos, jint nDataLen, jlong nTimestamp);

/*
 * Class:     com_utility_SE_MP4
 * Method:    TS2MP4
 * Signature: (Ljava/lang/String;IIIILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_TS2MP4
  (JNIEnv *, jclass, jstring, jint, jint, jint, jint, jstring);






/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4Read_Create
 * Signature: ([I)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4Read_1Create
  (JNIEnv *env, jclass clazThis, jintArray ppHandle);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4Read_Destroy
 * Signature: ([I)V
 */
JNIEXPORT void JNICALL Java_com_utility_SE_1MP4_SEMP4Read_1Destroy
  (JNIEnv *env, jclass clazThis, jintArray ppHandle);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4Read_OpenMp4File
 * Signature: (ILjava/lang/String;Lcom/utility/FILE_INFO;)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4Read_1OpenMp4File
  (JNIEnv *env, jclass clazThis, jint pHandle, jstring pFilename, jobject pFileInfo);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4Read_CloseMp4File
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4Read_1CloseMp4File
  (JNIEnv *env, jclass clazThis, jint pHandle);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4Read_SetBeginPos
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4Read_1SetBeginPos
  (JNIEnv *env, jclass clazThis, jint pHandle, jint nBeginPos_sec);

/*
 * Class:     com_utility_SE_MP4
 * Method:    SEMP4Read_ReadOneFrame
 * Signature: (I[BI)I
 */
JNIEXPORT jint JNICALL Java_com_utility_SE_1MP4_SEMP4Read_1ReadOneFrame
  (JNIEnv *env, jclass clazThis, jint pHandle, jbyteArray pBuf, jint nBufMaxSize);

#ifdef __cplusplus
}
#endif
#endif
