.PHONY: config build debug clean run

config:
	cmake . -B build -G "Visual Studio 17 2022" -A x64

build:
	cmake --build ./build --target install --config Release --parallel 8

debug:
	cmake --build ./build --target install --config Debug --parallel 8

clean:
	cmake --build ./build --target clean

run:
	cd build && threedee.exe
