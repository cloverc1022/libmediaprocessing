#include "napi/native_api.h"

#include <cstdint>
#include <string>
#include <vector>

#include "sem4_native.h"

static bool getInt32(napi_env env, napi_value value, int32_t *out) {
    return napi_get_value_int32(env, value, out) == napi_ok;
}

static bool getInt64(napi_env env, napi_value value, int64_t *out) {
    return napi_get_value_int64(env, value, out) == napi_ok;
}

static bool getString(napi_env env, napi_value value, std::string *out) {
    size_t len = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &len) != napi_ok) {
        return false;
    }
    std::vector<char> buf(len + 1, '\0');
    size_t copied = 0;
    if (napi_get_value_string_utf8(env, value, buf.data(), buf.size(), &copied) != napi_ok) {
        return false;
    }
    out->assign(buf.data(), copied);
    return true;
}

static bool getArrayBuffer(napi_env env, napi_value value, void **data, size_t *length) {
    bool isArrayBuffer = false;
    if (napi_is_arraybuffer(env, value, &isArrayBuffer) == napi_ok && isArrayBuffer) {
        return napi_get_arraybuffer_info(env, value, data, length) == napi_ok;
    }
    napi_typedarray_type type;
    napi_value arrayBuffer;
    size_t byteOffset = 0;
    if (napi_get_typedarray_info(env, value, &type, length, data, &arrayBuffer, &byteOffset) !=
        napi_ok) {
        return false;
    }
    return type == napi_uint8_array || type == napi_int8_array;
}

static bool getInt32ArrayElement(napi_env env, napi_value arr, uint32_t index, int32_t *out) {
    napi_value elem;
    if (napi_get_element(env, arr, index, &elem) != napi_ok) {
        return false;
    }
    return getInt32(env, elem, out);
}

static napi_value SEMP4_GetSdkVer(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    void *desc = nullptr;
    size_t descLen = 0;
    int32_t maxSize = 0;
    napi_valuetype arg0Type;
    if (argc < 2 || napi_typeof(env, args[0], &arg0Type) != napi_ok ||
        arg0Type == napi_null || arg0Type == napi_undefined) {
        int ver = sem4_getSdkVer(nullptr, 0);
        napi_value result;
        napi_create_int32(env, ver, &result);
        return result;
    }
    if (!getArrayBuffer(env, args[0], &desc, &descLen) || !getInt32(env, args[1], &maxSize)) {
        napi_value zero;
        napi_create_int32(env, 0, &zero);
        return zero;
    }
    if (maxSize < 128) {
        int ver = sem4_getSdkVer(nullptr, 0);
        napi_value result;
        napi_create_int32(env, ver, &result);
        return result;
    }
    int ver = sem4_getSdkVer((char *) desc, maxSize);
    napi_value result;
    napi_create_int32(env, ver, &result);
    return result;
}

static napi_value SEMP4_Init(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    std::string pkg;
    int32_t ver = 0;
    if (!getString(env, args[0], &pkg) || !getInt32(env, args[1], &ver)) {
        napi_value err;
        napi_create_int32(env, API_ER_NULL, &err);
        return err;
    }
    napi_value result;
    napi_create_int32(env, sem4_init(pkg.c_str(), ver), &result);
    return result;
}

static napi_value SEMP4_Open(napi_env env, napi_callback_info info) {
    size_t argc = 9;
    napi_value args[9];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    std::string filename;
    int32_t width = 0, height = 0, videoType = 0, audioSampleRate = 0, audioChannel = 0,
            audioType = 0, rotateVideo = 0;
    if (!getString(env, args[1], &filename) || !getInt32(env, args[2], &width) ||
        !getInt32(env, args[3], &height) || !getInt32(env, args[4], &videoType) ||
        !getInt32(env, args[5], &audioSampleRate) || !getInt32(env, args[6], &audioChannel) ||
        !getInt32(env, args[7], &audioType) || !getInt32(env, args[8], &rotateVideo)) {
        napi_value err;
        napi_create_int32(env, API_ER_NULL, &err);
        return err;
    }
    int handle = 0;
    int ret = sem4_open(&handle, filename.c_str(), width, height, videoType, audioSampleRate,
                        audioChannel, audioType, rotateVideo);
    napi_value handleVal;
    napi_create_int32(env, handle, &handleVal);
    napi_set_element(env, args[0], 0, handleVal);
    napi_value result;
    napi_create_int32(env, ret, &result);
    return result;
}

static napi_value SEMP4_Close(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    int32_t handle = 0;
    if (!getInt32ArrayElement(env, args[0], 0, &handle)) {
        napi_value err;
        napi_create_int32(env, API_ER_NULL, &err);
        return err;
    }
    sem4_close(&handle);
    napi_value handleVal;
    napi_create_int32(env, handle, &handleVal);
    napi_set_element(env, args[0], 0, handleVal);
    napi_value one;
    napi_create_int32(env, 1, &one);
    return one;
}

static napi_value SEMP4_AppendAACHead(napi_env env, napi_callback_info info) {
    size_t argc = 6;
    napi_value args[6];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    int32_t handle = 0, sampleRate = 0, srcPos = 0, dataSize = 0;
    void *aac = nullptr;
    void *audio = nullptr;
    size_t aacLen = 0, audioLen = 0;
    if (!getInt32(env, args[0], &handle) || !getArrayBuffer(env, args[1], &aac, &aacLen) ||
        !getInt32(env, args[2], &sampleRate) || !getArrayBuffer(env, args[3], &audio, &audioLen) ||
        !getInt32(env, args[4], &srcPos) || !getInt32(env, args[5], &dataSize)) {
        napi_value err;
        napi_create_int32(env, API_ER_NULL, &err);
        return err;
    }
    napi_value result;
    napi_create_int32(env, sem4_appendAacHead(handle, (char *) aac, sampleRate, (char *) audio,
                                              srcPos, dataSize),
                      &result);
    return result;
}

static napi_value SEMP4_AddVideoFrame(napi_env env, napi_callback_info info) {
    size_t argc = 6;
    napi_value args[6];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    int32_t handle = 0, srcPos = 0, dataLen = 0, keyframe = 0;
    int64_t timestamp = 0;
    void *data = nullptr;
    size_t bufLen = 0;
    if (!getInt32(env, args[0], &handle) || !getArrayBuffer(env, args[1], &data, &bufLen) ||
        !getInt32(env, args[2], &srcPos) || !getInt32(env, args[3], &dataLen) ||
        !getInt64(env, args[4], &timestamp) || !getInt32(env, args[5], &keyframe)) {
        napi_value err;
        napi_create_int32(env, API_ER_NULL, &err);
        return err;
    }
    napi_value result;
    napi_create_int32(env, sem4_addVideoFrame(handle, (const char *) data, srcPos, dataLen,
                                             timestamp, keyframe),
                      &result);
    return result;
}

static napi_value SEMP4_AddAudioFrame(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    napi_value args[5];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    int32_t handle = 0, srcPos = 0, dataLen = 0;
    int64_t timestamp = 0;
    void *data = nullptr;
    size_t bufLen = 0;
    if (!getInt32(env, args[0], &handle) || !getArrayBuffer(env, args[1], &data, &bufLen) ||
        !getInt32(env, args[2], &srcPos) || !getInt32(env, args[3], &dataLen) ||
        !getInt64(env, args[4], &timestamp)) {
        napi_value err;
        napi_create_int32(env, API_ER_NULL, &err);
        return err;
    }
    napi_value result;
    napi_create_int32(env, sem4_addAudioFrame(handle, (const char *) data, srcPos, dataLen, timestamp),
                      &result);
    return result;
}

static napi_value TS2MP4(napi_env env, napi_callback_info info) {
    size_t argc = 6;
    napi_value args[6];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    std::string tsList, outFile;
    int32_t videoType = 0, audioType = 0, sampleRate = 0, channel = 0;
    if (!getString(env, args[0], &tsList) || !getInt32(env, args[1], &videoType) ||
        !getInt32(env, args[2], &audioType) || !getInt32(env, args[3], &sampleRate) ||
        !getInt32(env, args[4], &channel) || !getString(env, args[5], &outFile)) {
        napi_value err;
        napi_create_int32(env, API_ER_NULL, &err);
        return err;
    }
    napi_value result;
    napi_create_int32(env, sem4_ts2mp4(tsList.c_str(), videoType, audioType, sampleRate, channel,
                                       outFile.c_str()),
                      &result);
    return result;
}

static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
            {"SEMP4_GetSdkVer", nullptr, SEMP4_GetSdkVer, nullptr, nullptr, nullptr, napi_default,
             nullptr},
            {"SEMP4_Init", nullptr, SEMP4_Init, nullptr, nullptr, nullptr, napi_default, nullptr},
            {"SEMP4_Open", nullptr, SEMP4_Open, nullptr, nullptr, nullptr, napi_default, nullptr},
            {"SEMP4_Close", nullptr, SEMP4_Close, nullptr, nullptr, nullptr, napi_default, nullptr},
            {"SEMP4_AppendAACHead", nullptr, SEMP4_AppendAACHead, nullptr, nullptr, nullptr,
             napi_default, nullptr},
            {"SEMP4_AddVideoFrame", nullptr, SEMP4_AddVideoFrame, nullptr, nullptr, nullptr,
             napi_default, nullptr},
            {"SEMP4_AddAudioFrame", nullptr, SEMP4_AddAudioFrame, nullptr, nullptr, nullptr,
             napi_default, nullptr},
            {"TS2MP4", nullptr, TS2MP4, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}

static napi_module sem4Module = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = Init,
        .nm_modname = "SE_MP4",
        .nm_priv = ((void *) 0),
        .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterSEMP4Module(void) {
    napi_module_register(&sem4Module);
}
