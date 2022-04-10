//
// Created by Anshul Saraf on 13/03/22.
//

#ifndef SHUTTER_LIBAVVIDEODECODER_H
#define SHUTTER_LIBAVVIDEODECODER_H

#include <string>

extern "C"{
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
};

namespace ShutterAndroidJNI{

    class LibavVideoDecoder {
    public:
        LibavVideoDecoder() = default;
        ~LibavVideoDecoder();
        int InitDecoder(std::string& sourcePath, bool loop, bool enable_multi_threading);
        bool Decode(int64_t elapsedTime, uint8_t* image_buffer, double& pt_in_seconds);
        void ReleaseDecoder();

        int open_codec_context(enum AVMediaType type);
        int getBufferSize() const;

        static const char* media_type_string(enum AVMediaType media_type){
            switch (media_type) {
                case AVMEDIA_TYPE_VIDEO:      return "video";
                case AVMEDIA_TYPE_AUDIO:      return "audio";
                case AVMEDIA_TYPE_DATA:       return "data";
                case AVMEDIA_TYPE_SUBTITLE:   return "subtitle";
                case AVMEDIA_TYPE_ATTACHMENT: return "attachment";
                default:                      return "unknown";
            }
        }
        static AVPixelFormat correct_for_deprecated_pixel_format(AVPixelFormat pix_fmt) {
            // Fix swscaler deprecated pixel format warning
            // (YUVJ has been deprecated, change pixel format to regular YUV)
            switch (pix_fmt) {
                case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
                case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
                case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
                case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
                default:                  return pix_fmt;
            }
        }

        int width = 0, height = 0;
    private:
        bool is_decoding = false;
        bool is_loop = true;
        AVCodecContext* av_codec_ctx = nullptr;
        AVFormatContext* av_format_ctx = nullptr;
        AVFrame* av_frame = nullptr;
        AVPacket* av_packet = nullptr;
        SwsContext* sws_scaler_ctx = nullptr;


        AVPixelFormat format;
        AVPixelFormat outputFormat = AV_PIX_FMT_RGBA;
        int video_stream_idx = -1;
        uint32_t image_buffer_size = 0;

        int avg_fps = 0;

        bool multi_thread = false;

        const char* src_filename = nullptr;
        int loop_cnt = 0 ;
    };

}

#endif //SHUTTER_LIBAVVIDEODECODER_H
