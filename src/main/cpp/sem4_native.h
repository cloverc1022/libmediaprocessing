#ifndef SEM4_NATIVE_H
#define SEM4_NATIVE_H

#include <cstdint>

#define API_ER_NULL -100

int sem4_getSdkVer(char *desc, int descMaxSize);

int sem4_init(const char *packageName, int buildVersion);

int sem4_open(int *outHandle, const char *filename, int width, int height, int videoType,
              int audioSampleRate, int audioChannel, int audioType, int rotateVideo);

int sem4_close(int *handle);

int sem4_appendAacHead(int handle, char *aacDataRecording, int audioSampleRate, char *audioData,
                       int srcPos, int audioDataSize);

int sem4_addVideoFrame(int handle, const char *data, int srcPos, int dataLen, int64_t timestamp,
                       int keyframe);

int sem4_addAudioFrame(int handle, const char *data, int srcPos, int dataLen, int64_t timestamp);

int sem4_ts2mp4(const char *tsFilelistBytime, int videoType, int audioType, int audioSampleRate,
                int audioChannel, const char *outFilename);

#endif
