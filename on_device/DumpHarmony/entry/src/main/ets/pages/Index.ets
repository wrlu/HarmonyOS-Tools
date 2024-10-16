import napi from 'libentry.so';
import fs from '@ohos.file.fs';
import zlib from '@ohos.zlib';
import request from '@ohos.request';

import { common } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { taskpool } from '@kit.ArkTS';
import { hilog } from '@kit.PerformanceAnalysisKit';

let TAG = "DumpHarmonyArk"
let context = getContext(this) as common.UIAbilityContext;
let uploadUrl = "http://192.168.0.193:5000/upload_from_ohos"

@Entry
@Component
struct Index {
  @State message: string = 'Start';

  build() {
    Row() {
      Column() {
        Text(this.message)
          .fontSize(50)
          .fontWeight(FontWeight.Bold)
          .onClick(() => {
            startDumpHarmony();
          })
      }
      .width('100%')
    }
    .height('100%')
  }
}

@Concurrent
function copyDirNative(from: string, to: string) {
  return napi.copyDir(from, to);
}

function copyAndSendSystemPath(from: string, to: string, output: string) {
  let dst = to + "/" + output;
  if (fs.accessSync(dst)) {
    fs.rmdirSync(dst);
  }
  fs.mkdirSync(dst);

  taskpool.execute(copyDirNative, from, dst).then((value: Object) => {
    hilog.debug(0, TAG, "copyDirNative from %{public}s to %{public}s finished.", from, dst);
    let zipFileName = output + ".zip";
    let zipFilePath = context.cacheDir + "/" + zipFileName;
    let compressOptions: zlib.Options = {
      level: zlib.CompressLevel.COMPRESS_LEVEL_DEFAULT_COMPRESSION,
      memLevel: zlib.MemLevel.MEM_LEVEL_DEFAULT,
      strategy: zlib.CompressStrategy.COMPRESS_STRATEGY_DEFAULT_STRATEGY
    };

    zlib.compressFile(dst, zipFilePath, compressOptions).then(() => {
      hilog.debug(0, TAG, "CompressFile %{public}s finished.", zipFilePath);
      sendZip(uploadUrl, zipFileName);
    }).catch((err: BusinessError) => {
      hilog.error(0, TAG, "CompressFile %{public}s error: %{public}s", zipFilePath, JSON.stringify(err) ?? '');
    });
  });
}

function sendZip(url: string, filename: string) {
  hilog.debug(0, TAG, "Start sendZip %{public}s.", filename);

  let header = new Map<Object, string>();
  let files: Array<request.File> = [
    { filename: filename, name: 'file', uri: 'internal://cache/' + filename, type: 'zip' }
  ]
  let data: Array<request.RequestData> = [];
  let uploadConfig: request.UploadConfig = {
    url: url,
    header: header,
    method: 'POST',
    files: files,
    data: data
  }

  try {
    request.uploadFile(context, uploadConfig)
      .then((uploadTask: request.UploadTask) => {
        uploadTask.on('complete', (taskStates: Array<request.TaskState>) => {
          for (let i = 0; i < taskStates.length; i++) {
            hilog.debug(0, TAG, `Upload complete taskState: ${JSON.stringify(taskStates[i])}`);
          }
        });
        uploadTask.on('progress', (uploadedSize: number, totalSize: number) => {
          let percent = (uploadedSize / totalSize) * 100;
          hilog.debug(0, TAG, `Upload file ${filename} progress: ${uploadedSize} / ${totalSize}, ${percent}%`);
        });
      }).catch((err: BusinessError) => {
        hilog.error(0, TAG, `Invoke uploadFile failed, code is ${err.code}, message is ${err.message}`);
      })
  } catch (error) {
    let err: BusinessError = error as BusinessError;
    hilog.error(0, TAG, `Invoke uploadFile failed, code is ${err.code}, message is ${err.message}`);
  }
}

function startDumpHarmony() {
  let dumpPath = context.cacheDir + "/system_dump";
  // clear dump folder first.
  if (fs.accessSync(dumpPath)) {
    fs.rmdirSync(dumpPath);
  }
  fs.mkdirSync(dumpPath);

  copyAndSendSystemPath("/system/app", dumpPath, "system_app");
  copyAndSendSystemPath("/system/lib64", dumpPath, "system_lib64");
  copyAndSendSystemPath("/system/lib", dumpPath, "system_lib");
}
