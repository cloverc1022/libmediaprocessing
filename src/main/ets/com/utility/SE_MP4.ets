import nativeFfmpeg from 'libffmpeg.so';
import native from 'libSE_MP4.so';

/** 与 Android {@code com.utility.SE_MP4} 一致（先加载 libffmpeg，再 libSE_MP4） */
export class SE_MP4 {
  static readonly MP4_AV_CODECID_UNKNOWN: number = 0x00;
  static readonly MP4_AV_CODECID_VIDEO_MJPEG: number = 0x01;
  static readonly MP4_AV_CODECID_VIDEO_H264: number = 0x02;
  static readonly MP4_AV_CODECID_VIDEO_H265: number = 0x03;

  static readonly MP4_AV_CODECID_AUDIO_ADPCM: number = 0x100;
  static readonly MP4_AV_CODECID_AUDIO_G726: number = 0x101;
  static readonly MP4_AV_CODECID_AUDIO_G711A: number = 0x102;
  static readonly MP4_AV_CODECID_AUDIO_AAC: number = 0x103;

  private static ensureNativeLoaded(): void {
    void nativeFfmpeg;
    void native;
  }

  static SEMP4_GetSdkVer(chDesc: Uint8Array, nDescMaxSize: number): number {
    SE_MP4.ensureNativeLoaded();
    return native.SEMP4_GetSdkVer(chDesc, nDescMaxSize) as number;
  }

  static SEMP4_Init(strAndroidPackageName: string, nAndroidBuildVersion: number): number {
    SE_MP4.ensureNativeLoaded();
    return native.SEMP4_Init(strAndroidPackageName, nAndroidBuildVersion) as number;
  }

  static SEMP4_Open(
    ppHandle: Int32Array,
    filename: string,
    nWidth: number,
    nHeigh: number,
    nVideoType: number,
    nAudioSampleRate: number,
    nAudioChannel: number,
    nAudioType: number,
    rotate: number
  ): number {
    SE_MP4.ensureNativeLoaded();
    return native.SEMP4_Open(
      ppHandle,
      filename,
      nWidth,
      nHeigh,
      nVideoType,
      nAudioSampleRate,
      nAudioChannel,
      nAudioType,
      rotate
    ) as number;
  }

  static SEMP4_Close(ppHandle: Int32Array): number {
    SE_MP4.ensureNativeLoaded();
    return native.SEMP4_Close(ppHandle) as number;
  }

  static SEMP4_AppendAACHead(
    pHandle: number,
    bytAACDataRecording: Uint8Array,
    nAudioSampleRate: number,
    bytAudioData: Uint8Array,
    iSrcPos: number,
    nAudioDataSize: number
  ): number {
    SE_MP4.ensureNativeLoaded();
    return native.SEMP4_AppendAACHead(
      pHandle,
      bytAACDataRecording,
      nAudioSampleRate,
      bytAudioData,
      iSrcPos,
      nAudioDataSize
    ) as number;
  }

  static SEMP4_AddVideoFrame(
    pHandle: number,
    pData: Uint8Array,
    iSrcPos: number,
    nDataLen: number,
    nTimestamp: number,
    keyframe: number
  ): number {
    SE_MP4.ensureNativeLoaded();
    return native.SEMP4_AddVideoFrame(
      pHandle,
      pData,
      iSrcPos,
      nDataLen,
      nTimestamp,
      keyframe
    ) as number;
  }

  static SEMP4_AddAudioFrame(
    pHandle: number,
    pData: Uint8Array,
    iSrcPos: number,
    nDataLen: number,
    nTimestamp: number
  ): number {
    SE_MP4.ensureNativeLoaded();
    return native.SEMP4_AddAudioFrame(pHandle, pData, iSrcPos, nDataLen, nTimestamp) as number;
  }

  static TS2MP4(
    tsFilelistBytime: string,
    nVideoType: number,
    nAudioType: number,
    nAudioSampleRate: number,
    nAudioChannel: number,
    outFilename: string
  ): number {
    SE_MP4.ensureNativeLoaded();
    return native.TS2MP4(
      tsFilelistBytime,
      nVideoType,
      nAudioType,
      nAudioSampleRate,
      nAudioChannel,
      outFilename
    ) as number;
  }
}
