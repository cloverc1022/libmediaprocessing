package com.utility;

public class SE_MP4 {
    static {
        try {
            System.loadLibrary("ffmpeg");
            System.loadLibrary("SE_MP4");
        } catch (UnsatisfiedLinkError ule) {
            System.out.println("loadLibrary(SE_MP4)," + ule.getMessage());
        }
    }

    public static final int MP4_AV_CODECID_UNKNOWN = 0x00;
    public static final int MP4_AV_CODECID_VIDEO_MJPEG = 0x01;
    public static final int MP4_AV_CODECID_VIDEO_H264 = 0x02;
    public static final int MP4_AV_CODECID_VIDEO_H265 = 0x03;

    public static final int MP4_AV_CODECID_AUDIO_ADPCM = 0x100;
    public static final int MP4_AV_CODECID_AUDIO_G726 = 0x101;
    public static final int MP4_AV_CODECID_AUDIO_G711A = 0x102;
    public static final int MP4_AV_CODECID_AUDIO_AAC = 0x103;

    public native static int SEMP4_GetSdkVer(byte[] chDesc, int nDescMaxSize);

    // strAndroidPackageName is the name of this application's package.
    public native static int SEMP4_Init(String strAndroidPackageName, int nAndroidBuildVersion);

    public native static int SEMP4_Open(
            int[] ppHandle,
            String filename,
            int nWidth,
            int nHeigh,
            int nVideoType,
            int nAudioSampleRate,
            int nAudioChannel,
            int nAudioType,
            int rotate
    );

    public native static int SEMP4_Close(int[] ppHandle);

    public native static int SEMP4_AppendAACHead(
            int pHandle,
            byte[] bytAACDataRecording,
            int nAudioSampleRate,
            byte[] bytAudioData,
            int iSrcPos,
            int nAudioDataSize
    );

    public native static int SEMP4_AddVideoFrame(
            int pHandle,
            byte[] pData,
            int iSrcPos,
            int nDataLen,
            long nTimestamp,
            int keyframe
    );

    public native static int SEMP4_AddAudioFrame(
            int pHandle,
            byte[] pData,
            int iSrcPos,
            int nDataLen,
            long nTimestamp
    );

    public synchronized native static int TS2MP4(
            String tsFilelistBytime,
            int nVideoType,
            int nAudioType,
            int nAudioSampleRate,
            int nAudioChannel,
            String outFilename
    );
}
