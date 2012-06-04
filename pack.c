#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <getopt.h>
#include <libgen.h>
#include <errno.h>
#include <ctype.h>
#include "datapack.h"

#define CHUNK 16384
static unsigned char  in[CHUNK];
static unsigned char out[CHUNK];
static const char* program_name = NULL;
static const char* prefix = "";
static FILE* verbose = NULL;
static FILE* normal  = NULL;

static struct option options[] = {
	{"from-file", required_argument, 0, 'f'},
	{"output",    required_argument, 0, 'o'},
	{"deps",      required_argument, 0, 'd'},
	{"header",    required_argument, 0, 'e'},
	{"prefix",    required_argument, 0, 'p'},
	{"srcdir",    required_argument, 0, 's'},
	{"verbose",   no_argument, 0, 'v'},
	{"quiet",     no_argument, 0, 'q'},
	{"help",      no_argument, 0, 'h'},
	{0,0,0,0} /* sentinel */
};

static void show_usage(){
	printf("%s-"VERSION"\n"
	       "(C) 2012 David Sveningsson <ext@sidvind.com>\n"
	       "Usage: %s [OPTIONS..] DATANAME:FILENAME[:TARGET]..\n"
	       "where: DATANAME is the variable name,\n"
	       "       FILENAME is the source filename,\n"
	       "       TARGET is the filename as it appears in binary (default is basename)\n"
	       "\n"
	       "Options:\n"
	       "  -f, --from-file=FILE    Read list from file (same format, one entry per line).\n"
	       "  -o, --output=FILE       Write output to file instead of stdout.\n"
	       "  -d, --deps=FILE         Write optional Makefile dependency list.\n"
	       "  -e, --header=FILE       Write optional header-file.\n"
	       "  -p, --prefix=STRING     Prefix all targets with STRING.\n"
	       "  -s, --srcdir=DIR        Read all files from DIR instead of current directory.\n"
	       "  -v, --verbose           Enable verbose output.\n"
	       "  -q, --quiet             Quiet mode, only returning error code.\n"
	       "  -h, --help              This text.\n", program_name, program_name);
}

struct entry {
	char variable[64];
	char* dst;
	char* src;
	size_t in;
	size_t out;
};

static size_t num_entries = 0;
static size_t max_entries = 0;
static struct entry* entries = NULL;

static void add_entry(char* str){
	if ( num_entries+1 == max_entries ){
		max_entries += 256;
		entries = realloc(entries, sizeof(void*)*max_entries);
		memset(entries+num_entries, 0, sizeof(void*)*(max_entries-num_entries));
	}

	/* remove trailing newline */
	const size_t len = strlen(str);
	if ( str[len-1] == '\n' ){
		str[len-1] = 0;
	}

	char* vname = str;

	/* locate filename */
	char* delim = strchr(vname, ':');
	if ( !delim ){
		fprintf(normal, "%s: missing delimiter in `%s', ignored.\n", program_name, str);
		return;
	}
	*delim = 0;
	char* sname = delim+1;
	char* dname = basename(sname);

	/* locate rename */
	delim = sname;
	do {
		delim = strchr(delim, ':');
		if ( !delim || *(delim-1) != '\\' ) break;
	} while (1);
	if ( delim ){
		*delim = 0;
		dname = delim+1;
	}

	/* sanity check */
	if ( strlen(vname) >= 64 ){
		fprintf(normal, "%s: variable name `%s' too long (max: 63, current: %zd), ignored\n", program_name, vname, strlen(vname));
		return;
	}
	for ( int i = 0; i < strlen(vname); i++ ){
		if ( !(isalnum(vname[i]) || vname[i] == '_') ){
			fprintf(normal, "%s: variable name `%s' contains illegal characters, ignored\n", program_name, vname);
			return;
		}
	}

	/* store */
	struct entry* e = &entries[num_entries];
	sprintf(e->variable, "%.63s", vname);
	e->dst = strdup(dname);
	e->src = strdup(sname);
	e->in  = 0;
	e->out = 0;
	num_entries++;
}

int main(int argc, char* argv[]){
	/* extract program name from path. e.g. /path/to/MArCd -> MArCd */
	const char* separator = strrchr(argv[0], '/');
	if ( separator ){
		program_name = separator + 1;
	} else {
		program_name = argv[0];
	}

	int level = 1;
	const char* output = "/dev/stdout";
	const char* deps = NULL;
	const char* header = NULL;
	const char* srcdir = ".";

	/* init entry table */
	max_entries = 256;
	entries = malloc(sizeof(void*)*max_entries);
	memset(entries, 0, sizeof(void*)*max_entries);

	int op, option_index;
	while ( (op=getopt_long(argc, argv, "f:o:d:e:p:s:vqh", options, &option_index)) != -1 ){
		switch ( op ){
		case 0:
			break;

		case '?':
			exit(1);

		case 'f':
		{
			FILE* fp = strcmp(optarg, "-") != 0 ? fopen(optarg, "r") : stdin;
			if ( !fp ){
				if ( level >= 1 ) fprintf(stderr, "%s: failed to read from `%s': %s.\n", program_name, optarg, strerror(errno));
				exit(1);
			}
			char* line = NULL;
			size_t bytes = 0;
			while ( getline(&line, &bytes, fp) != -1 ){
				add_entry(line);
			}
			if ( ferror(fp) ){
				if ( level >= 1 ) fprintf(stderr, "%s: failed to read from `%s': %s.\n", program_name, optarg, strerror(errno));
				exit(1);
			}
			free(line);
			fclose(fp);
		}
		break;

		case 'o':
			output = optarg;
			break;

		case 'd':
			deps = optarg;
			break;

		case 'e':
			header = optarg;
			break;

		case 'p':
			prefix = optarg;
			break;

		case 's':
			srcdir = optarg;
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

	/* bail out if trying to write Makefile dependencies when no filename is given */
	if ( deps && strcmp(output, "/dev/stdout") == 0 ){
		fprintf(stderr, "%s: cannot write Makefile dependencies when writing output to stdout, must set filename with -o\n", program_name);
		return 1;
	}

	verbose = fopen(level >= 2 ? "/dev/stderr" : "/dev/null", "w");
	normal  = fopen(level >= 1 ? "/dev/stderr" : "/dev/null", "w");
	FILE* dst = fopen(output, "w");

	int ret;
	z_stream strm;
	int flush;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	/* read entries from arguments */
	for ( int i = optind; i < argc; i++ ){
		add_entry(argv[i]);
	}

	if ( num_entries == 0 ){
		fprintf(normal, "usage: c DATANAME:FILENAME..\n");
		return 1;
	}

	/* strip trailing slash from srcdir (will be appended later). Ensures
	 * consistency both with or without the trailing slash. */
	{
		size_t len = strlen(srcdir);
		if ( srcdir[len-1] == '/' ) len--;
		srcdir = strndup(srcdir, len);
	}

	/* prepend prefixes to both src and dst. (this is deferred as --prefix should apply to
	 * --from-file no matter what order the arguments are given in) */
	for ( struct entry* e = &entries[0]; e->src; e++ ){
		char* tmp;

		/* prepend srcdir to src path */
		tmp = e->src;
		asprintf(&e->src, "%s/%s", srcdir, tmp);
		free(tmp);

		/* prepend prefix to dst path */
		tmp = e->dst;
		asprintf(&e->dst, "%s%s", prefix, tmp);
		free(tmp);
	}

	/* output header */
	fprintf(dst, "#include \"datapack.h\"\n\n");

	/* output binary data */
	int files = 0;
	for ( struct entry* e = &entries[0]; e->src; e++ ){
		fprintf(verbose, "Processing %s from `%s' to `%s'\n", e->variable, e->src, e->dst);

		FILE* fp = fopen(e->src, "r");
		if ( !fp ){
			fprintf(normal, "%s: failed to read `%s', ignored.\n", program_name, e->src);
			free(e->dst);
			e->dst = NULL; /* mark as invalid */
			continue;
		}

		fprintf(dst, "static const char %s_buf[] = \"", e->variable);

		ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
		if (ret != Z_OK)
			return ret;

		size_t bytes_r = 0;
		size_t bytes = 0;
		do {
			strm.avail_in = fread(in, 1, CHUNK, fp);
			bytes_r += strm.avail_in;
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
		e->in = bytes;
		e->out = bytes_r;

		deflateEnd(&strm);
		fclose(fp);
		files++;
	}
	fprintf(dst, "\n");

	/* output entries */
	for ( struct entry* e = &entries[0]; e->src; e++ ){
		if ( !e->dst ) continue;
		fprintf(dst, "struct datapack_file_entry %s = {\"%s\", %s_buf, %zd, %zd};\n",
		        e->variable, e->dst, e->variable, e->in, e->out);
	};
	fprintf(dst, "\n");

	/* output Makefile dependencies */
	if ( deps ){
		fprintf(verbose, "%s: writing Makefile dependencies to `%s'\n", program_name, deps);
		FILE* fp = fopen(deps, "w");
		if ( !fp ){
			fprintf(normal, "%s: failed to write Makefile dependencies to `%s': %s\n", program_name, deps, strerror(errno));
		} else {
			fprintf(fp, "%s: \\\n", output);
			for ( struct entry* e = &entries[0]; e->src; e++ ){
				if ( !e->dst ) continue;
				fprintf(fp, "\t%s \\\n", e->src);
			}
			fprintf(fp, "\n");
			fclose(fp);
		}
	}

	/* output file header */
	if ( header ){
		fprintf(verbose, "%s: writing file header to `%s'\n", program_name, header);
		FILE* fp = fopen(header, "w");
		if ( !fp ){
			fprintf(normal, "%s: failed to write header to `%s': %s\n", program_name, header, strerror(errno));
		} else {
			fprintf(fp, "#ifndef DATAPACKER_FILES_H\n#define DATAPACKER_FILES_H\n\n#include \"datapack.h\"\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");
			for ( struct entry* e = &entries[0]; e->src; e++ ){
				if ( !e->dst ) continue;
				fprintf(fp, "extern struct datapack_file_entry %s;\n", e->variable);
			}
			fprintf(fp, "\n#ifdef __cplusplus\n}\n#endif\n\n#endif /* DATAPACKER_FILES_H */\n");
			fclose(fp);
		}
	}

	/* output file table and clear files */
	fprintf(dst, "struct datapack_file_entry* filetable[] = {\n");
	for ( struct entry* e = &entries[0]; e->src; e++ ){
		if ( e->dst ){
			fprintf(dst, "\t&%s,\n", e->variable);
		}
		free(e->dst);
		free(e->src);
		e->dst = NULL;
		e->src = NULL;
	}
	fprintf(dst, "\tNULL\n};\n\n");

	fprintf(verbose, "%d datafile(s) processed.\n", files);

	fclose(dst);
	fclose(verbose);
	fclose(normal);
	free((char*)srcdir);
	free(entries);
	entries = NULL;

	return Z_OK;
}
