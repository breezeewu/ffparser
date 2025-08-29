#include "FFDemuxer.h"
int parserFlag(const char* str)
{
	int enable_flag = 0;
	for (int j = 0; j < strlen(str); j++)
	{
        if (0 == strcmp(str + j, "all")) {
            enable_flag |= ENABLE_VIDEO;
            enable_flag |= ENABLE_AUDIO;
            enable_flag |= ENABLE_SUBTITLE;
        }
		if (str[j] == 'v')
		{
			enable_flag |= ENABLE_VIDEO;
		}
		else if (str[j] == 'a')
		{
			enable_flag |= ENABLE_AUDIO;
		}
		else if (str[j] == 's')
		{
			enable_flag |= ENABLE_SUBTITLE;
		}
	}

	return enable_flag;
}

int get_height(int w, int h, int tow)
{
    w = w ? w : 1;
    int ret = tow * h / w;
    ret &= ~7;

    int a = ret % 4;
    if (a == 1)
    {
        ret -= 1;
    }
    if (a == 2)
    {
        ret += 2;
    }
    if (a == 3)
    {
        ret += 1;
    }
    return ret;
}

int main(int argc, const char** argv)
{
	//freopen("log.txt", "w", stdout);
	FFDemuxer* ffdec = new FFDemuxer();//std::make_shared<FFDemuxer>();
	//const char* purl = "C:\\Users\\Administrator\\Desktop\\1\\5151695557554952695754505254506648526950525466685670574956567051.mkv_1920x800-1.ts";
	//const char* purl = "D:\\Downloads\\Jade.No.Poverty.Land.E09.1080i.HDTV.H264-NGB.ts";
	const char* purl = "D:\\Downloads\\Frozen.2013.UHD.BluRay.2160p.10bit.HDR.4Audio.TrueHD(Atmos).7.1.DTS-HD.MA.7.1.x265-beAst.mkv";
	//const char* purl = "D:\\Downloads\\kongfupanna4.mkv";
	//const char* purl = "C:\\Users\\Administrator\\Desktop\\1\\1920x804_6000000_00000.ts";//5151695557554952695754505254506648526950525466685670574956567051.mkv_1920x800-1.ts";// 
	//const char* purl = "D:\\code\\debug\\mediaserver\\build\\src\\server_app\\1920x1080_5000000_01563.ts";
	//const char* purl = "D:\\code\\debug\\mediaserver\\build\\src\\server_app\\6549526850574865546657666553526855696557694853495253685368495653.mkv_1920x1080-1563.ts";
	//const char* purl = "D:\\Downloads\\9_(AVC_M@L4.1_722x406_2021K.AAC_128_44K_2ch)The_Police_Certifiable_2010.mov";
	//const char* purl = "D:\\Downloads\\h264_1920x816_7555462_1_none_00004.ts";
	//const char* purl = "D:\\Downloads\\8K_HDR_Weathering_With_You.mp4";
	if (argc >= 2)
	{
		purl = argv[1];
	}

    get_height(1920, 1010, 1920);
	int parser_flag = 0;
	int dec_flag = 0;
    int write_flag = 0;
    int64_t begin = 0;
    int log_flag = 0;
    int64_t end = INT_MAX;
	int i = 2;
	for(int i = 2; i < argc; i++)
	{
        if (strcmp(argv[i], "-l") == 0)
        {
            log_flag = 1;
        }
		else if (strcmp(argv[i], "-p") == 0)
		{
			i++;
			parser_flag = parserFlag(argv[i]);
		}
		else if (strcmp(argv[i], "-w") == 0)
		{
			i++;
            write_flag = parserFlag(argv[i]);
		}
        else if (strcmp(argv[i], "-d") == 0)
        {
            i++;
            dec_flag = parserFlag(argv[i]);
        }
        else if (strcmp(argv[i], "-b") == 0) {
            i++;
            begin = atof(argv[i])*1000000;
        } else if (strcmp(argv[i], "-e") == 0) {
            i++;
            end = atof(argv[i])*1000000;
        }
	}

	if (parser_flag <= 0 && write_flag <= 0 && dec_flag <= 0) {
		parser_flag = ENABLE_VIDEO | ENABLE_AUDIO | ENABLE_SUBTITLE;
	}

    if (log_flag)
    {
        freopen("output.txt", "w", stdout);
    }
	//const char* purl = "D:\\media\\video\\Dunkirk.2017.IMAX.2160p.UHD.BluRay.REMUX.DV.HDR10Plus.HEVC.TrueHD.7.1.96K-WiLDCAT.mkv";
	ffdec->setDemuxerFlag(parser_flag, dec_flag, write_flag);
	int ret = ffdec->open(purl);
	printf("ret:%d = ffdec->open(purl)\n", ret);
	//return 0;
	/*int64_t duration = ffdec->parserDuration(AVMEDIA_TYPE_AUDIO);
	if (duration <= 0)
	{
		duration = ffdec->parserDuration(AVMEDIA_TYPE_VIDEO);
	}*/
	ffdec->seek(begin);

	ffdec->start(end);
	int num = 0;
	while (ffdec->isAlive())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (num++ == 10)
		{
			//break;
		}
	}

	return 0;
}