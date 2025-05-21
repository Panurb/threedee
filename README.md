# README #

### How do I get set up? ###

MSVC:
* cmake . -B build -G "Visual Studio 17 2022" -A x64
* cd build
* cmake --build . --target install --config Release

Emscripten:
* emcmake cmake . -B build
* cd build
* cmake --build .
* emrun threedee.html
