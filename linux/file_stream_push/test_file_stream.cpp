#include <stdio.h>
#include "ffmpeg_comm.h"

int main()
{
	const char *url = "../res/xrq-sxtpy.flv";
	const char *o_file = "rtmp://192.168.1.104/live";
	ffmpg  ff;

	AVFormatContext *i_ps = NULL;
	AVFormatContext *o_ps = NULL;

	ff.ffmpg_init();
	ff.input_start(&i_ps, url);

	ff.get_stream_inf(i_ps);

	ff.dump_av_inf(*i_ps, url, 0);

	ff.alloc_context(&o_ps, o_file);
	ff.out_start(o_ps, o_file, *i_ps);

	ff.dump_av_inf(*o_ps, url, 1);
	
	ff.push_stream(*o_ps, *i_ps);

	 getchar();
	 
	return 0;
}


