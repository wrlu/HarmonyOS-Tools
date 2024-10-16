# DumpHarmony
## 功能
1. 实现以App权限复制HarmonyOS NEXT系统文件夹到电脑，主要用于获取系统`/system/lib64`或者`/system/app`这类目录文件，方便安全研究人员进行逆向分析。
2. 实现hdc shell复制HarmonyOS NEXT第三方应用到电脑。

## 环境要求
* DevEco Studio 5.0.3.300SP2或更高版本
* HarmonyOS NEXT Developer Beta1或更高版本
* python 3.x并安装好flask

## 获取系统文件夹
**注意：您需要首先成为鸿蒙开发者，否则无法获取调试profile为应用签名。**
1. 使用DevEco Studio打开项目，然后配置正确的调试profile；
2. HarmonyOS NEXT手机进入“设置-设备名称”，点按软件版本7次，打开开发者模式，手机会自动重启，手机重启后进入“设置-系统-开发者模式”，打开USB调试；
3. HarmonyOS NEXT手机和PC运行在同一局域网，修改app.run的参数为你想要的，并且运行web/file_receiver.py，并在Index.ets中修改uploadUrl为相同正确的URL；
4. 使用DevEco Studio运行应用，过滤日志输出“DumpHarmony”查看上传进度，文件保存在uploads下。

## 获取第三方应用文件
1. 电脑为鸿蒙Command line tools配置好环境变量，可执行hdc命令；
2. 直接执行`tools/dump_harmony.py`。

## 为什么使用两种获取方式
1. 获取系统文件夹：华为对hdc shell的权限限制极为严格，并不能访问系统大部分文件路径的内容，所以无法像Android手机一样通过adb shell来获取文件。如果在Android上有类似的需求可以使用：https://github.com/wrlu/Android-Tools
2. 获取第三方应用文件：可通过bm命令获取到hap路径，不过无法直接使用hdc file recv拉取，从某个版本之后可以先复制到`/data/local/tmp`然后再recv.但是在App的权限下限制了获取第三方应用文件，这个操作需要一个system_basic级别的系统权限`ohos.permission.ACCESS_BUNDLE_DIR`，参见：[仅对系统应用开放的权限](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/security/AccessToken/permissions-for-system-apps.md#ohos.permission.access_bundle_dir)

## License
MIT License
