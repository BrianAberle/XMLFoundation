
## Build only OpenSSL
This project has separate independent script to build only OpenSSL library.
```
Usage:
./openssl-build <ANDROID_NDK_PATH> <OPENSSL_SOURCES_PATH> <ANDROID_TARGET_API> \
                <ANDROID_TARGET_ABI> <GCC_VERSION> <OUTPUT_PATH>

Supported target ABIs: armeabi, armeabi-v7a, x86, x86_64, arm64-v8a

Example using GCC 4.8, NDK 10e, OpenSSL 1.0.2d and Android API 21 for armeabi-v7a.
./openssl-build /home/user/android-ndk-r10e /home/user/openssl-1.0.2d 21 \
                armeabi-v7a 4.8 /home/user/output/armeabi-v7a
```
If you want to leverage on the <b>config.conf</b> properties and build OpenSSL for every configured target arch, you can use the following helper script:
```
./openssl-build-target-archs
```
When everything goes well, you will see an output like this:
```
Building OpenSSL for target arch armeabi ...
Building OpenSSL for target arch armeabi-v7a ...
Building OpenSSL for target arch x86 ...
Building OpenSSL for target arch x86_64 ...
Building OpenSSL for target arch arm64-v8a ...
Finished building OpenSSL! Check output folder: /home/user/pjsip-android-builder/openssl-build-output
```
The script is going to create a new folder named <b>openssl-build-output</b> organized as follows:
```
openssl-build-output
 |-- logs/  contains the full build log for each target architecture
 |-- libs/  contains the compiled libraries for each target architecture
```
If something goes wrong during the compilation of a particular target architecture, the main script will be terminated and you can see the full log in `./openssl-build-output/logs/<arch>.log`. So for example if there's an error for <b>x86</b>, you can see the full log in `./openssl-build-output/logs/x86.log`

### Change OpenSSL release version
1. Go to [OpenSSL download page](https://www.openssl.org/source/) and get the link to download the latest stable version (at the time of this writing the version is 1.0.2h and the link is: https://www.openssl.org/source/openssl-1.0.2h.tar.gz)
2. Edit `config.conf` and change: 
  ```
OPENSSL_DOWNLOAD_URL="https://www.openssl.org/source/openssl-1.0.2h.tar.gz"
OPENSSL_DIR_NAME="openssl-1.0.2h"

DOWNLOAD_NDK=0
DOWNLOAD_SDK=0
DOWNLOAD_ANDROID_APIS=0
DOWNLOAD_PJSIP=0
DOWNLOAD_SWIG=0
DOWNLOAD_OPENSSL=1
DOWNLOAD_OPENH264=0
DOWNLOAD_LIBYUV=0
  ```
3. remove `openssl-build-output` and the old `openssl-1.0.2g` (or whatever version instead of 1.0.2g) directories
4. Execute `./prepare-build-system` and the new OpenSSL will be downloaded and compiled (go get a coffee :D)
5. Rebuild PJSIP (by either using `./build` or `./build-with-g729`)

## License

    Copyright (C) 2015-2016 Aleksandar Gotev

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
