[![Linux](https://github.com/tomasmark79/index2Free/actions/workflows/linux.yml/badge.svg)](https://github.com/tomasmark79/index2Free/actions/workflows/linux.yml)
[![MacOS](https://github.com/tomasmark79/index2Free/actions/workflows/macos.yml/badge.svg)](https://github.com/tomasmark79/index2Free/actions/workflows/macos.yml)
<!-- [![Windows](https://github.com/tomasmark79/index2Free/actions/workflows/windows.yml/badge.svg)](https://github.com/tomasmark79/index2Free/actions/workflows/windows.yml)   -->

## Index2

## Refactored public release of C++ Portfolio Website [digitalspace.name](https://digitalspace.name/new/index.html)  

>refactored 80%

Examples from development 

https://digitalspace.name/new/oop1/index2Free.html
https://digitalspace.name/new/oop2/index2Free.html
https://digitalspace.name/new/oop3/index2Free.html

## Docker dev - exprimental - preparation to bump to upstream template

AllInOneBatch

```bash
conan export ~/.conan2/tomaspack/m4/ --name=m4 --version=1.4.20 --user=local --channel=stable
conan install . --output-folder="./build/standalone/dockerhell/debug" --deployer=full_deploy --build=missing --settings build_type=Debug
source "./build/standalone/dockerhell/debug/conanbuild.sh" && cmake -S "./standalone" -B "./build/standalone/dockerhell/debug" -DCMAKE_TOOLCHAIN_FILE="/workspace/build/standalone/dockerhell/debug/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="/workspace/build/installation/dockerhell/debug"
source "./build/standalone/dockerhell/debug/conanbuild.sh" && cmake --build "./build/standalone/dockerhell/debug" -j $(nproc)
```

## ToDo
- raspberry PI4/5 cross aarch64 portability
- fragment shaders convertor to WebGL1
- fragment shaders convertor to WebGL2
- shader switcher on the fly (more vertex & fragment shaders builtin)
- window position (centered, corners, etc.)
- audio implementation + audio player
- ImGui theme switcher on the fly (more themes builtin)
- build window structure from json declaration
- fx for printed text within windows (typewriter, etc.)
- collect licenses from 3rd party shaders
- ~~Linux x86_64, Windows x64, Wasm Emscripten ready~~
- ~~fragment shaders convertor to Desktop OpenGL~~
- ~~scaling ui on the fly with + - keys~~

## Missing dependencies

How to look for required library in Fedora.

```bash
dnf provides */libGLEW.so.2.2
```

## License

MIT License  
Copyright (c) 2024-2025 Tomáš Mark

## Disclaimer

This template is provided "as is," without any guarantees regarding its functionality.