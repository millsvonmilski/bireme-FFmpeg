/*----------------------------------------------------------------
| includes
+----------------------------------------------------------------*/
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"

#if !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#else
#include "AudioToolbox.h"
#include "CoreFoundation.h"
#endif

/*----------------------------------------------------------------
| constants
+----------------------------------------------------------------*/
#define AVT_AUDIO_CONVERTER_DECODER_AAC_PACKETS_PER_CONVERSION 1024
#define AVT_AUDIO_CONVERTER_DECODER_MP3_PACKETS_PER_CONVERSION 1152
#define AVT_AUDIO_CONVERTER_DECODER_MAX_OUTPUT_BUFFER_SIZE     (2048*2*8) /* max 2048 samples, 8 channels @ 16 bits */
#define AVT_AUDIO_CONVERTER_DATA_UNDERFLOW_ERROR               1234       /* arbitrary error code */

/*----------------------------------------------------------------
| types
+----------------------------------------------------------------*/
typedef struct AT_DecoderContext {
    const AVClass *class;

    AudioConverterRef            converter;
    AVPacket*                    input_packet;
    AudioStreamPacketDescription input_packet_description;
    unsigned char                output_buffer[AVT_AUDIO_CONVERTER_DECODER_MAX_OUTPUT_BUFFER_SIZE];
} AT_DecoderContext;

/*----------------------------------------------------------------*/
static OSStatus
at_decoder_data_proc(AudioConverterRef              inAudioConverter,
UInt32*                        ioNumberDataPackets,
AudioBufferList*               ioData,
AudioStreamPacketDescription** outDataPacketDescription,
void*                          inUserData)
{
    AT_DecoderContext* self = (AT_DecoderContext*)inUserData;

    (void)inAudioConverter;

    // if we don't have data available, return nothing
    if (self->input_packet == NULL || self->input_packet->size == 0) {
        *ioNumberDataPackets = 0;
        return AVT_AUDIO_CONVERTER_DATA_UNDERFLOW_ERROR;
    }

    // return one packet's worth of data
    ioData->mNumberBuffers = 1;
    ioData->mBuffers[0].mNumberChannels = 0;
    ioData->mBuffers[0].mDataByteSize = self->input_packet->size;
    ioData->mBuffers[0].mData = self->input_packet->data;
    *ioNumberDataPackets = 1;
    if (outDataPacketDescription) {
        self->input_packet_description.mStartOffset = 0;
        self->input_packet_description.mDataByteSize = ioData->mBuffers[0].mDataByteSize;
        self->input_packet_description.mVariableFramesInPacket = 0;
        *outDataPacketDescription = &self->input_packet_description;
    }

    return noErr;
}

/*----------------------------------------------------------------*/
static int
at_decoder_close(AVCodecContext* avctx)
{
    AT_DecoderContext* self = avctx->priv_data;

    av_log(avctx, AV_LOG_TRACE, "at_decoder_close\n");

    if (self->converter) {
        AudioConverterDispose(self->converter);
    }

    return 0;
}

/*----------------------------------------------------------------*/
static int
at_decoder_init(AVCodecContext* avctx)
{
    AT_DecoderContext*          self = avctx->priv_data;
    AudioStreamBasicDescription source_format;
    AudioStreamBasicDescription dest_format;
    OSStatus                    status = noErr;

    av_log(avctx, AV_LOG_TRACE, "at_decoder_init\n");

    // setup the source format defaults
    memset(&source_format, 0, sizeof(source_format));
    source_format.mSampleRate = (Float64)avctx->sample_rate;
    source_format.mFormatFlags = 0;
    source_format.mBytesPerPacket = 0;
    source_format.mFramesPerPacket = 0;
    source_format.mBytesPerFrame = 0;
    source_format.mChannelsPerFrame = avctx->channels;
    source_format.mBitsPerChannel = 0;

    // be more specific about the format based on the codec we're using
    if (avctx->codec_id == AV_CODEC_ID_MP3) {
        source_format.mFormatID = kAudioFormatMPEGLayer3;
    }
    else if (avctx->codec_id == AV_CODEC_ID_AAC) {
        source_format.mFormatID = kAudioFormatMPEG4AAC;

        if (avctx->extradata_size && avctx->extradata) {
            //AP4_Mp4AudioDecoderConfig dec_config;
            //AP4_Result result = dec_config.Parse(mp4_type->decoder_info, mp4_type->decoder_info_length);

            // use the info from the decoder info to update the source format
            // (what's in the MP4 is not always accurate)
            //            source_format.mChannelsPerFrame = dec_config.m_ChannelCount;
            //            source_format.mSampleRate       = dec_config.m_SamplingFrequency;

            // check if this looks like HeAAC
            if (avctx->extradata_size > 2) {
                // if the decoder info is more than 2 bytes, assume this is He-AAC
                //                if (AP4_SUCCEEDED(result)) {
                //                    ATX_LOG_FINE("He-AAC detected");
                //                    if (dec_config.m_Extension.m_SbrPresent) {
                //                        source_format.mFormatID = kAudioFormatMPEG4AAC_HE;
                //                        source_format.mSampleRate = dec_config.m_Extension.m_SamplingFrequency;
                //                    }
                //                    if (dec_config.m_Extension.m_PsPresent) {
                //                        source_format.mFormatID = kAudioFormatMPEG4AAC_HE_V2;
                //                        source_format.mSampleRate = dec_config.m_Extension.m_SamplingFrequency;
                //                        source_format.mChannelsPerFrame = 2*dec_config.m_ChannelCount;
                //                    }
                //                } else {
                //                    ATX_LOG_WARNING_1("unable to parse decoder specific info (%d)", result);
                //                }
            }
        }
    }
    else {
        return AVERROR_BUG;
    }

    // setup the dest format
    memset(&dest_format, 0, sizeof(dest_format));
    dest_format.mFormatID = kAudioFormatLinearPCM;
    dest_format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    dest_format.mSampleRate = 0;
    dest_format.mBytesPerPacket = 0;
    dest_format.mFramesPerPacket = 1;
    dest_format.mBytesPerFrame = 0;
    dest_format.mChannelsPerFrame = 2;
    dest_format.mBitsPerChannel = 16;

    status = AudioConverterNew(&source_format,
        &dest_format,
        &self->converter);
    if (status != noErr) {
        av_log(avctx, AV_LOG_WARNING, "AudioConverterNew failed (%d)\n", (int)status);
        return AVERROR_EXTERNAL;
    }

    // setup codec-specific parameters
    if (avctx->codec_id == AV_CODEC_ID_AAC) {
        unsigned int   magic_cookie_size = avctx->extradata_size + 25;
        unsigned char* magic_cookie = av_malloc(magic_cookie_size);

        /* construct the content of the magic cookie (the 'ES Descriptor') */
        magic_cookie[0] = 0x03;                 /* ES_Descriptor tag */
        magic_cookie[1] = magic_cookie_size - 2;  /* ES_Descriptor payload size */
        magic_cookie[2] = 0;                    /* ES ID */
        magic_cookie[3] = 0;                    /* ES ID */
        magic_cookie[4] = 0;                    /* flags */
        magic_cookie[5] = 0x04;                 /* DecoderConfig tag */
        magic_cookie[6] = magic_cookie_size - 10; /* DecoderConfig payload size */
        magic_cookie[7] = 0x40;                 /* object type */
        magic_cookie[8] = 0x05 << 2 | 1;          /* stream type | reserved */
        magic_cookie[9] = 0;                    /* buffer size */
        magic_cookie[10] = 0x18;                 /* buffer size */
        magic_cookie[11] = 0;                    /* buffer size */
        magic_cookie[12] = 0;                    /* max bitrate */
        magic_cookie[13] = 0x08;                 /* max bitrate */
        magic_cookie[14] = 0;                    /* max bitrate */
        magic_cookie[15] = 0;                    /* max bitrate */
        magic_cookie[16] = 0;                    /* avg bitrate */
        magic_cookie[17] = 0x04;                 /* avg bitrate */
        magic_cookie[18] = 0;                    /* avg bitrate */
        magic_cookie[19] = 0;                    /* avg bitrate */
        magic_cookie[20] = 0x05;                 /* DecoderSpecificInfo tag */
        magic_cookie[21] = avctx->extradata_size; /* DecoderSpecificInfo payload size */
        if (avctx->extradata_size) {
            memcpy(&magic_cookie[22], avctx->extradata, avctx->extradata_size);
        }
        magic_cookie[22 + avctx->extradata_size] = 0x06; /* SLConfigDescriptor tag    */
        magic_cookie[22 + avctx->extradata_size + 1] = 0x01; /* SLConfigDescriptor length */
        magic_cookie[22 + avctx->extradata_size + 2] = 0x02; /* fixed                     */

        status = AudioConverterSetProperty(self->converter,
            kAudioConverterDecompressionMagicCookie,
            magic_cookie_size,
            magic_cookie);
        av_free(magic_cookie);
        if (status != noErr) {
            av_log(avctx, AV_LOG_WARNING, "AudioConverterSetProperty failed (%d)\n", (int)status);
            return AVERROR_EXTERNAL;
        }
    }

    return 0;
}

/*----------------------------------------------------------------*/
static int
at_decoder_decode(AVCodecContext* avctx, void* data, int* got_frame, AVPacket* avpkt)
{
    AT_DecoderContext*          self = avctx->priv_data;
    AVFrame*                    frame = data;
    AudioBufferList             output_buffers;
    UInt32                      output_packet_count = (avctx->codec_id == AV_CODEC_ID_AAC) ?
    AVT_AUDIO_CONVERTER_DECODER_AAC_PACKETS_PER_CONVERSION :
                                                           AVT_AUDIO_CONVERTER_DECODER_MP3_PACKETS_PER_CONVERSION;
    AudioStreamBasicDescription output_format;
    UInt32                      output_format_size = sizeof(output_format);
    unsigned int                output_sample_count = 0;
    OSStatus                    status = noErr;
    int                         ff_result = 0;

    av_log(avctx, AV_LOG_TRACE, "at_decoder_decode - size=%d\n", avpkt->size);

    // default return value
    *got_frame = 0;

    // init the frame
    av_frame_unref(frame);

    // remember the current input packet for the data callback
    self->input_packet = avpkt;

    // decode some of the input data
    output_buffers.mNumberBuffers = 1;
    output_buffers.mBuffers[0].mNumberChannels = 0;
    output_buffers.mBuffers[0].mDataByteSize = AVT_AUDIO_CONVERTER_DECODER_MAX_OUTPUT_BUFFER_SIZE;
    output_buffers.mBuffers[0].mData = self->output_buffer;
    status = AudioConverterFillComplexBuffer(self->converter,
        at_decoder_data_proc,
        (void*)self,
        &output_packet_count,
        &output_buffers,
        NULL);
    av_log(avctx, AV_LOG_TRACE, "AudioConverterFillComplexBuffer() returned %d - output_packet_count=%u\n", (int)status, output_packet_count);

    if (status != noErr && status != AVT_AUDIO_CONVERTER_DATA_UNDERFLOW_ERROR) {
        av_log(avctx, AV_LOG_WARNING, "AudioConverterFillComplexBuffer() failed (%d)\n", (int)status);
        return AVERROR_EXTERNAL;
    }
    if (output_packet_count == 0) {
        return avpkt->size;
    }

    // get the output format
    status = AudioConverterGetProperty(self->converter,
        kAudioConverterCurrentOutputStreamDescription,
        &output_format_size,
        &output_format);
    if (status != noErr) {
        av_log(avctx, AV_LOG_WARNING, "AudioConverterGetProperty() failed (%d)\n", (int)status);
        return AVERROR_EXTERNAL;
    }

    // compute the number of samples we decoded
    if (output_format.mChannelsPerFrame) {
        output_sample_count = output_buffers.mBuffers[0].mDataByteSize / (2 * output_format.mChannelsPerFrame);
    }
    else {
        return 0;
    }

    // setup and copy the output frame
    frame->format = AV_SAMPLE_FMT_S16;
    frame->nb_samples = output_sample_count;
    frame->sample_rate = (unsigned int)output_format.mSampleRate;
    frame->channels = output_format.mChannelsPerFrame;
    frame->channel_layout = AV_CH_LAYOUT_STEREO; // FIXME: hardcoded
    ff_result = avctx->get_buffer2(avctx, frame, 0);
    if (ff_result != 0) {
        av_log(avctx, AV_LOG_WARNING, "get_buffer2 failed (%d)\n", ff_result);
        return ff_result;
    }
    memcpy(frame->data[0],
        self->output_buffer,
        frame->channels * output_sample_count * 2);

    // copy some of the parameters to the codec context (shouldn't be needed, but some progs depend on it)
    avctx->sample_fmt = AV_SAMPLE_FMT_S16;
    avctx->frame_size = frame->nb_samples;
    avctx->channels = frame->channels;
    avctx->channel_layout = frame->channel_layout;

    // done!
    *got_frame = 1;

    return avpkt->size;
}

/*----------------------------------------------------------------*/
static void
at_decoder_flush(AVCodecContext *avctx)
{
    //AT_DecoderContext* self = avctx->priv_data;

    av_log(avctx, AV_LOG_TRACE, "at_decoder_flush\n");

}

/*----------------------------------------------------------------
| AAC module definitions
+----------------------------------------------------------------*/
static const AVOption at_aac_decoder_options[] = {
    { NULL }
};

static const AVClass at_aac_decoder_class = {
    "audiotoolbox aac decoder", av_default_item_name, at_aac_decoder_options, LIBAVUTIL_VERSION_INT
};

AVCodec ff_aac_audiotoolbox_decoder = {
    .name = "aac_audiotoolbox",
    .long_name = "AudioToolbox AAC decoder",
    .type = AVMEDIA_TYPE_AUDIO,
    .id = AV_CODEC_ID_AAC,
    .priv_data_size = sizeof(AT_DecoderContext),
    .init = at_decoder_init,
    .decode = at_decoder_decode,
    .close = at_decoder_close,
    .flush = at_decoder_flush,
    .capabilities = AV_CODEC_CAP_DR1 | AV_CODEC_CAP_CHANNEL_CONF,
    .priv_class = &at_aac_decoder_class
};

/*----------------------------------------------------------------
| MP3 module definitions
+----------------------------------------------------------------*/
static const AVOption at_mp3_decoder_options[] = {
    { NULL }
};

static const AVClass at_mp3_decoder_class = {
    "audiotoolbox mp3 decoder", av_default_item_name, at_mp3_decoder_options, LIBAVUTIL_VERSION_INT
};

AVCodec ff_mp3_audiotoolbox_decoder = {
    .name = "mp3_audiotoolbox",
    .long_name = "AudioToolbox MP3 decoder",
    .type = AVMEDIA_TYPE_AUDIO,
    .id = AV_CODEC_ID_MP3,
    .priv_data_size = sizeof(AT_DecoderContext),
    .init = at_decoder_init,
    .decode = at_decoder_decode,
    .close = at_decoder_close,
    .flush = at_decoder_flush,
    .capabilities = AV_CODEC_CAP_DR1 | AV_CODEC_CAP_CHANNEL_CONF,
    .priv_class = &at_mp3_decoder_class
};