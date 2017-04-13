/*
Copyright (c) 2017, Michael Brumlow <mbrumlow@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sys/types.h>
#include <sys/stat.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#define MAX_VARS (16)

struct db_entry {
	char *name;
	uint8_t off;
};

char *token(int fd);
void compile(int fdin, int fdout, char *t, struct db_entry db[]);

int main(int argc, char *argv[]) {

	bool debug = false;
	char *in_file = NULL;
	char *out_file = NULL;
	struct db_entry db[MAX_VARS];

	for(int  optind = 1; optind < argc; optind++) {
		if( argv[optind][0] == '-' ) {
			switch( argv[optind][1] ) {
				case 'd': debug = true; break;
				case 'o':
					if( optind + 1 < argc ) {
						out_file = strdup(argv[++optind]);
					}
					break;
			}
		} else if (in_file) {
			fprintf(stderr, "%s: to many input files\n", argv[0]);
			return -1;
		} else {
			in_file = strdup(argv[optind]);
		}
	}

	if(!in_file) {
		fprintf(stderr, "%s: no input file\n", argv[0]);
		return -1;
	}

	if(!out_file) {
		fprintf(stderr, "%s: no output file\n", argv[0]);
		return -1;
	}

	int fdin = open(in_file, O_RDONLY);
	if(fdin <= 0) {
		fprintf(stderr, "%s: unable to open input!\n", argv[0]);
		return -1;
	}

	int fdout = open(out_file, O_WRONLY|O_CREAT|O_TRUNC, 00600);
	if(fdout <= 0) {
		fprintf(stderr, "%s: unable to open output!\n", argv[0]);
		return -1;
	}

	// Pass one -- load db vars.
	int off = 0;
	int dbpos = 0;
	while(1) {
		char *t = token(fdin);
		if(!t) break;

		if( strcmp(t, "lda") == 0)
			off++;

		if( strcmp(t, "add") == 0)
			off++;

		if( strcmp(t, "out") == 0)
			off++;

		if( strcmp(t, "sta") == 0)
			off++;

		if( strcmp(t, "jmp") == 0)
			off++;

		if( strcmp(t, "jc") == 0)
			off++;

		if( strcmp(t, "ldi") == 0)
			off++;

		if( strcmp(t, "hlt") == 0)
			off++;

		if( strcmp(t, "db") == 0)  {

			char *arg1 = token(fdin);
			char *arg2 = token(fdin);

			db[dbpos].name = arg1;
			db[dbpos].off = (uint8_t) off;

			printf("%s @ 0x%01x\n", arg1, off);

			free(arg2);

			off++;
			dbpos++;
		}

		free(t);
	}

	lseek(fdin, SEEK_SET, 0);

	// Pass two - compile.
	while(1) {
		char *t = token(fdin);
		if(!t) break;
		printf("T: %s\n", t);
		compile(fdin, fdout, t, db);
		free(t);
	}

	close(fdin);
	close(fdout);
}

void compile(int fdin, int fdout, char *t, struct db_entry db[]) {

	uint8_t out = 0;
	uint8_t control = 0;
	uint8_t arg = 0;
	bool has_arg = false;
	bool found_arg = false;

	if(strcmp(t, "lda") == 0) {
		control = 0x1;
		has_arg = true;
	}

	if(strcmp(t, "add") == 0) {
		control = 0x2;
		has_arg = true;
	}

	if(strcmp(t, "sta") == 0) {
		control = 0x4;
		has_arg = true;
	}

	if(strcmp(t, "jmp") == 0) {
		control = 0x6;
		has_arg = true;
	}

	if(strcmp(t, "ldi") == 0) {
		control = 0x7;
		has_arg = true;
	}

	if(strcmp(t, "jc") == 0) {
		control = 0x8;
		has_arg = true;
	}

	if(control) {
		if(has_arg) {
			char *arg_name = token(fdin);
			if(!arg_name) {
				fprintf(stderr, "expected argument!\n");
				_exit(-1);
			}

			if(arg_name[0] == '[' ) {
				for(int i = 0; i < MAX_VARS; i++) {
					if(strcmp(db[i].name,arg_name) == 0 ) {
						arg = db[i].off;
						found_arg = true;
						break;
					}
				}

				if(!found_arg) {
					fprintf(stderr, "lda failed to find db variable.!\n");
					_exit(-1);
				}
			} else {
				arg = (uint8_t) strtol(arg_name, NULL, 16);
			}
		}

		out = control << 4 | arg;
		goto write_out;
	}

	if(strcmp(t, "db") == 0) {

		char *arg1 = token(fdin);
		if(!arg1) {
			fprintf(stderr, "db Expected argument!\n");
			_exit(-1);
		}

		char *arg2 = token(fdin);
		if(!arg2) {
			fprintf(stderr, "db Expected argument!\n");
			_exit(-1);
		}

		printf("-- db %s %s\n", arg1, arg2);

		out = (uint8_t) atoi(arg2);

		free(arg1);
		free(arg2);
		goto write_out;
	}

	if(strcmp(t, "out") == 0) {
		printf("-- out\n");
		out = 0x5 << 4;
		goto write_out;
	}

	if(strcmp(t, "hlt") == 0) {
		printf("-- hlt\n");
		out = 0xf << 4;
		goto write_out;
	}

write_out:

	write(fdout, &out, sizeof(out));

	return;
}

char *token(int fd) {

	char *tok = malloc(16);
	if(!tok) {
		fprintf(stderr, "Failed to allocate token.\n");
		_exit(-1);
	}
	memset(tok, 0, 16);

	char *t = tok;
	while( read(fd, t, sizeof(char)) != 0 ) {

		if( *t == ' ' || *t == '\n' || *t == '\0' ) {
			if( t != tok)  {
				*t = '\0';
				break;
			}
		} else {
			t++;
		}
	}

	if(t == tok) {
		free(tok);
		return NULL;
	}

	return tok;
}


