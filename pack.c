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
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "datapack.h"

#define CHUNK 16384
static unsigned char  in[CHUNK];
static unsigned char out[CHUNK];
static const char* program_name = NULL;
static const char* prefix = "";
static FILE* verbose = NULL;
static FILE* normal  = NULL;
static int file_given = 0; /* 1 if '-f' was given */
static int missing_fatal = 1;
static int log_level = 1;
static const char* struct_attrib = "";
static const char* data_attrib   = "__attribute__((section (\"datapack\")))";

static struct option options[] = {
	{"from-file", required_argument, 0, 'f'},
	{"from-dir",  required_argument, 0, 'r'},
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
	       "  -r, --from-dir=DIR      Use everything in directory.\n"
	       "  -o, --output=FILE       Write output to file instead of stdout.\n"
	       "  -d, --deps=FILE         Write optional Makefile dependency list.\n"
	       "  -e, --header=FILE       Write optional header-file.\n"
	       "  -p, --prefix=STRING     Prefix all targets with STRING.\n"
	       "  -s, --srcdir=DIR        Read all files from DIR instead of current directory.\n"
	       "  -b, --break             Break on missing files. [default]\n"
	       "  -i, --ignore            Ignore missing files.\n"
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
	struct entry * lnk; /* Pointer to a entry that this is a lnk to, or NULL */
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
	e->lnk = NULL;
	num_entries++;
}

static struct entry * find_entry(const char * path) {
	char buffer[PATH_MAX];
	for(int i = 0; i < num_entries; ++i) {
		char * ret = realpath(entries[i].src, buffer);
		if(ret != NULL && strcmp(buffer, path) == 0) {
			return entries + i;
		}
	}
	return NULL;
}

int parse_dir(const char * internal_path, const char * base_path) {

	char * path = NULL;
	if(asprintf(&path, "%s/%s", base_path, internal_path) == -1) {
		fprintf(verbose, "%s: asprintf returned -1\n", program_name);
		return 1;
	}


	DIR * dir = opendir(path);
	free(path);
	if ( !dir ){
		fprintf(verbose, "%s: failed to read directory `%s': %s.\n", program_name, optarg, strerror(errno));
		return 1;
	}

	char * line = NULL;

	struct dirent * entry = NULL;

	while( ( entry = readdir(dir)) != NULL) {
		if(entry->d_name[0] == '.') continue; //Ignore hidden files and .., .

		char * internal;
		char * var_name;

		if(asprintf(&internal, "%s/%s", internal_path, entry->d_name) == -1) {
			fprintf(verbose, "%s: asprintf returned -1\n", program_name);
			return 1;
		}

		var_name = strdup(internal);

		for(int i=0; i<strlen(var_name); ++i) {
			if(!isalnum(var_name[i]) && var_name[i] != '_') {
				var_name[i] = '_';
			}
		}

		switch(entry->d_type) {
			case DT_LNK:
			case DT_REG:
				{
					if(asprintf(&line, "%s:%s:%s", var_name, internal, internal) == -1) {
						fprintf(verbose, "%s: asprintf returned -1\n", program_name);
						return 1;
					}
					add_entry(line);
					free(line);
				}
				break;
			case DT_DIR:
				{
					if(parse_dir(internal, base_path)) {
						free(internal);
						free(var_name);
						return 1;
					}
				}
			break;
		}
		free(internal);
		free(var_name);
	}

	closedir(dir);
	return 0;
}

static void reopen_output(){
	fclose(verbose);
	fclose(normal);
	verbose = fopen(log_level >= 2 ? "/dev/stderr" : "/dev/null", "w");
	normal  = fopen(log_level >= 1 ? "/dev/stderr" : "/dev/null", "w");
}

int main(int argc, char* argv[]){
	/* extract program name from path. e.g. /path/to/MArCd -> MArCd */
	const char* separator = strrchr(argv[0], '/');
	if ( separator ){
		program_name = separator + 1;
	} else {
		program_name = argv[0];
	}

	const char* output = "/dev/stdout";
	const char* deps = NULL;
	const char* header = NULL;
	const char* srcdir = ".";

	/* initial output */
	verbose = fopen("/dev/null",   "w");
	normal  = fopen("/dev/stderr", "w");

	/* init entry table */
	max_entries = 256;
	entries = malloc(sizeof(void*)*max_entries);
	memset(entries, 0, sizeof(void*)*max_entries);

	int op, option_index;
	while ( (op=getopt_long(argc, argv, "r:f:o:d:e:p:s:vqhbi", options, &option_index)) != -1 ){
		switch ( op ){
		case 0:
			break;

		case '?':
			exit(1);

		case 'r':
			srcdir = optarg;
			if(parse_dir("", optarg)) {
				exit(-1);
			}
			break;
		case 'f':
		{
			FILE* fp = strcmp(optarg, "-") != 0 ? fopen(optarg, "r") : stdin;
			if ( !fp ){
				fprintf(normal, "%s: failed to read from `%s': %s.\n", program_name, optarg, strerror(errno));
				exit(1);
			}
			char* line = NULL;
			size_t bytes = 0;
			while ( getline(&line, &bytes, fp) != -1 ){
				add_entry(line);
			}
			if ( ferror(fp) ){
				fprintf(normal, "%s: failed to read from `%s': %s.\n", program_name, optarg, strerror(errno));
				exit(1);
			}
			free(line);
			fclose(fp);
		}
		file_given = 1;
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
			log_level = 2;
			reopen_output();
			break;

		case 'q':
			log_level = 0;
			reopen_output();
			break;

		case 'b': /* --break */
			missing_fatal = 1;
			break;

		case 'i': /* --ignore */
			missing_fatal = 0;
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

	FILE* dst = fopen(output, "w");
	if ( !dst ){
		fprintf(stderr, "%s: failed to open `%s' for writing: %s\n", program_name, output, strerror(errno));
		return 1;
	}

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

	/* bail out if there is no files to pack. If '-f' was given but it was empty
	 * it is probably what the user want so let it generate an empty pack. */
	if ( num_entries == 0 && file_given == 0 ){
		fprintf(stderr, "%s: no files given.\n", program_name);
		fprintf(normal, "usage: %s DATANAME:FILENAME..\n", program_name);
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
		if ( asprintf(&e->src, "%s/%s", srcdir, tmp) == -1 ){
			perror(program_name);
			return 1;
		}
		free(tmp);

		/* prepend prefix to dst path */
		tmp = e->dst;
		if ( asprintf(&e->dst, "%s%s", prefix, tmp) == -1 ){
			perror(program_name);
			return 1;
		}
		free(tmp);
	}

	/* output header */
	fprintf(dst, "#include \"datapack.h\"\n\n");

	/* output binary data */
	int files = 0;
	for ( struct entry* e = &entries[0]; e->src; e++ ){
		char * tmp;
		fprintf(verbose, "Processing %s from `%s' to `%s'\n", e->variable, e->src, e->dst);

		struct stat st;
		lstat(e->src, &st);

		if(S_ISLNK(st.st_mode)) {
			tmp = realpath(e->src, NULL);
			if(tmp == NULL) {
				if ( missing_fatal ){
					fprintf(stderr, "%s: failed to expand real path for lnk `%s': %s.\n", program_name, e->src, strerror(errno));
					fclose(dst);
					unlink(output);
					exit(1);
				}

				fprintf(normal, "%s: failed to expand real path for lnk `%s': %s. Ignored.", program_name, e->src, strerror(errno));
				free(e->dst);
				e->dst = NULL; /* mark as invalid */
				continue;
			}

			lstat(tmp, &st);

			if(S_ISDIR(st.st_mode)) {
				fprintf(normal, "%s: target for lnk `%s'->`%s' is a directory, ignored\n", program_name, e->src, tmp);
				free(e->dst);
				free(tmp);
				e->dst = NULL; /* mark as invalid */
				continue;
			}

			e->lnk = find_entry(tmp);
			if(e->lnk == NULL) {
				fprintf(normal, "%s: failed to read target `%s' for lnk `%s', ignored.\n", program_name, tmp, e->src);
				free(e->dst);
				free(tmp);
				e->dst = NULL; /* mark as invalid */
				continue;
			}
			free(tmp);
		} else {
			FILE* fp = fopen(e->src, "r");
			if ( !fp ){
				if ( missing_fatal ){
					fprintf(stderr, "%s: failed to read `%s'.\n", program_name, e->src);
					fclose(dst);
					unlink(output);
					exit(1);
				}

				fprintf(normal, "%s: failed to read `%s', ignored.\n", program_name, e->src);
				free(e->dst);
				e->dst = NULL; /* mark as invalid */
				continue;
			}

			fprintf(dst, "static const char %s_buf[] %s = \"", e->variable, data_attrib);

			ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
			if (ret != Z_OK)
				return ret;

			size_t bytes_r = 0;
			size_t bytes = 0;
			do {
				strm.avail_in = (unsigned int) fread(in, 1, CHUNK, fp);
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
		}

		files++;
	}
	fprintf(dst, "\n");

	/* output entries */
	for ( struct entry* e = &entries[0]; e->src; e++ ){
		if ( !e->dst ) continue;
		struct entry * real = e;
		if(e->lnk != NULL) real = e->lnk;
		fprintf(dst, "struct datapack_file_entry %s %s = {\"%s\", %s_buf, %zd, %zd};\n",
		        e->variable, struct_attrib, e->dst, real->variable, real->in, real->out);
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
				fprintf(fp, "\t%s %s\n", e->src, (e+1)->src ? "\\" : "");
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
