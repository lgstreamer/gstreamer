LG webOS 5.x GStreamer - GStreamer
==================================

## Description

This directory contains the base GStreamer source, as compiled by LG to be
included in webOS 5.x devices, such as **LG CX OLED TVs**.

Thus, unless LG applied some changes that have not yet been published, the
binaries produced by compiling the source from this repository should work
as a drop-in replacement for the GStreamer binaries that are officially used
by LG in their CX OLED Smart TVs.

This can be very useful if you have [rooted your TV](https://github.com/RootMyTV/RootMyTV.github.io/issues/85#issuecomment-1295058979)
and want to alter this source to enable or restore functionality that is
not provided by default on your CX model.

## Origin

This source, which is licensed under LPGL v2.0, was obtained through a legal
inquiry at https://opensource.lge.com/inquiry and was extracted from the
`webOS 5.0 JO 2.0` archive that can be downloaded [here](http://opensource.lge.com/product/list?page=&ctgr=005&subCtgr=006&keyword=OLED65CX5LB).

The changes that have been applied by LG on top of the official GStreamer
1.14.4 source can be found in [this commit](https://github.com/lgstreamer/gstreamer/commit/ad983a88323edddcb1b4236076ce56c9850632dc).

## Compilation

### Toolchain installation

You will need a recent Linux system, with some GTK related system updates as
well as the webosbrew toolchain from https://www.webosbrew.org. On Debian 11,
the toolchain can be installed as follows:

```
apt install cmake doxygen libglib2.0-dev-bin gobject-introspection libgirepository1.0-dev
wget https://github.com/webosbrew/meta-lg-webos-ndk/releases/download/1.0.g-rev.5/webos-sdk-x86_64-armv7a-neon-toolchain-1.0.g.sh
chmod 755 webos-sdk-x86_64-armv7a-neon-toolchain-1.0.g.sh
./webos-sdk-x86_64-armv7a-neon-toolchain-1.0.g.sh
```

### Build process

Once the toolchain has been installed, you can compile and install the
LG version of GStreamer (which you will need to do to compile additional 
GStreamer plugins) by issuing:

```
git clone https://github.com/lgstreamer/gstreamer.git
cd gstreamer
. /opt/webos-sdk-x86_64/1.0.g/environment-setup-armv7a-neon-webos-linux-gnueabi
./autogen.sh --noconfigure
patch -p1 < gstreamer-1.14.4-make43.patch
./autogen.sh --host=arm-webos-linux-gnueabi --with-sysroot=${SDKTARGETSYSROOT} \
  --prefix=${SDKTARGETSYSROOT}/usr/ --disable-silent-rules \
  --disable-dependency-tracking --disable-gtk-doc \
  --disable-introspection --disable-examples --disable-tests \
  --disable-valgrind --disable-debug --enable-fatal-warnings \
  --enable-system-log --disable-static --without-dw \
  --enable-gst-tracer-hooks --without-unwind --enable-nls
make -j6 install
```