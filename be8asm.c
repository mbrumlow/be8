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

struct instruction {
	char name[8];
	uint8_t control;
	bool args;
	uint8_t size;
};

char *token(int fd);
void compile(int fdin, int fdout, char *t, struct db_entry db[]);

static struct instruction instructions[] = {
	{ .name = "lda", .control = 0x1, .args = true, .size = 1  },
	{ .name = "add", .control = 0x2, .args = true, .size = 1  },
	{ .name = "sub", .control = 0x3, .args = true, .size = 1  },
	{ .name = "sta", .control = 0x4, .args = true, .size = 1  },
	{ .name = "out", .control = 0x5, .args = false, .size = 1  },
	{ .name = "jmp", .control = 0x6, .args = true, .size = 1  },
	{ .name = "ldi", .control = 0x7, .args = true, .size = 1  },
	{ .name = "jc",  .control = 0x8, .args = true, .size = 1  },
	{ .name = "hlt", .control = 0xf, .args = false, .size = 1  },
	{ .name = "db",  .control = 0x0, .args = false, .size = 1  },
	{ .name = "",    .control = 0x0, .args = false, .size = -1  }
};

int main(int argc, char *argv[]) {

	bool debug = false;
	char *in_file = NULL;
	char *out_file = NULL;
	struct db_entry db[MAX_VARS+1];


	memset(db, 0, sizeof(db));

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

		if( strcmp(t, "db") == 0)  {

			char *arg1 = token(fdin);
			char *arg2 = token(fdin);

			db[dbpos].name = arg1;
			db[dbpos].off = (uint8_t) off;

			printf("%s @ 0x%01x\n", arg1, off);
			free(arg2);

			dbpos++;
		}

		int i = 0;
		while(instructions[i].size > 0) {
			if(strcmp(t, instructions[i].name) == 0) {
				off += instructions[i].size;
				break;
			}
			i++;
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

int match_index(char *str) {

	int len = strlen(str);

	if(len > 0 && str[0] == '[' && str[len-1] == ']')
		return 1;

	return 0;

}

int match_db_name(char *db_name, char *name) {

	int min = strlen(db_name);
	int len = strlen(name);

	if( (len - 1) < min )
		min = len - 1;

	if( len > 2 && strncmp(name+1, db_name, min) == 0 ) {
		return 1;
	}

	return 0;
}

void compile(int fdin, int fdout, char *t, struct db_entry db[]) {

	uint8_t out = 0;
	uint8_t control = 0;
	uint8_t arg = 0;
	bool has_arg = false;
	bool found_arg = false;

	int i = 0;
	while(instructions[i].size > 0) {
		if(strcmp(t, instructions[i].name) == 0) {
			control = instructions[i].control;
			has_arg = instructions[i].args;
			break;
		}
		i++;
	}

	if(control) {

		if(has_arg) {
			char *arg_name = token(fdin);
			if(!arg_name) {
				fprintf(stderr, "expected argument!\n");
				_exit(-1);
			}

			if(match_index(arg_name)) {
				for(int i = 0; db[i].name != NULL; i++) {
					if(match_db_name(db[i].name,arg_name)) {
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


