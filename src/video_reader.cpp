//
// Created by Anshul Saraf on 13/03/22.
//

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

            if (!dec) {
                fprintf(stderr, "Failed to find %s codec\n",media_type_string(type));
                return ret;
            }
            av_codec_ctx = avcodec_alloc_context3(dec);
            avcodec_parameters_to_context(av_codec_ctx, dec_param);

            if(multi_thread) {
                // set codec to automatically determine how many threads suits best for the decoding job
                av_codec_ctx->thread_count = 0;

                if (dec->capabilities | AV_CODEC_CAP_FRAME_THREADS)
                    av_codec_ctx->thread_type = FF_THREAD_FRAME;
                else if (dec->capabilities | AV_CODEC_CAP_SLICE_THREADS)
                    av_codec_ctx->thread_type = FF_THREAD_SLICE;
                else
                    av_codec_ctx->thread_count = 1; //don't use multithreading
            }

            if ((ret = avcodec_open2(av_codec_ctx, dec, nullptr)) < 0) {
                fprintf(stderr, "Failed to open %s codec\n",media_type_string(type));
                return ret;
            }
        }
        return 0;
    }

    int LibavVideoDecoder::InitDecoder(std::string& sourcePath, bool loop, bool enable_multi_threading){
        is_decoding = true;
        is_loop = loop;
        src_filename = sourcePath.c_str();
        multi_thread = enable_multi_threading;

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

        auto avg_frame_rate = av_format_ctx->streams[video_stream_idx]->avg_frame_rate;
        avg_fps = avg_frame_rate.num / avg_frame_rate.den;

        return (int) image_buffer_size;

    }

    bool LibavVideoDecoder::Decode(int64_t elapsedTime, uint8_t* image_buffer, double & pt_in_seconds){
        int ret;
        while (av_read_frame(av_format_ctx, av_packet) >= 0) {
            if (av_packet->size == 0) {
                break;
            }

            if (av_packet->stream_index != video_stream_idx) {
                fprintf(stderr,"wrong stream index : , %d \n", av_packet->stream_index);
                continue;
            }

            fprintf(stderr, "Video frame size %d\r\n", av_packet->size);

            // This function might fail because of parameter set packets, just ignore and continue
            ret = avcodec_send_packet(av_codec_ctx, av_packet);
            if (ret < 0) {
                fprintf(stderr,"avcodec_send_packet ret < 0, %d\n", ret);
                av_packet_unref(av_packet);
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
