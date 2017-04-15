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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#define RAM_SIZE (16)

#define DEBUGF(fmt, ...) \
	do { if(debug) fprintf(stderr, fmt, __VA_ARGS__); } while(0)

int main(int argc, char *argv[]) {

	uint8_t x = 0;
	uint8_t c = 0;
	uint8_t pc = 0;
	uint8_t ir = 0;
	uint8_t mar = 0;
	uint8_t ra = 0;
	uint8_t rb = 0;
	uint8_t *ram = NULL;

	bool debug = false;
	char *file = NULL;

	for( int optind = 1; optind < argc; optind++) {
		if( argv[optind][0] == '-' ) {
			switch( argv[optind][1] ) {
				case 'd': debug = true; break;
			}
		} else if (file) {
			fprintf(stderr, "%s: to many input files\n", argv[0]);
			return -1;
		} else {
			file = strdup(argv[optind]);
		}
	}

	if(!file) {
		fprintf(stderr, "%s: no input file\n", argv[0]);
		return -1;
	}

	ram = (uint8_t *) malloc(RAM_SIZE);
	if(!ram) {
		fprintf(stderr, "%s: Failed to allocate %d bytes for ram.\n", argv[0], RAM_SIZE);
		return -1;
	}

	// Load file into ram.
	int fd = open(file, O_RDONLY);
	if(fd <= 0) {
		fprintf(stderr, "%s: Failed to open program!\n", argv[0]);
		return -1;
	}

	while( read(fd, &x, sizeof(x)) != 0 && mar < RAM_SIZE) {
		ram[mar++] = x;
	}
	mar = 0;

	close(fd);
	free(file);

	while(1) {

		// T0 - PC out MAR in
		mar = pc;

		// T1 - RAM out IR in
		ir = ram[mar];

		// T2 - PC++
		pc++;

		// T3 -
		switch(ir>>4) {

			case 0x1: // lda
				DEBUGF("0x%01x: lda [0x%01x]\n", pc-1, ir & 0xF);
				mar = ir & 0xF;
				ra = ram[mar];
				break;

			case 0x2: // add
				DEBUGF("0x%01x: add [0x%01x]\n", pc-1, ir & 0xF);
				mar = ir & 0xF;
				rb = ram[mar];

				// Check if this is going to carry - better way to do this?
				if(rb > 0xFF - ra)
					c = 1;
				else
					c = 0;

				ra += rb;
				break;

			case 0x3: // sub
				DEBUGF("0x%01x: sub [0x%01x]\n", pc-1, ir & 0xF);
				mar = ir & 0xF;
				rb = ram[mar];
				ra -= rb;
				break;

			case 0x4: // sta
				DEBUGF("0x%01x: sta [0x%01x]\n", pc-1, ir & 0xF);
				ram[ir & 0xF] = ra;
				break;

			case 0x5: // out
				printf("%d\n", ra);
				break;

			case 0x6: // jmp
				DEBUGF("0x%01x: jmp 0x%01x\n", pc-1, ir & 0xF);
				pc = ir & 0xF;
				break;

			case 0x7: // ldi
				DEBUGF("0x%01x: ldi 0x%01x\n", pc-1, ir & 0xF);
				ra = ir & 0xF;
				break;

			case 0x8: // jc
				DEBUGF("0x%01x: jc 0x%01x\n", pc-1, ir & 0xF);
				if(c)
					pc = ir & 0xF;
				break;

			case 0xF: // hlt
				return 0;

			default:
				fprintf(stderr, "%s: unknown instruction 0x%01x at 0x%01x\n", argv[0], ir >> 4, pc-1);
				return -1;
		}
	}

	free(ram);
}


