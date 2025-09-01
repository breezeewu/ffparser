#include "FFDemuxer.h"
#include "argparse.hpp"
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

int main(int argc, const char** argv)
{
	int parser_flag = 0;
	int dec_flag = 0;
    int write_flag = 0;
    int64_t begin = 0;
    int log_flag = 0;
    int64_t end = INT_MAX;
	std::string log_path;
    std::string input_url;

	argparse::ArgumentParser arg_parser("ffparser", "ffparser 1.0.0");
    arg_parser.add_argument("-i").help("input file path");
	arg_parser.add_argument("-p").help("printf packet timestamp of media packet, a:audio,v:video, s:subtitle, all:all of packet").default_value("all");
	arg_parser.add_argument("-w").help("write vide or audio data to ouput file");
	arg_parser.add_argument("-l").help("write printf log to output file");
	arg_parser.add_argument("-d").help("decoder packet and write the raw data to output file");
	arg_parser.add_argument("-b").help("begin timestamp with float type").scan<'f', float>();
	arg_parser.add_argument("-e").help("end timestamp with float type").scan<'f', float>();
    
	try {
        arg_parser.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << arg_parser;
        return 0;
    }

    if (arg_parser.is_used("-i")) {
        input_url = arg_parser.get<std::string>("-i");
    }
    if (arg_parser.is_used("-h")) {
        std::string usage = arg_parser.usage();
        std::cout << arg_parser.usage() << std::endl;
    }
	if (arg_parser.is_used("-p")) {
		parser_flag = parserFlag(arg_parser.get<std::string>("-p").c_str());
	}

	if (arg_parser.is_used("-d")) {
		dec_flag = parserFlag(arg_parser.get<std::string>("-p").c_str());
	}

	if (arg_parser.is_used("-w")) {
		write_flag = parserFlag(arg_parser.get<std::string>("-w").c_str());
	}

	if (arg_parser.is_used("-b")) {
		begin = (int64_t)(arg_parser.get<float>("-b") * 1000000);
	}

    if (arg_parser.is_used("-e")) {
		end = (int64_t)(arg_parser.get<float>("-e") * 1000000);
	}

    if (input_url.empty()) {
        printf("usage:%s\n", arg_parser.help().str().c_str());
    }
	FFDemuxer* ffdec = new FFDemuxer();
	/*const char* purl = "D:\\Downloads\\Frozen.2013.UHD.BluRay.2160p.10bit.HDR.4Audio.TrueHD(Atmos).7.1.DTS-HD.MA.7.1.x265-beAst.mkv";
	if (argc >= 2)
	{
		purl = argv[1];
	}

	
	/*int i = 2;
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
	}*/

	if (parser_flag <= 0 && write_flag <= 0 && dec_flag <= 0) {
		printf("%s\n", arg_parser.usage().c_str());
		return 0;
		//parser_flag = ENABLE_VIDEO | ENABLE_AUDIO | ENABLE_SUBTITLE;
	}

    if (!log_path.empty())
    {
        freopen(log_path.c_str(), "w", stdout);
    }

	ffdec->setDemuxerFlag(parser_flag, dec_flag, write_flag);
	int ret = ffdec->open(input_url.c_str());
	printf("ret:%d = ffdec->open(purl)\n", ret);

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