#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <getopt.h>
#include <libgen.h>

#define VERSION "2"
#define CHUNK 16384
static unsigned char  in[CHUNK];
static unsigned char out[CHUNK];

static struct option options[] = {
	{"from-file", required_argument, 0, 'f'},
	{"output",    required_argument, 0, 'o'},
	{"verbose",   no_argument, 0, 'v'},
	{"quiet",     no_argument, 0, 'q'},
	{"help",      no_argument, 0, 'h'},
	{0,0,0,0} /* sentinel */
};

static void show_usage(){
	printf("pack-"VERSION"\n"
	       "(C) 2012 David Sveningsson <ext@sidvind.com>\n"
	       "Usage: pack [OPTIONS..] DATANAME:FILENAME..\n"
	       "  -f, --from-file=FILE    Read list from file.\n"
	       "  -o, --output=FILE       Write output to file instead of stdout.\n"
	       "  -v, --verbose           Enable verbose output.\n"
	       "  -q, --quiet             Quiet mode, only returning error code.\n"
	       "  -h, --help              This text.\n");
}

int main(int argc, char* argv[]){
	int level = 1;
	const char* output = "/dev/stdout";

	int op, option_index;
	while ( (op=getopt_long(argc, argv, "f:o:vqh", options, &option_index)) != -1 ){
		switch ( op ){
		case 0:
			break;

		case '?':
			exit(1);

		case 'f':
			break;

		case 'o':
			output = optarg;
			break;

		case 'v':
			level = 2;
			break;

		case 'q':
			level = 0;
			break;

		case 'h':
			show_usage();
			exit(0);
		}
	}

	FILE* verbose = fopen(level >= 2 ? "/dev/stderr" : "/dev/null", "w");
	FILE* normal  = fopen(level >= 1 ? "/dev/stderr" : "/dev/null", "w");
	FILE* dst = fopen(output, "w");

	int ret;
	z_stream strm;
	int flush;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	if ( optind == argc  ){
		fprintf(normal, "usage: c DATANAME:FILENAME..\n");
		return 1;
	}

	fprintf(dst, "#include \"datapack.h\"\n");

	int files = 0;
	for ( int set = optind; set < argc; set++ ){
		char* dataname = argv[set];
		char* delim = strchr(dataname, ':');
		if ( !delim ){
			fprintf(normal, "%s: missing delimiter in `%s', ignored.\n", argv[0], dataname);
			continue;
		}
		*delim = 0;
		char* filename = delim+1;
		char* base = basename(filename);
		fprintf(verbose, "Processing %s from `%s'\n", dataname, filename);

		FILE* fp = fopen(filename, "r");
		if ( !fp ){
			fprintf(normal, "%s: failed to read `%s', ignored.\n", argv[0], filename);
			continue;
		}

		fprintf(dst, "static const char %s_buf[] = \"", dataname);

		ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
		if (ret != Z_OK)
			return ret;

		size_t input = 0;
		size_t bytes = 0;
		do {
			strm.avail_in = fread(in, 1, CHUNK, fp);
			input += strm.avail_in;
			if (ferror(fp)) {
				deflateEnd(&strm);
				return Z_ERRNO;
			}
			flush = feof(fp) ? Z_FINISH : Z_NO_FLUSH;
			strm.next_in = in;

			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				deflate(&strm, flush);

				unsigned int have = CHUNK - strm.avail_out;
				for ( int i = 0; i < have; i++ ){
					fprintf(dst, "\\x%02X", out[i]);
					bytes++;
				}

			} while (strm.avail_out == 0);
		} while (flush != Z_FINISH);

		fprintf(dst, "\";\n");
		fprintf(dst, "struct datapack_file_entry %s = {\"%.63s\", %s_buf, %zd, %zd};\n", dataname, base, dataname, bytes, input);

		deflateEnd(&strm);
		files++;
	}

	fprintf(verbose, "%d datafiles processed\n", files);

	fclose(verbose);
	fclose(normal);

	return Z_OK;
}
