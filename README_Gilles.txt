Media Foundation Audio & Video Decoders for FFMPEG
==================================================

Prerequisites
-------------

Visual Studio 2015 (Community Edition works fine)
msys2 : http://msys2.github.io/
SDL 1.2.15 (needed for ffplay) : http://libsdl.org/  download SDL-1.2.15-win32-x64.zip
yasm (needed to compile ffmpeg) : http://yasm.tortall.net/Download.html

Edit your environment variables to add yasm to your executable PATH

Modifications to ffmpeg/libavcodec
-----------------------------------

configure : modified to allow compilation of c++ file with Visual Studio and add the MF audio/video decoder modules
Edit this file so that the sdl_ variables point to where you downloaded SDL
example:
sdl_cflags="-Ic:/users/gilles/Develop/SDL-1.2.15/include"
sdl_libs="-lSDL -Lc:/users/gilles/Develop/SDL-1.2.15/lib/x86"

libavcodec/Makefile and libavcodec/allcodecs.c : modified to add the MF audio/video decoder modules

mf_audio_decoder.cpp and mf_video_decoder.cpp : MF audio/video decoder modules implementation

Building
--------

1. Setup your build environment: open a Visual Studio command shell ('Developer Command Prompt for VS2015') so that the Visual Studio command line tools are in your path.
2. From that shell, start an MSYS shell : run msys2_shell.bat (from where you installed msys, typically c:\msys64\msys2_shell.bat)
3. configure ffmpeg/libavcodec:
./configure --toolchain=msvc --optflags='-Zi -Od -Og - ' --arch=x86 --enable-sdl --enable-yasm --enable-asm --enable-shared --disable-static --enable-debug --disable-optimizations --disable-everything --disable-dxva2 --disable-doc --disable-htmlpages --disable-manpages --disable-podpages --disable-txtpages --disable-network --enable-ffprobe --enable-ffplay --enable-protocol=file --enable-filter=scale --enable-demuxer=m4v,mov,mp3 --enable-filter=aresample --enable-decoder=h264_mf_video,aac_mf_audio,mp3_mf_audio --prefix=/c/Users/gilles/Temp/ffmpeg
(change the install prefix to the location where you want to put the built binaries)
4. build and install
'make V=1 install'
This will build and install the libs and binaries to the location you have chosen in the configure step.

NOTE: if you edit the source files, you only need to rebuild with 'make install' or 'make V=1 install', you don't need to re-run the configure step.

Running
-------

1. Copy SDL.dll from the SDL SDK download to the 'bin' directory of your ffmpeg build output directory. You only need to do this once, because 'make install' from ffmpeg won't delete it.
2. run ffplay.exe from the ffmpeg build output directory.

Working Visual Studio
---------------------

 Project Files
 -------------
 FfmpegMfDecoders.sln --> solution file. Open this with Visual Studio
 FfmpegMfDecoders/FfmpegMfDecoders/FfmpegMfDecoders.vcxproj --> project file

 Debugging
----------
Edit the debug configuration to set the 'Command' setting to your built copy of ffplay (ex: C:\Users\gilles\Temp\ffmpeg\bin\ffplay.exe) and 'Command Arguments' to the file you want to play.
