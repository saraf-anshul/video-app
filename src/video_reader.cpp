//#include "video_reader.hpp"
//
//// av_err2str returns a temporary array. This doesn't work in gcc.
//// This function can be used as a replacement for av_err2str.
//static const char* av_make_error(int errnum) {
//    static char str[AV_ERROR_MAX_STRING_SIZE];
//    memset(str, 0, sizeof(str));
//    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
//}
//
//static AVPixelFormat correct_for_deprecated_pixel_format(AVPixelFormat pix_fmt) {
//    // Fix swscaler deprecated pixel format warning
//    // (YUVJ has been deprecated, change pixel format to regular YUV)
//    switch (pix_fmt) {
//        case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
//        case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
//        case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
//        case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
//        default:                  return pix_fmt;
//    }
//}
//
//bool video_reader_open(VideoReaderState* state, const char* filename) {
//
//    // Unpack members of state
//    auto& width = state->width;
//    auto& height = state->height;
//    auto& time_base = state->time_base;
//    auto& av_format_ctx = state->av_format_ctx;
//    auto& av_codec_ctx = state->av_codec_ctx;
//    auto& video_stream_index = state->video_stream_index;
//    auto& av_frame = state->av_frame;
//    auto& av_packet = state->av_packet;
//
//    // Open the file using libavformat
//    av_format_ctx = avformat_alloc_context();
//    if (!av_format_ctx) {
//        printf("Couldn't created AVFormatContext\n");
//        return false;
//    }
//
//    if (avformat_open_input(&av_format_ctx, filename, NULL, NULL) != 0) {
//        printf("Couldn't open video file\n");
//        return false;
//    }
//
//    // Find the first valid video stream inside the file
//    video_stream_index = -1;
//    AVCodecParameters* av_codec_params;
//    const AVCodec* av_codec ;
//    for (int i = 0; i < av_format_ctx->nb_streams; ++i) {
//        av_codec_params = av_format_ctx->streams[i]->codecpar;
//        if (av_codec_params->codec_type == AVMEDIA_TYPE_VIDEO) {
//            video_stream_index = i;
//            width = av_codec_params->width;
//            height = av_codec_params->height;
//            break;
//        }
//    }
//
//    if (video_stream_index == -1) {
//        printf("Couldn't find valid video stream inside file\n");
//        return false;
//    }
//
//    av_codec = avcodec_find_decoder(av_format_ctx->streams[video_stream_index]->codecpar->codec_id);
//    time_base = av_format_ctx->streams[video_stream_index]->time_base;
//
//
//    // Set up a codec context for the decoder
//    av_codec_ctx = avcodec_alloc_context3(av_codec);
//    if (!av_codec_ctx) {
//        printf("Couldn't create AVCodecContext\n");
//        return false;
//    }
//    if (avcodec_parameters_to_context(av_codec_ctx, av_codec_params) < 0) {
//        printf("Couldn't initialize AVCodecContext\n");
//        return false;
//    }
//    if (avcodec_open2(av_codec_ctx, av_codec, NULL) < 0) {
//        printf("Couldn't open codec\n");
//        return false;
//    }
//
//    av_frame = av_frame_alloc();
//    if (!av_frame) {
//        printf("Couldn't allocate AVFrame\n");
//        return false;
//    }
//    av_packet = av_packet_alloc();
//    if (!av_packet) {
//        printf("Couldn't allocate AVPacket\n");
//        return false;
//    }
//
//    return true;
//}
//
//bool video_reader_read_frame(VideoReaderState* state, uint8_t* frame_buffer, int64_t* pts) {
//
//    // Unpack members of state
//    auto& width = state->width;
//    auto& height = state->height;
//    auto& av_format_ctx = state->av_format_ctx;
//    auto& av_codec_ctx = state->av_codec_ctx;
//    auto& video_stream_index = state->video_stream_index;
//    auto& av_frame = state->av_frame;
//    auto& av_packet = state->av_packet;
//    auto& sws_scaler_ctx = state->sws_scaler_ctx;
//
////    if(frame_buffer != nullptr)
////    free(frame_buffer);
//
//    // Decode one frame
//    int response;
//    while (av_read_frame(av_format_ctx, av_packet) >= 0) {
//        if (av_packet->stream_index != video_stream_index) {
//            av_packet_unref(av_packet);
//            continue;
//        }
//
//        response = avcodec_send_packet(av_codec_ctx, av_packet);
//        if (response < 0) {
//            printf("Failed to decode packet: %s\n", av_make_error(response));
//            return false;
//        }
//
//        response = avcodec_receive_frame(av_codec_ctx, av_frame);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            av_packet_unref(av_packet);
//            printf("reached eof\n");
//            return false;
//        } else if (response < 0) {
//            printf("Failed to decode packet: %s\n", av_make_error(response));
//            return false;
//        }
//
//        av_packet_unref(av_packet);
//        break;
//    }
//    *pts = av_frame->pts;
//
//    printf(
//            "Frame %c (%d) pts %lld dts %lld key_frame %d [coded_picture_number %d, display_picture_number %d]\n",
//            av_get_picture_type_char(av_frame->pict_type),
//            av_codec_ctx->frame_number,
//            av_frame->pts,
//            av_frame->pkt_dts,
//            av_frame->key_frame,
//            av_frame->coded_picture_number,
//            av_frame->display_picture_number
//    );
//
//    // Set up sws scaler
//    if (!sws_scaler_ctx) {
//        auto source_pix_fmt = correct_for_deprecated_pixel_format(av_codec_ctx->pix_fmt);
//        sws_scaler_ctx = sws_getContext(width, height, source_pix_fmt,
//                                        width, height, AV_PIX_FMT_RGB24,
//                                        SWS_BICUBIC, NULL, NULL, NULL);
//    }
//    if (!sws_scaler_ctx) {
//        printf("Couldn't initialize sw scaler\n");
//        return false;
//    }
//
//    uint8_t* dest[4] = { frame_buffer, NULL, NULL, NULL };
//    int dest_linesize[4] = { width * 4, 0, 0, 0 };
//    sws_scale(sws_scaler_ctx, av_frame->data, av_frame->linesize, 0, av_frame->height, dest, dest_linesize);
////    av_image_copy_to_buffer(frame_buffer, av_frame->height*av_frame->width*4, av_frame->data, av_frame->linesize, AVPixelFormat(av_frame->format), av_frame->width, av_frame->height, 1);
//
//
//    return true;
//}
//
//bool video_reader_seek_frame(VideoReaderState* state, int64_t ts) {
//
//    // Unpack members of state
//    auto& av_format_ctx = state->av_format_ctx;
//    auto& av_codec_ctx = state->av_codec_ctx;
//    auto& video_stream_index = state->video_stream_index;
//    auto& av_packet = state->av_packet;
//    auto& av_frame = state->av_frame;
//
//    av_seek_frame(av_format_ctx, video_stream_index, ts, AVSEEK_FLAG_BACKWARD);
//
//    // av_seek_frame takes effect after one frame, so I'm decoding one here
//    // so that the next call to video_reader_read_frame() will give the correct
//    // frame
//    int response;
//    while (av_read_frame(av_format_ctx, av_packet) >= 0) {
//        if (av_packet->stream_index != video_stream_index) {
//            av_packet_unref(av_packet);
//            continue;
//        }
//
//        response = avcodec_send_packet(av_codec_ctx, av_packet);
//        if (response < 0) {
//            printf("Failed to decode packet: %s\n", av_make_error(response));
//            return false;
//        }
//
//        response = avcodec_receive_frame(av_codec_ctx, av_frame);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            av_packet_unref(av_packet);
//            continue;
//        } else if (response < 0) {
//            printf("Failed to decode packet: %s\n", av_make_error(response));
//            return false;
//        }
//
//        av_packet_unref(av_packet);
//        break;
//    }
//
//    return true;
//}
//
//void video_reader_close(VideoReaderState* state) {
//    sws_freeContext(state->sws_scaler_ctx);
//    avformat_close_input(&state->av_format_ctx);
//    avformat_free_context(state->av_format_ctx);
//    av_frame_free(&state->av_frame);
//    av_packet_free(&state->av_packet);
//    avcodec_free_context(&state->av_codec_ctx);
//}


//
// Created by Anshul Saraf on 13/03/22.
//

//#include "include/LibavVideoDecoder.h"
#include "video_reader.hpp"

namespace ShutterAndroidJNI{

    LibavVideoDecoder::~LibavVideoDecoder() {
        if(is_decoding){
            ReleaseDecoder();
        }
    }

    int LibavVideoDecoder::open_codec_context(enum AVMediaType type) {
        int ret;
        AVStream *st;
        AVCodecParameters *dec_param;
        const AVCodec *dec;
        ret = av_find_best_stream(av_format_ctx, type, -1, -1, &dec, 0);
        if (ret < 0) {
            fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                    media_type_string(type), src_filename);
            return ret;
        }
        else {
            video_stream_idx = ret;
            st = av_format_ctx->streams[video_stream_idx];

            // find decoder for the stream

            dec_param = st->codecpar;
            // update info
            width = dec_param->width;
            height = dec_param->height;
            format = correct_for_deprecated_pixel_format(AVPixelFormat(dec_param->format));

//            dec = avcodec_find_decoder(dec_param->codec_id);
            if (!dec) {
                fprintf(stderr, "Failed to find %s codec\n",media_type_string(type));
                return ret;
            }
            av_codec_ctx = avcodec_alloc_context3(dec);
            avcodec_parameters_to_context(av_codec_ctx, dec_param);
            if ((ret = avcodec_open2(av_codec_ctx, dec, nullptr)) < 0) {
                fprintf(stderr, "Failed to open %s codec\n",media_type_string(type));
                return ret;
            }
        }
        return 0;
    }

    int LibavVideoDecoder::InitDecoder(std::string& sourcePath, bool loop){
        is_decoding = true;
        is_loop = loop;
        src_filename = sourcePath.c_str();

        // Open the file using libavformat
        av_format_ctx = avformat_alloc_context(); // TODO FIX (NEEDED ?)
        if (!av_format_ctx) {
            fprintf(stderr, "Couldn't create AVFormatContext\n");
            is_decoding = false;
            return -1;
        }
        if (avformat_open_input(&av_format_ctx, src_filename, nullptr, nullptr) < 0) {
            fprintf(stderr,"Could not open source file %s\n", src_filename);
            is_decoding = false;
            return -1;
        }

        // retrieve stream information
        if (avformat_find_stream_info(av_format_ctx, nullptr) < 0) {
            fprintf(stderr,"Could not find stream information\n");
            is_decoding = false;
            return -1;
        }

        // Open video context
        if (open_codec_context(AVMEDIA_TYPE_VIDEO) < 0) {
            fprintf(stderr,"open_codec_context failed\n");
            is_decoding = false;
            return -1;
        }

//        av_init_packet(av_packet);
        av_packet = av_packet_alloc(); // TODO FIX CHECK DEPRECATION
        if (!av_packet) {
            fprintf(stderr,"Couldn't create AVPacket\n");
            is_decoding = false;
            return -1;
        } // TODO , IS NEEDED ?

        av_frame = av_frame_alloc();
        if (!av_frame) {
            fprintf(stderr,"Couldn't create AVFrame\n");
            is_decoding = false;
            return -1;
        } // TODO , IS NEEDED ?

        image_buffer_size = av_image_get_buffer_size(outputFormat, width, height, 1);

        fprintf(stderr,"native source path %s, buffer size %d", src_filename, image_buffer_size);

        return (int) image_buffer_size;

    }

    bool LibavVideoDecoder::Decode(int64_t elapsedTime, uint8_t* image_buffer, double & pt_in_seconds){
        int ret;
        while (av_read_frame(av_format_ctx, av_packet) >= 0) {
            if (av_packet->size == 0) {
                break;
            }

            if (av_packet->stream_index != video_stream_idx) continue; // TODO FIX

            fprintf(stderr, "Video frame size %d\r\n", av_packet->size);

            // This function might fail because of parameter set packets, just ignore and continue
            ret = avcodec_send_packet(av_codec_ctx, av_packet);
            if (ret < 0) {
                fprintf(stderr,"avcodec_send_packet ret < 0\n");
                continue;
            }

            // Receive the uncompressed frame back
            ret = avcodec_receive_frame(av_codec_ctx, av_frame);

            if( ret == AVERROR_EOF || ret == AVERROR(AVERROR_EOF)){
                fprintf(stderr,"EOF END STREAM\n");
                break;
            }
            if (ret < 0) {
                // Sometimes we cannot get a new frame, continue in this case
                if (ret == AVERROR(EAGAIN))
                    continue;

                fprintf(stderr,"avcodec_receive_frame ret < 0\n");
                break;
            }

            // Print frame info
            fprintf(stderr, "[%d] Got frame of size: %dx%d (%d bytes)\r\n", av_codec_ctx->frame_number,
                    av_frame->width,
                    av_frame->height,
                    image_buffer_size
            );
//            SHUTTER_ASSERT( image_buffer_size == av_image_get_buffer_size(AVPixelFormat(av_frame->format), av_frame->width, av_frame->height, 1), "Buffer size mismatch" );

            if (!sws_scaler_ctx) {
                auto source_pix_fmt = correct_for_deprecated_pixel_format(av_codec_ctx->pix_fmt);
                fprintf(stderr,"format : %d\n", av_codec_ctx->pix_fmt);
                sws_scaler_ctx = sws_getContext(width, height, source_pix_fmt,
                                        width, height, outputFormat,
                                        SWS_BICUBIC, NULL, NULL, NULL);
            }
            if (!sws_scaler_ctx) {
                fprintf(stderr,"Couldn't initialize sw scaler\n");

                av_packet_unref(av_packet);
                return false;
            }

            // copy data to buffer
//            av_image_copy_to_buffer(image_buffer, image_buffer_size, av_frame->data, av_frame->linesize, format, width, height, 1);

                uint8_t* dest[4] = { image_buffer, NULL, NULL, NULL };
                int dest_linesize[4] = { width * 4, 0, 0, 0 };
                sws_scale(sws_scaler_ctx, av_frame->data, av_frame->linesize, 0, av_frame->height, dest, dest_linesize);

            // if here , we read 1 frame successfully
            auto pts = av_frame->pts;
            auto num = av_format_ctx->streams[video_stream_idx]->time_base.num;
            auto den = av_format_ctx->streams[video_stream_idx]->time_base.den;

            pt_in_seconds = (double)pts * (double)num / (double)den;

            av_packet_unref(av_packet);
            return true;
        }
        if( is_loop ){
            loop_cnt++;
            fprintf(stderr,"LOOPING %dth time\n", loop_cnt);
            if(loop_cnt > 5){
                fprintf(stderr,"LOOPED %d times, now releasing\n", loop_cnt);

                av_packet_unref(av_packet);
                return false;
            }
            auto stream = av_format_ctx->streams[video_stream_idx];
            avio_seek(av_format_ctx->pb, 0, SEEK_SET);
            avformat_seek_file(av_format_ctx, video_stream_idx, av_format_ctx->start_time, av_format_ctx->start_time, av_format_ctx->duration, 0);

            av_packet_unref(av_packet);
            return true;
        }

        av_packet_unref(av_packet);
        return false;
    }

    void LibavVideoDecoder::ReleaseDecoder(){
        avcodec_free_context(&av_codec_ctx); // avcodec_close(av_codec_ctx);
        av_frame_free(&av_frame);
        avformat_close_input(&av_format_ctx);
        avformat_free_context(av_format_ctx);
        av_packet_free(&av_packet);
        image_buffer_size = -1;
        width = -1;
        height = -1;

        is_decoding = false;
        fprintf(stderr, "Decoder Destroyed\n");
        sws_freeContext(sws_scaler_ctx);
    }

    int LibavVideoDecoder::getBufferSize() const {
        return image_buffer_size;
    }
}
