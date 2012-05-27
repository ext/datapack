Simple test of compressing and storing datafiles inside executable.

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
