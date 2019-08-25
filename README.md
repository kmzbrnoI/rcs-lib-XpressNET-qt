# XpressNET library for RCS of hJOPserver

This library allows [hJOPserver](https://github.com/kmzbrnoI/hJOPserver) to
operate with XpressNET accessory modules. It replaces
[MTB library](https://github.com/kmzbrnoI/mtb-lib). Library is distributed as
a single dll file. It uses [XpressNET library](https://github.com/kmzbrnoI/xn-lib-cpp-qt)
for XpressNET communication. The library is written in C++ in framework
[Qt](https://www.qt.io/)5. Built dlls are available in *Releases* section.

This library implements
[RCS API of hJOPserver](https://github.com/kmzbrnoI/mtb-lib/wiki/api-specs).

## Building & toolkit

This library was developed in `vim` using `qmake` & `make`. It is suggested
to use `clang` as a compiler, because then you may use `clang-tools` (see below).

### Prerequisities

 * Qt 5
 * Qt's `serialport`
 * [Bear](https://github.com/rizsotto/Bear)

### Toolchain setup on debian

```bash
apt install qt5-default libqt5serialport5-dev
apt install bear
apt install clang-7 clang-tools-7 clang-tidy-7 clang-format-7
```

### Build

Clone this repository (including submodules!):

```
$ git clone --recurse-submodules https://github.com/kmzbrnoI/rcs-lib-XpressNET-qt
```

And then build:

```
$ mkdir build
$ cd build
$ qmake -spec linux-clang ..
$ bear make
```

## Cross-compiling for Windows

This library could be cross-compiled for Windows via [MXE](https://mxe.cc/).
Follow [these instructions](https://stackoverflow.com/questions/14170590/building-qt-5-on-linux-for-windows)
for building standalone `dll` file.

You may want to use similar script as `activate.sh`:

```bash
export PATH="$HOME/...../mxe/usr/bin:$PATH"
~/...../mxe/usr/i686-w64-mingw32.static/qt5/bin/qmake ..
```

Make MXE this way:

```bash
make qtbase qtserialport
```

## Style checking

```
$ clang-tidy-7 -p ../build -extra-arg-before=-x -extra-arg-before=c++ -extra-arg=-std=c++14 *.cpp *.h
$ clang-format-7 *.cpp *.h
```

## Authors

This library was created by:

 * Jan Horacek ([jan.horacek@kmz-brno.cz](mailto:jan.horacek@kmz-brno.cz))

Do not hesitate to contact author in case of any troubles!

## License

This application is released under the [Apache License v2.0
](https://www.apache.org/licenses/LICENSE-2.0).
