#include "napi/native_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <hilog/log.h>

#define TAG "DumpHarmonyNative"
#define MAX_PATH_LEN 256
#define FILE_BUF_SIZE 4096

static napi_status copy_from_ts(napi_env env, napi_callback_info info, size_t require_argc, napi_value *argv) {
    if (env == nullptr || info == nullptr || argv == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0, TAG, "env, callback_info or argv is null");
        return napi_invalid_arg;
    }
    size_t real_argc = require_argc;
    napi_status status;
    if ((status = napi_get_cb_info(env, info, &real_argc, argv, nullptr, nullptr)) != napi_ok) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0, TAG, "napi_get_cb_info failed, returns %{public}d", status);
        return status;
    }
    if (real_argc != require_argc) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0, TAG, "Missing parameters, required %{public}zu but received %{public}zu.",
                     require_argc, real_argc);
        return napi_invalid_arg;
    }
    return napi_ok;
}

static char *get_string_utf_chars(napi_env env, napi_value arg, size_t max_len) {
    if (env == nullptr || arg == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0, TAG, "env or arg is null");
        return nullptr;
    }

    char *buf = (char *)malloc(max_len * sizeof(char));
    if (buf == nullptr) {
        return nullptr;
    }

    size_t len = 0;
    napi_status status;
    if ((status = napi_get_value_string_utf8(env, arg, buf, max_len, &len)) != napi_ok) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0, TAG, "napi_get_value_string_utf8 failed, returns %{public}d", status);
        return nullptr;
    }

    char *utf = (char *)realloc(buf, (len + 1) * sizeof(char));
    if (utf == nullptr) {
        free(buf);
        return nullptr;
    }

    utf[len] = '\0';
    return utf;
}

static void release_string_utf_chars(char *utf) {
    if (utf != nullptr) {
        free(utf);
        utf = nullptr;
    }
}

napi_status copy_dir(const char *src, const char *dst) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(src)) == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0, TAG, "Error opening source directory: %{public}s", src);
        return napi_invalid_arg;
    }
    
    napi_status status = napi_ok;

    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char currentSrc[strlen(src) + strlen(entry->d_name) + 2];
        char currentDst[strlen(dst) + strlen(entry->d_name) + 2];

        sprintf(currentSrc, "%s/%s", src, entry->d_name);
        sprintf(currentDst, "%s/%s", dst, entry->d_name);

        if (entry->d_type == DT_DIR) {
            mkdir(currentDst, 0700);
            napi_status tmp_status = copy_dir(currentSrc, currentDst);
            if (tmp_status != napi_ok) {
                status = tmp_status;
                OH_LOG_Print(LOG_APP, LOG_WARN, 0, TAG, "Error copying dir: %{public}s.", currentSrc);
            }
        } else if (entry->d_type == DT_REG){
            FILE *sourceFile = fopen(currentSrc, "rb");
            FILE *destinationFile = fopen(currentDst, "wb");

            if (sourceFile == nullptr) {
                OH_LOG_Print(LOG_APP, LOG_WARN, 0, TAG, "Cannot open source file: %{public}s.", currentSrc);
            } else if (destinationFile == nullptr) {
                OH_LOG_Print(LOG_APP, LOG_WARN, 0, TAG, "Cannot open destination file: %{public}s.", currentDst);
            } else {
                char buf[FILE_BUF_SIZE];
                while (!feof(sourceFile)) {
                    size_t bytesRead = fread(buf, 1, FILE_BUF_SIZE, sourceFile);
                    fwrite(buf, 1, bytesRead, destinationFile);
                }
                fclose(sourceFile);
                fclose(destinationFile);
            }
        }
    }
    closedir(dir);
    
    return status;
}

static napi_value copy_dir(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    napi_value argv[2] = {nullptr};

    napi_status status;
    if ((status = copy_from_ts(env, info, requireArgc, argv)) != napi_ok) {
        char status_str[3];
        snprintf(status_str, 3, "%d", status);
        napi_throw_error(env, status_str, "copy_from_ts failed.");
        return nullptr;
    }
    
    char *src = nullptr;
    if ((src = get_string_utf_chars(env, argv[0], MAX_PATH_LEN)) == nullptr) {
        napi_throw_error(env, "1", "get_string_utf_chars src failed.");
        return nullptr;
    }

    char *dst = nullptr;
    if ((dst = get_string_utf_chars(env, argv[1], MAX_PATH_LEN)) == nullptr) {
        napi_throw_error(env, "1", "get_string_utf_chars dst failed.");
        return nullptr;
    }
    
    status = copy_dir(src, dst);
    
    release_string_utf_chars(src);
    release_string_utf_chars(dst);

    napi_value result;
    napi_create_int64(env, static_cast<int64_t>(status), &result);
    return result;
}

EXTERN_C_START
static napi_value init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        { "copyDir", nullptr, copy_dir, nullptr, nullptr, nullptr, napi_default, nullptr }
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&module);
}
