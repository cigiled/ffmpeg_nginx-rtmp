#include "ffmpeg_comm.h"


ffmpg::ffmpg()
{

}

ffmpg::~ffmpg()
{

}

static double r2d(AVRational rat)
{
	return  (rat.den == 0 || rat.num == 0) ? 0. : ((double)rat.num/ (double)rat.den);
}

void ffmpg::p_error(int ret_t)
{
	char buf[1024];
	av_strerror(ret_t, buf, sizeof(buf));
	cout << buf << endl;
	getchar();
}

int ffmpg::ffmpg_init()
{
	av_register_all();
	avfilter_register_all();
	avformat_network_init();
	return 0;
}

int ffmpg::timeout_cb(void *tm)
{
    int  timeout  = 3;
	long long *lastReadPacktTime = (long long *)tm;
	
    if(av_gettime() - *lastReadPacktTime > timeout *1000 *1000)
    {
        return -1;
    }
    return 0;
}


int ffmpg::input_start(AVFormatContext **i_ps,  const char *url)
{
	//当ps是NULL时,avformat_open_input调用avformat_alloc_context分配内存,并写信息到ps.
	PRT_COND(url == NULL, "Input URL is NULL, exit !");

	//注册网络超时中断处理函数.
	i_ps->interrupt_callback.callback = timeout_cb;
	i_ps->interrupt_callback.opaque = av_gettime();//函数参数.

	//指定传输协议(当推rtsp流时)
	#if 0
	if(rtsp)
	{
		AVDictionary *options = NULL;
		AVDictionary *optionsDict = NULL;
		av_dict_set(&options, "rtsp_transport", "udp", 0);
		av_dict_set(&optionsDict, "stimeout", "5000000", 0);//设置网络超时时间.
	}
	#endif
	
	int ret = avformat_open_input(i_ps, url, NULL, NULL);
	if (ret != 0)
	{
		 p_error(ret);
	   return -1;
	}

	cout << "open file：" << url << "success !" << endl;

	return 0;
}

int ffmpg::get_stream_inf(AVFormatContext *ps)
{	
	int ret = avformat_find_stream_info(ps, 0);
	if(ret < 0) 
	{
		p_error(ret);
		return -1;
	}

	return 0;
}

int ffmpg::alloc_context(AVFormatContext **o_ps,  const char *o_file, int format)
{
	const char *fmt[8] = {"flv", "fmp4", "rtsp", "mpegts", NULL};
	//指定格式为flv.
	int ret = avformat_alloc_output_context2(o_ps, 0, fmt[format], o_file);
	if (ret < 0)
	{
		p_error(ret);
		return -1;
	}

	return 0;
}

int ffmpg::out_start(AVFormatContext *&o_ps, const char *o_file, AVFormatContext &i_ps)
{
	PRT_COND(o_ps == NULL, "Input o_file is NULL, exit !");
	PRT_COND(o_file == NULL, "Input o_file is NULL, exit !");
	
	unsigned int i;
	int ret = 0;
	for (i = 0; i < i_ps.nb_streams; i++)
	{
		AVStream *o_new = avformat_new_stream(o_ps, i_ps.streams[i]->codec->codec);
		//AVStream *o_new = avformat_new_stream(o_ps, ps->streams[i]->codecpar);
		if (!o_new)
		{
			p_error(ret);
			return -1;
		}

		//avcodec_copy_context(o_new->codec, ps->streams[i]->codec);
		ret = avcodec_parameters_copy(o_new->codecpar, i_ps.streams[i]->codecpar);
		o_new->codec->codec_tag= 0;
	}
	cout << "get output info: " << endl;

    //为输出文件格式填充heard
	ret= avformat_write_header(o_ps, 0);
	if (ret < 0)
	{
		p_error(ret);
		return -1;
	}
	cout << "read heard success !" << endl;

	//打开io, 必须打开rtm服务器，否则无法连接到
	ret = avio_open(&o_ps->pb, o_file, AVIO_FLAG_WRITE);
	if (ret < 0)
	{
		cout << "open io faile !" << endl;
		p_error(ret);
		return -1;
	}
    cout << "open io success !" << endl;
	return 0;
}

int  ffmpg::dump_av_inf(AVFormatContext &ps,  const char *url,  int value)
{
	cout << "get stream info:" << endl;
	av_dump_format(&ps, 0, url, value);
	return 0;
}

int ffmpg::push_stream(AVFormatContext &o_ps, AVFormatContext &i_ps)
{
	int ret;
	AVPacket pkt;
	
	StartTime = av_gettime(); //获取当前系统时间:秒+微妙
	for (;;)
	{
		ret = av_read_frame(&i_ps, &pkt);//ps从输入端读码流帧
		if (ret != 0){
			p_error(ret);
			break;
		}

		adjust_pts_dts(&pkt, &o_ps, &i_ps);
		speed_contrl(&pkt, &o_ps, &i_ps);

		//推流,里面会对缓冲对pts,dts进行排序,并且可把ptk空间释放.
		ret = av_interleaved_write_frame(&o_ps, &pkt);
		if (ret != 0){
			p_error(ret);
			break;
		}
	}
	
	return 0;
}

void ffmpg::adjust_pts_dts(AVPacket *pkt, AVFormatContext *o_ps, AVFormatContext *i_ps)
{
	//进行PTS,DTS转换,因为从input读到的pts,dts，output端在加入header后，time_base改变.
	AVRational inttime = i_ps->streams[pkt->stream_index]->time_base;
	AVRational outtime = o_ps->streams[pkt->stream_index]->time_base;
	pkt->pts = av_rescale_q_rnd(pkt->pts, inttime, outtime, (AVRounding)AV_ROUND_NEAR_INF);
	pkt->dts = av_rescale_q_rnd(pkt->pts, inttime, outtime, (AVRounding)AV_ROUND_NEAR_INF);
	pkt->duration = av_rescale_q_rnd(pkt->pts, inttime, outtime, (AVRounding)AV_ROUND_NEAR_INF);
	pkt->pos = -1;
}

int ffmpg::speed_contrl(AVPacket *pkt,  AVFormatContext *o_ps, AVFormatContext *i_ps)
{
	//视频帧推送速度控制,【不控制流的话，vlc直接播放不了或者播放卡住】
	//方式1: if (pkt.stream_index == 0)//视频
	//方式2:if(o_ps->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)//视频
	if(o_ps->streams[pkt->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{  
		//老的方式:av_usleep(30 * 1000); //弊端:播放出的视频会卡顿或者花屏,音频也偶尔出现卡顿.
		//新的方式:精确 计算时间pts:	//音视频同步,无卡顿.
		AVRational dt = i_ps->streams[pkt->stream_index]->time_base;//时间基数,表示1秒有多少个量
		long long now = av_gettime() - StartTime;//2个视频帧的时间差
		long long dts = pkt->dts * 1000 * 1000 * r2d(dt);//视频帧的解码时间
		if (dts > now)
			av_usleep(dts - now);		
	}
	
	return 0;
}

int ffmpg::closeInput(AVFormatContext *i_ps)
{
    if(i_ps != NULL)
    {
        avformat_close_input(&i_ps);
    }
	return 0;
}

int ffmpg::closeoutput(AVFormatContext *o_ps)
{
    if(o_ps != NULL)
    {
        for( unsigned int i = 0 ; i < o_ps->nb_streams; i++)
        {
            AVCodecContext *codecContext = o_ps->streams[i]->codec;
            avcodec_close(codecContext);
        }
        avformat_close_input(&o_ps);
    }
	return 0;
}

void ffmpg::destroy_in_out(AVFormatContext &i_ps, AVFormatContext &o_ps)
{
	closeInput(&i_ps);
	closeoutput(&o_ps);
}

//++++-----for save stream---------++++
int ffmpg::save_realstream(AVFormatContext &o_ps, AVFormatContext &i_ps)
{
	AVPacket pkt;
	int ret = 0;
	
	while(1)
	{
		memset(&pkt, 0, sizeof(AVPacket));
		av_init_packet(&pkt);
		i_ps->interrupt_callback.opaque = av_gettime();//函数参数.
		ret = av_read_frame(&i_ps, &pkt);
		if(ret > 0)
		{
			//写之前，进行输入,输出时间戳的转换[时间基准可能不一样].
			AVStream *in = i_ps.streams[pkt.stream_index];
   			AVStream *out = o_ps.streams[pkt.stream_index];  
			av_packet_rescale_ts(&pkt, in->time_base, out->time_base);
			
			ret = av_interleaved_write_frame(&o_ps, &pkt); //交错写.
			PRT_COND(ret < 0, "Write pack failed .");

			cout<<"Write pack success ."<<endl;
		}
	}
	
	return 0;
}
