//
// Created by zsh on 2024/6/22.
//
#ifdef __cplusplus
extern "C" {
#endif
// 包含FFmpeg的头文件
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/log.h>
#ifdef __cplusplus

}
#endif
#include <iostream>
#include"stdafx.h"
int main(int argc,char *argv[]){
    int ret =-1;
    int idx =-1;

    //1.处理一些参数
    char *src =nullptr;
    char *dst =nullptr;

    AVFormatContext *pFormatCtx =nullptr;
    AVFormatContext *oFormatCtx  =nullptr;

    AVOutputFormat *outFmt  =nullptr;
    AVStream *outStream  =nullptr;
    AVStream *inSteam =nullptr;
    AVPacket pkt ;

    // 日志信息
    av_log_set_level(AV_LOG_DEBUG);
    if(argc<3){
        av_log(nullptr,AV_LOG_ERROR,"Usage:%s <input file> <output file>\n",argv[0]);
        exit(-1);
    }

    src = argv[1];
    dst = argv[2];

    // 2.打开多媒体输入文件
    ret = avformat_open_input(&pFormatCtx,src,nullptr,nullptr);
    if(ret<0){
        av_log(nullptr,AV_LOG_ERROR,"Could not open input file:%s\n",src);
        exit(-1);
    }

    // 3.获取多媒体文件信息
    ret = av_find_best_stream(pFormatCtx,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
    if(ret<0){
        av_log(nullptr,AV_LOG_ERROR,"Failed to retrieve input stream information\n");
        goto _ERROR;
    }
    // 打开目的多媒体文件
    oFormatCtx = avformat_alloc_context();
    if(oFormatCtx==nullptr){
        av_log(nullptr,AV_LOG_ERROR,"Failed to allocate output context\n");
        goto _ERROR;
    }

    outFmt = av_guess_format(nullptr,dst,nullptr);
    if(outFmt==nullptr){
        av_log(nullptr,AV_LOG_ERROR,"Failed to guess output format\n");
        goto _ERROR;
    }
    oFormatCtx->oformat = outFmt;

    // 为目的文件，创建一个新的视频流
    outStream = avformat_new_stream(oFormatCtx,nullptr);
    // 设置视频参数
    inSteam = pFormatCtx->streams[idx];
    avcodec_parameters_copy(outStream->codecpar,pFormatCtx->streams[ret]->codecpar);
    outStream->codecpar->codec_tag = 0;

    // 绑定
    ret = avio_open2(&oFormatCtx->pb,dst,AVIO_FLAG_WRITE, nullptr, nullptr);
    if(ret<0){
        av_log(nullptr,AV_LOG_ERROR,"Failed to open output file:%s\n",dst);
        goto _ERROR;
    }
    // 写入头信息
    ret = avformat_write_header(oFormatCtx,nullptr);
    if(ret<0){
        av_log(nullptr,AV_LOG_ERROR,"Failed to write output file header\n");
        goto _ERROR;
    }

    // 写多媒体文件到目的文件
    ret = avformat_write_header(oFormatCtx,nullptr);
    if(ret<0){
        //av_log(nullptr,AV_LOG_ERROR,"Failed to write output file header:%s\n", av_err2str(ret));
        goto _ERROR;
    }
    while(av_read_frame(pFormatCtx,&pkt)>=0) {
        if (pkt.stream_index == idx) {

            pkt.pts = av_rescale_q_rnd(pkt.pts, inSteam->time_base, outStream->time_base,
                                       (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.dts = av_rescale_q_rnd(pkt.dts, inSteam->time_base, outStream->time_base,
                                       (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.duration = av_rescale_q(pkt.duration, inSteam->time_base, outStream->time_base);
            pkt.stream_index = 0;
            pkt.pos = -1;
            av_interleaved_write_frame(oFormatCtx, &pkt);
        }
        av_packet_unref(&pkt);
    }
    // 写入尾信息
    av_write_trailer(oFormatCtx);

_ERROR:

    if(oFormatCtx->pb){
        avio_close(oFormatCtx->pb);
        oFormatCtx->pb = nullptr;
    }
    if(oFormatCtx){
        avformat_free_context(oFormatCtx);
        oFormatCtx = nullptr;
    }
    if(pFormatCtx){
        avformat_free_context(pFormatCtx);
        pFormatCtx = nullptr;
    }
    return 0;
}

