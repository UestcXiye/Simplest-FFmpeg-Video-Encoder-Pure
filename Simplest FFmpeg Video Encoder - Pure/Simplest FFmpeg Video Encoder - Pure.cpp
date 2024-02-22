// Simplest FFmpeg Video Encoder - Pure.cpp : 定义控制台应用程序的入口点。
//

/**
* 最简单的基于 FFmpeg 的视频编码器（纯净版）
* Simplest FFmpeg Video Encoder Pure
*
* 源程序：
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 修改：
* 刘文晨 Liu Wenchen
* 812288728@qq.com
* 电子科技大学/电子信息
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* 本程序实现了 YUV 像素数据编码为视频码流（H264，MPEG2，VP8 等等）。
* 它仅仅使用了 libavcodec（而没有使用 libavformat）。
* 是最简单的 FFmpeg 视频编码方面的教程。
* 通过学习本例子可以了解 FFmpeg 的编码流程。
*
* This software encode YUV420P data to video bitstream
* (Such as H.264, H.265, VP8, MPEG2 etc).
* It only uses libavcodec to encode video (without libavformat)
* It's the simplest video encoding software based on FFmpeg.
* Suitable for beginner of FFmpeg
*/

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

// 解决报错：fopen() 函数不安全
#pragma warning(disable:4996)

// 解决报错：无法解析的外部符号 __imp__fprintf，该符号在函数 _ShowError 中被引用
#pragma comment(lib, "legacy_stdio_definitions.lib")
extern "C"
{
	// 解决报错：无法解析的外部符号 __imp____iob_func，该符号在函数 _ShowError 中被引用
	FILE __iob_func[3] = { *stdin, *stdout, *stderr };
}

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif

// test different codec
#define TEST_H264  0
#define TEST_HEVC  1

int main(int argc, char* argv[])
{
	AVCodec *pCodec;
	AVCodecContext *pCodecCtx = NULL;
	AVFrame *pFrame;
	AVPacket pkt;

	FILE *fp_in;
	FILE *fp_out;

	int ret;
	int got_output = 0;
	int y_size;
	int i = 0;
	int framecnt = 0;

	char filename_in[] = "ds_480x272.yuv";

#if TEST_HEVC
	AVCodecID codec_id = AV_CODEC_ID_HEVC;
	char filename_out[] = "ds.hevc";
#else
	AVCodecID codec_id = AV_CODEC_ID_H264;
	char filename_out[] = "ds.h264";
#endif

	const int in_width = 480, in_height = 272; // Input data's width and height
	int framenum = 100; // Frames to encode 

	avcodec_register_all();

	pCodec = avcodec_find_encoder(codec_id);
	if (!pCodec)
	{
		// 没有找到合适的编码器
		printf("Can't find encoder.\n");
		return -1;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx)
	{
		printf("Can't allocate video codec context.\n");
		return -1;
	}

	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = in_width;
	pCodecCtx->height = in_height;
	pCodecCtx->bit_rate = 400000;
	pCodecCtx->gop_size = 10;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	// H.264
	// pCodecCtx->me_range = 16;
	// pCodecCtx->max_qdiff = 4;
	// pCodecCtx->qcompress = 0.6;

	// pCodecCtx->qmin = 10;
	// pCodecCtx->qmax = 51;

	// Optional Param
	pCodecCtx->max_b_frames = 1;

	if (codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);
	}

	ret = avcodec_open2(pCodecCtx, pCodec, NULL);
	if (ret < 0)
	{
		// 编码器打开失败
		printf("Failed to open encoder.\n");
		return -1;
	}

	pFrame = av_frame_alloc();
	if (!pFrame)
	{
		printf("Can't allocate video frame.\n");
		return -1;
	}

	pFrame->format = pCodecCtx->pix_fmt;
	pFrame->width = pCodecCtx->width;
	pFrame->height = pCodecCtx->height;

	ret = av_image_alloc(pFrame->data, pFrame->linesize, pCodecCtx->width, pCodecCtx->height,
		pCodecCtx->pix_fmt, 16);
	if (ret < 0)
	{
		printf("Can't allocate raw picture buffer.\n");
		return -1;
	}

	// Input raw data
	fp_in = fopen(filename_in, "rb");
	if (!fp_in)
	{
		printf("Can't open %s.\n", filename_in);
		return -1;
	}
	// Output bitstream
	fp_out = fopen(filename_out, "wb");
	if (!fp_out)
	{
		printf("Can't open %s.\n", filename_out);
		return -1;
	}

	y_size = pCodecCtx->width * pCodecCtx->height;

	// Encode
	for (i = 0; i < framenum; i++)
	{
		av_init_packet(&pkt);
		pkt.data = NULL; // packet data will be allocated by the encoder
		pkt.size = 0;
		// Read raw YUV data
		if (fread(pFrame->data[0], 1, y_size, fp_in) <= 0 ||
			fread(pFrame->data[1], 1, y_size / 4, fp_in) <= 0 ||
			fread(pFrame->data[2], 1, y_size / 4, fp_in) <= 0)
		{
			return -1;
		}
		else if (feof(fp_in))
		{
			break;
		}

		// PTS
		pFrame->pts = i;
		// Encode the frame
		ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_output);
		if (ret < 0)
		{
			printf("Error encoding frame.\n");
			return -1;
		}

		if (got_output)
		{
			printf("Succeed to encode frame: %5d\tsize:%5d.\n", framecnt, pkt.size);
			framecnt++;
			fwrite(pkt.data, 1, pkt.size, fp_out);
			av_free_packet(&pkt);
		}
	}

	// Flush Encoder
	for (got_output = 1; got_output; i++)
	{
		ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_output);
		if (ret < 0)
		{
			printf("Error encoding frame.\n");
			return -1;
		}

		if (got_output)
		{
			printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d.\n", pkt.size);
			fwrite(pkt.data, 1, pkt.size, fp_out);
			av_free_packet(&pkt);
		}
	}

	fclose(fp_out);
	avcodec_close(pCodecCtx);
	av_free(pCodecCtx);
	av_freep(&pFrame->data[0]);
	av_frame_free(&pFrame);

	system("pause");
	return 0;
}

