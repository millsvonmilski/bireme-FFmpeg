Building FFMPEG with the videotoolbox_decoder and audiotoolbox_decoder modules
==============================================================================

[Change the --prefix option with a path that makes sense for your dev environment]

# mini config with debug
./configure --enable-debug --disable-optimizations --disable-everything --disable-vda --disable-videotoolbox --disable-doc --disable-htmlpages --disable-manpages --disable-podpages --disable-txtpages --disable-network --enable-ffplay --enable-ffprobe --enable-protocol=file --enable-filter=scale --enable-demuxer=m4v,mov,mp3 --enable-filter=aresample --enable-decoder=aac_audiotoolbox,mp3_audiotoolbox,h264_videotoolbox --prefix=/Users/gilles/Temp/ffmpeg

# mini config without debug
./configure --enable-shared --disable-everything --disable-vda --disable-videotoolbox --disable-doc --disable-htmlpages --disable-manpages --disable-podpages --disable-txtpages --disable-network --enable-ffplay --enable-ffprobe --enable-protocol=file --enable-filter=scale --enable-parser=mpegaudio,h264 --enable-demuxer=m4v,mov,mp3 --enable-filter=aresample --enable-decoder=aac_audiotoolbox,mp3_audiotoolbox,h264_videotoolbox --prefix=/Users/gilles/Temp/ffmpeg

# full config with debug
./configure --enable-debug --disable-optimizations --disable-vda --disable-videotoolbox --disable-decoder=aac,mp3,h264 --enable-decoder=aac_audiotoolbox,mp3_audiotoolbox,h264_videotoolbox --prefix=/Users/gilles/Temp/ffmpeg

# full config without debug
./configure --disable-vda --disable-videotoolbox --disable-decoder=aac,mp3,h264 --enable-decoder=aac_audiotoolbox,mp3_audiotoolbox,h264_videotoolbox --prefix=/Users/gilles/Temp/ffmpeg

Once configured, you can run make.
You can then run from the locally built binary (ffplay_g in debug mode, of ffplay in release mode), you don't need to run 'make install', since those are statically linked.

Building and Debugging with xcode
=================================
Project file: FfmpegAudioVideoToolboxDecoders.xcodeproj

* Build from the project either library (or both)
* Link from the command line to get a version of ffplay_g that's linked against the newly built module:
(replace path of the library build location for your system. On my setup, the build is under build/Debug for debug builds)

For Video:
gcc -Lbuild/Debug -lFfmpegVideotoolboxDecoder -Llibavcodec -Llibavdevice -Llibavfilter -Llibavformat -Llibavresample -Llibavutil -Llibpostproc -Llibswscale -Llibswresample -Wl,-dynamic,-search_paths_first -Qunused-arguments   -o ffplay_g cmdutils.o ffplay.o   -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil -framework QTKit -framework Foundation -framework QuartzCore -framework CoreVideo -framework Foundation -framework AVFoundation -framework CoreMedia -liconv -Wl,-framework,CoreFoundation -Wl,-framework,Security -lSDLmain -lSDL -Wl,-framework,Cocoa -framework CoreGraphics -lm -lbz2 -lz -pthread -framework CoreServices -L/usr/local/lib -lSDLmain -lSDL -Wl,-framework,Cocoa -framework AudioToolbox -framework VideoToolbox

For Audio:
gcc -Lbuild/Debug -lFfmpegAudiotoolboxDecoder -Llibavcodec -Llibavdevice -Llibavfilter -Llibavformat -Llibavresample -Llibavutil -Llibpostproc -Llibswscale -Llibswresample -Wl,-dynamic,-search_paths_first -Qunused-arguments   -o ffplay_g cmdutils.o ffplay.o   -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil -framework QTKit -framework Foundation -framework QuartzCore -framework CoreVideo -framework Foundation -framework AVFoundation -framework CoreMedia -liconv -Wl,-framework,CoreFoundation -Wl,-framework,Security -lSDLmain -lSDL -Wl,-framework,Cocoa -framework CoreGraphics -lm -lbz2 -lz -pthread -framework CoreServices -L/usr/local/lib -lSDLmain -lSDL -Wl,-framework,Cocoa -framework AudioToolbox -framework VideoToolbox

For both:
gcc -Lbuild/Debug -lFfmpegAudiotoolboxDecoder -lFfmpegVideotoolboxDecoder -Llibavcodec -Llibavdevice -Llibavfilter -Llibavformat -Llibavresample -Llibavutil -Llibpostproc -Llibswscale -Llibswresample -Wl,-dynamic,-search_paths_first -Qunused-arguments   -o ffplay_g cmdutils.o ffplay.o   -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil -framework QTKit -framework Foundation -framework QuartzCore -framework CoreVideo -framework Foundation -framework AVFoundation -framework CoreMedia -liconv -Wl,-framework,CoreFoundation -Wl,-framework,Security -lSDLmain -lSDL -Wl,-framework,Cocoa -framework CoreGraphics -lm -lbz2 -lz -pthread -framework CoreServices -L/usr/local/lib -lSDLmain -lSDL -Wl,-framework,Cocoa -framework AudioToolbox -framework VideoToolbox

Now you can run and debug from XCode.
(don't forget to re-link manually after you rebuild the module)

Testing
=======

Test with an MP4 file that contains AAC Audio, H.264 Video or both. You can also test with an MP3 Audio file

Checking that you're loading the right module
=============================================

For Video, you will see an inverted square on the top-left corner of the image.
You can also run ffplay with -loglevel 56 to get a trace of the calls (you can use lower log values of course if you don't want to trace all the calls)
