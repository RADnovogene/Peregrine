#include <assert.h>
#include <errno.h>
#include <zlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "kseq.h"

KSEQ_INIT(gzFile, gzread);

extern char *optarg;
extern int optind, opterr, optopt;

int main(int argc, char *argv[])
{
	gzFile fp;
	FILE *seq_dataset_file;
	FILE *index_file;
	FILE *seqdb_file;
	kseq_t *seq;
	uint32_t rid;
	char *seq_dataset_path = NULL;
	char *seqdb_prefix = NULL;
	char index_fn[8192];
	char seqdb_fn[8192];
	char fn[8192];
	int l, c;

	opterr = 0;

	while ((c = getopt (argc, argv, "d:p:")) != -1)  {
		switch (c) {
			case 'd':
				seq_dataset_path = optarg;
				break;
			case 'p':
				seqdb_prefix = optarg;
				break;

			case '?':
				if (optopt == 'd') {
					fprintf(stderr, "Option -%c not specified, using 'seq_dataset.lst' as the input file\n", optopt);
				}
				else if (optopt == 'i') {
					fprintf(stderr, "Option -%c not specified, using 'seq_dataset.idx' as the output index path\n", optopt);
				} else {
					fprintf(stderr, "Usage: build_read_index -d seq_dataset.lst -i seq_dataset.idxi\n");
				}
				return 1;
			default:
				abort();
		}
	}

	if (seq_dataset_path == NULL) {
		seq_dataset_path = (char *) calloc(8192, 1);
		snprintf( seq_dataset_path, 8191, "seq_dataset.lst" );
	}

	if (seqdb_prefix == NULL) {
		seqdb_prefix = (char *) calloc(8192, 1);
		snprintf( seqdb_prefix, 8191, "seq_dataset" );
	}


	seq_dataset_file = fopen(seq_dataset_path, "r");
	printf("input sequence dataset file list: '%s'\n", seq_dataset_path);
	if (!seq_dataset_file) {
		fprintf(stderr, "file '%s' open error: %s\n", seq_dataset_path, strerror(errno));
		exit(1);
	}

	int written;
	written = snprintf(index_fn, sizeof(index_fn), "%s.idx", seqdb_prefix);
	assert(written < sizeof(index_fn));
	printf("output index file: %s\n", index_fn);
	index_file = fopen(index_fn, "w");  // use text file for now
	if (!index_file) {
		fprintf(stderr, "file '%s' open error: %s\n", index_fn, strerror(errno));
		exit(1);
	}

	written = snprintf(seqdb_fn, sizeof(index_fn), "%s.seqdb", seqdb_prefix);
	assert(written < sizeof(seqdb_fn));
	printf("output seqdb file: %s\n", index_fn);
	seqdb_file = fopen(seqdb_fn, "wb");  // use text file for now
	if (!index_file) {
		fprintf(stderr, "file '%s' open error: %s\n", index_fn, strerror(errno));
		exit(1);
	}

	rid = 0;
	size_t offset = 0;
	while( fscanf(seq_dataset_file, "%s", fn) != EOF ) {
		fp = gzopen(fn, "r");
		if (!fp) {
			fprintf(stderr, "file '%s' open error: %s\n", fn, strerror(errno));
			exit(1);
		}
		seq = kseq_init(fp);
		while ((l = kseq_read(seq)) >= 0) {
			fprintf(index_file, "%09d %s %u %lu\n", rid, seq->name.s, seq->seq.l, offset);
			fwrite(seq->seq.s, sizeof(char), seq->seq.l, seqdb_file);
			fwrite("\0", sizeof(char), 1, seqdb_file); //terminate the strings
			rid += 1;
			offset += seq->seq.l + 1;
		}
		kseq_destroy(seq);
		gzclose(fp);
	}
	fclose(seq_dataset_file);
	fclose(index_file);
	fclose(seqdb_file);
	if (!seq_dataset_path) free(seq_dataset_path);
	if (!seqdb_prefix) free(seqdb_prefix);
	return 0;
}
