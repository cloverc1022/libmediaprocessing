package net.ajcloud.mediaprocessing

import android.util.Log
import com.utility.SE_MP4

object MediaProcessing {
    private const val TAG = "MediaProcessing"
    const val MP4_AV_CODECID_UNKNOWN: Int = 0x00
    const val MP4_AV_CODECID_VIDEO_MJPEG: Int = 0x01
    const val MP4_AV_CODECID_VIDEO_H264: Int = 0x02
    const val MP4_AV_CODECID_VIDEO_H265: Int = 0x03
    const val MP4_AV_CODECID_AUDIO_ADPCM: Int = 0x100
    const val MP4_AV_CODECID_AUDIO_G726: Int = 0x101
    const val MP4_AV_CODECID_AUDIO_G711A: Int = 0x102
    const val MP4_AV_CODECID_AUDIO_AAC: Int = 0x103

    fun SEMP4_GetSdkVer(chDesc: ByteArray, nDescMaxSize: Int): Int =
        SE_MP4.SEMP4_GetSdkVer(chDesc, nDescMaxSize)

    //strAndroidPackageName is the name of this application's package
    fun SEMP4_Init(strAndroidPackageName: String, nAndroidBuildVersion: Int): Int =
        SE_MP4.SEMP4_Init(strAndroidPackageName, nAndroidBuildVersion)

    //	public native static int SEMP4_Open(int[] ppHandle, String filename, int nWidth, int nHeigh, int nVideoType,
    //										int nAudioSampleRate, int nAudioChannel, int nAudioType);
    fun SEMP4_Open(
        ppHandle: IntArray?, filename: String, nWidth: Int, nHeigh: Int, nVideoType: Int,
        nAudioSampleRate: Int, nAudioChannel: Int, nAudioType: Int, rotate: Int
    ): Int {
        val ret = SE_MP4.SEMP4_Open(
            ppHandle, filename, nWidth, nHeigh, nVideoType,
            nAudioSampleRate, nAudioChannel, nAudioType, rotate
        )
        Log.d(
            TAG,
            "SEMP4_Open ret=$ret handle=${ppHandle?.getOrNull(0)} file=$filename " +
                "videoType=$nVideoType audioType=$nAudioType sampleRate=$nAudioSampleRate"
        )
        return ret
    }

    fun SEMP4_Close(ppHandle: IntArray): Int {
        val ret = SE_MP4.SEMP4_Close(ppHandle)
        Log.d(TAG, "SEMP4_Close ret=$ret handle=${ppHandle.getOrNull(0)}")
        return ret
    }

    //parameter: (for mono audio)
    //	nSampleRate is among of g_nSampleRate
    //		static INT32 g_nSampleRate[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
    //	pBufOut: the buffer outputing. its size is at least (7+nSizeOfES);
    //
    fun SEMP4_AppendAACHead(
        pHandle: Int,
        bytAACDataRecording: ByteArray,
        nAudioSampleRate: Int,
        bytAudioData: ByteArray,
        iSrcPos: Int,
        nAudioDataSize: Int
    ): Int = SE_MP4.SEMP4_AppendAACHead(
        pHandle, bytAACDataRecording, nAudioSampleRate, bytAudioData, iSrcPos, nAudioDataSize
    )

    fun SEMP4_AddVideoFrame(
        pHandle: Int,
        pData: ByteArray,
        iSrcPos: Int,
        nDataLen: Int,
        nTimestamp: Long,
        keyframe: Int
    ): Int {
        val ret = SE_MP4.SEMP4_AddVideoFrame(pHandle, pData, iSrcPos, nDataLen, nTimestamp, keyframe)
        if (ret != 0) {
            Log.w(
                TAG,
                "SEMP4_AddVideoFrame ret=$ret handle=$pHandle ts=$nTimestamp key=$keyframe len=$nDataLen"
            )
        }
        return ret
    }

    fun SEMP4_AddAudioFrame(
        pHandle: Int,
        pData: ByteArray,
        iSrcPos: Int,
        nDataLen: Int,
        nTimestamp: Long
    ): Int {
        val ret = SE_MP4.SEMP4_AddAudioFrame(pHandle, pData, iSrcPos, nDataLen, nTimestamp)
        if (ret != 0 && ret != 1) {
            Log.w(
                TAG,
                "SEMP4_AddAudioFrame ret=$ret handle=$pHandle ts=$nTimestamp len=$nDataLen " +
                    descNativeAudioRet(ret)
            )
        }
        return ret
    }

    /** 对应 mp4_manager.cpp AddAudioFrame 返回值 */
    private fun descNativeAudioRet(ret: Int): String = when (ret) {
        1 -> "g711_buffering"
        -2 -> "oc_null"
        -3 -> "audio_ts_before_video_start"
        -4 -> "video_header_not_written"
        -5 -> "audio_stream_null"
        -6 -> "not_opened"
        -100 -> "g711_to_aac_failed"
        else -> "other"
    }

    fun TS2MP4(
        tsFileListByTime: String,
        nVideoType: Int,
        nAudioType: Int,
        nAudioSampleRate: Int,
        nAudioChannel: Int,
        outFilename: String?
    ): Int = SE_MP4.TS2MP4(
        tsFileListByTime, nVideoType, nAudioType, nAudioSampleRate, nAudioChannel, outFilename
    )
}