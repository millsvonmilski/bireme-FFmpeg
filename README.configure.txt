./configure --toolchain=msvc --arch=x86_64 --enable-yasm --enable-asm 
            --enable-debug --enable-sdl --disable-dxva2 --disable-doc --disable-htmlpages
            --disable-manpages --disable-podpages --disable-txtpages --disable-network 
			--enable-ffprobe --enable-ffplay --enable-protocol=file --enable-filter=scale 
			--enable-demuxer='m4v,mov,mp3' --enable-filter=aresample --enable-decoder='h264_mf_video,aac_mf_audio,mp3_mf_audio' 
			--prefix=<disk temp location> --sdl_dir=<location of SDL 1.x (eg includes and libs) >

./configure --toolchain=msvc --arch=x86_64 --enable-yasm --enable-asm --enable-debug --enable-sdl --disable-dxva2 --disable-doc --disable-htmlpages --disable-manpages --disable-podpages --disable-txtpages --disable-network --enable-ffprobe --enable-ffplay --enable-protocol=file --enable-filter=scale --enable-demuxer='m4v,mov,mp3' --enable-filter=aresample --enable-decoder='h264_mf_video,aac_mf_audio,mp3_mf_audio' --prefix=/c/Users/username/Temp/ffmpeg --sdl_dir=/d/bt/SDL_TOP_DIR
