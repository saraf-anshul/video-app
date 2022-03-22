//
// Created by Anshul Saraf on 10/03/22.
//

#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavformat/avformat.h"
}

#define INBUF_SIZE 4096


static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx;
static AVStream *video_stream = NULL, *audio_stream = NULL;
static const char *src_filename = NULL;
static int video_dst_bufsize;
static int video_stream_idx = -1, audio_stream_idx = -1;
static AVFrame *frame = NULL;
static int video_frame_count = 0;

const char *media_type_string(enum AVMediaType media_type)
{
    switch (media_type) {
        case AVMEDIA_TYPE_VIDEO:      return "video";
        case AVMEDIA_TYPE_AUDIO:      return "audio";
        case AVMEDIA_TYPE_DATA:       return "data";
        case AVMEDIA_TYPE_SUBTITLE:   return "subtitle";
        case AVMEDIA_TYPE_ATTACHMENT: return "attachment";
        default:                      return "unknown";
    }
}

static int open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecParameters *dec_param = NULL;
    const AVCodec *dec = NULL;
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                media_type_string(type), src_filename);
        return ret;
    }
    else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];
        // find decoder for the stream
        dec_param = st->codecpar;
        dec = avcodec_find_decoder(dec_param->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    media_type_string(type));
            return ret;
        }
        video_dec_ctx = avcodec_alloc_context3(dec);
        avcodec_parameters_to_context(video_dec_ctx, dec_param);
        if ((ret = avcodec_open2(video_dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    media_type_string(type));
            return ret;
        }
    }
    return 0;
}

static void video_decode_example()
{
//    AVCodecContext *c = NULL;
    AVPacket avpkt;

    av_init_packet(&avpkt);

    printf("Video decoding\n");
    int ret = 0;
    frame = av_frame_alloc();

    uint8_t* imagebuffer = NULL;
    // Read all the frames
    while (av_read_frame(fmt_ctx, &avpkt) >= 0) {
        if (avpkt.size == 0)
            break;

        fprintf(stderr, "Video frame size %d\r\n", avpkt.size);

        int got_frame = 0;

        // This function might fail because of parameter set packets, just ignore and continue
        ret = avcodec_send_packet(video_dec_ctx, &avpkt);
        if (ret < 0) {
            fprintf(stderr, "avcodec_send_packet ret < 0\n");
            continue;
        }

        // Receive the uncompressed frame back
        ret = avcodec_receive_frame(video_dec_ctx, frame);
        if (ret < 0) {
            // Sometimes we cannot get a new frame, continue in this case
            if (ret == AVERROR(EAGAIN)) continue;

            fprintf(stderr, "avcodec_receive_frame ret < 0\n");
            break;
        }

        // Calculate output buffer requirements
        uint32_t image_buffer_size = av_image_get_buffer_size(AVPixelFormat(frame->format), frame->width, frame->height, 1);

        // Print frame info
        fprintf(stderr, "[%d] Got frame of size: %dx%d (%d bytes)\r\n", video_dec_ctx->frame_number,
                frame->width,
                frame->height,
                image_buffer_size
        );

        // Use temp buffer for the video data
        if (imagebuffer == nullptr) imagebuffer = new uint8_t[image_buffer_size];
        av_image_copy_to_buffer(imagebuffer, image_buffer_size, frame->data, frame->linesize, AVPixelFormat(frame->format), frame->width, frame->height, 1);

        // Dump the frame to a file![](../../../../Downloads/out_864x486.png)
        FILE* out = fopen("/Users/anshulsaraf/Downloads/out_864x486.jpg", "wb");
        fwrite(imagebuffer, image_buffer_size, 1, out);
        fclose(out);
//        break; // for one call based approach
    }

    delete imagebuffer;
    fprintf(stderr, "out of loop av_read_frame\n");
    avcodec_close(video_dec_ctx);
    av_free(video_dec_ctx);
    av_frame_free(&frame);
}

int main(int argc, char **argv)
{

    /* register all the codecs */

    // ffmpeg 4.0 deprecated registering, might still need for libav
    //fprintf(stderr, "Register everything\r\n");
    //av_register_all();
    //avcodec_register_all();


    src_filename = "/Users/anshulsaraf/Downloads/40_sss_loop.mov";

    // open input file, and allocate format context
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }

    // retrieve stream information
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    // Open video context
    if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) < 0) {
        fprintf(stderr, "open_codec_context failed\n");
        exit(1);
    }

    video_decode_example();

    return 0;
}