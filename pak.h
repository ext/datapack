#ifndef DATAPACK_PAK_H
#define DATAPACK_PAK_H

#define DATAPACK_MAGIC {'D', 'A', 'T', 'A', 'P', 'A', 'C', 'K'}

/**
 * File entry for binary formats.
 */
struct datapack_pakfile_entry {
	uint32_t csize;            /* compressed size */
	uint32_t usize;            /* uncompressed size */
	uint32_t fsize;            /* length of filename */
	char filename[0];          /* filename */
} __attribute__((packed));

/**
 * File header for binary formats.
 */
struct datapack_pak_header {
	uint8_t dp_version;        /* pak-version */
	uint16_t dp_offset;        /* offset to file entry table */
	uint16_t dp_num_entries;   /* number of entries */
} __attribute__((packed));

#endif /* DATAPACK_PAK_H */
