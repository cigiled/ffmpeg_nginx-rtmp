#ifndef __FFMPEG_COMM_H__
#define __FFMPEG_COMM_H__

#include <iostream>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;


#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL2/SDL.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"

#ifdef __cplusplus
}
#endif


class ffmpg
{
	public:
		ffmpg();
		~ffmpg();

		int ffmpg_init();
		int input_start(AVFormatContext **ps,  const char *url);
		int get_stream_inf(AVFormatContext *ps);
		int alloc_context(AVFormatContext **o_ps,  const char * o_file);
		int out_start(AVFormatContext *&o_ps, const char *o_file, AVFormatContext &i_ps);
		int dump_av_inf(AVFormatContext &ps,  const char *url,  int value);
		int push_stream(AVFormatContext &o_ps, AVFormatContext &i_ps);
		void adjust_pts_dts(AVPacket *pkt, AVFormatContext *o_ps, AVFormatContext *ps);
		int speed_contrl(AVPacket *pkt,	AVFormatContext *o_ps, AVFormatContext *ps);

	protected:
			void p_error(int ret_t);
	private:
			long long StartTime;
};

#endif
