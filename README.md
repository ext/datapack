Simple test of compressing and storing datafiles inside executable.

# Features

* Packs datafiles directly into executable.
* Compression using zlib.
* Allows users to override files (must be explicitly enabled)
* API to access files in-memory (entire file is loaded into memory)
* Supports FILE* for reading/writing (data is streamed).

# Usage

1. Create ando/or edit datafiles
2. `datapacker NAME:FILENAME..` to generate c-file.
3. Build your app as usual and include generated c-file.
4. Use `unpack` or `unpack_filename` to load the data into memory.
   `unpack` uses structure directly.
   `unpack_filename` uses a virtual filename to locate the struct.

# Install

1. ./configure
2. make
3. make install

# Using with automake

See Makefile.am for real example.

    files.c: datapacker Makefile
    	$(AM_V_GEN)./datapacker -f ${top_srcdir}/datafiles -s ${top_srcdir} -d $(DEPDIR)/files.data -o files.c -e files.h
    
    -include ./$(DEPDIR)/files.data

where `datafiles` is a list of files to pack.
