#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#define HASHLENMID 	                   16
#define HASHLEN                        32
#define TILE_ROWS                      32
#define TILE_LENGTH (TILE_ROWS * HASHLEN)
#define TILE_TRANSFORMS                 8
#define MAP                       1048576
#define MAP_LENGTH    (TILE_LENGTH * MAP)
#define JUMP                            8

void cl_find_peach(uint32_t threads, uint8_t *g_map, int32_t *g_found, uint8_t *g_seed, uint8_t *c_input, uint8_t c_difficulty, uint32_t thread);
void cl_build_map(uint8_t *g_map, uint8_t *c_phash, uint32_t start_index, uint32_t thread);

uint32_t num_devices = 1;

typedef struct __trigg_opencl_ctx {
	uint8_t curr_seed[16], next_seed[16];
	char cp[256], *next_cp;
	int *found;
	uint8_t *d_map;
	uint8_t *d_phash;
	int32_t *d_found;
	uint8_t *seed;
	uint8_t *d_seed;
	uint32_t *midstate, *input;
	uint8_t *d_midstate256, *d_input32, *d_blockNumber8;
} TriggCLCTX;

TriggCLCTX ctx[64];

int trigg_init_cl(uint8_t  difficulty, uint8_t *blockNumber) {
	for (int i = 0; i < num_devices; i++) {
		ctx[i].d_map = (uint8_t*)malloc(MAP_LENGTH);
		if (ctx[i].d_map == NULL) {
			printf("Unable to malloc map\n");
		}
		memset(ctx[i].d_map, 0, MAP_LENGTH);

		ctx[i].d_phash = (uint8_t*)malloc(32);
		if (ctx[i].d_phash == NULL) {
			printf("Unable to malloc phash\n");
		}
		memset(ctx[i].d_phash, 0, 32);

#if 0
		printf("Running build_map\n");
		size_t build_map_work_size = 4096;
		for (int z = 0; z < 256; z++) {
			printf("build_map progress = %d / 256\n", z);
			uint32_t start_index = z*4096;
			for (int zz = 0; zz < build_map_work_size; zz++) {
				cl_build_map(ctx[i].d_map, ctx[i].d_phash, start_index, zz);
			}
		}
		printf("Build_map complete\n");
		uint8_t map[1024];
		memcpy(map, ctx[i].d_map, 1024);
		printf("map: ");
		for (int j = 0; j < 1024; j++) {
			printf("%02x ", map[j]);
		}
		printf("\n");

		uint8_t *full_map = (uint8_t*)malloc(MAP_LENGTH);
		memcpy(full_map, ctx[i].d_map, MAP_LENGTH);
		FILE *fp = fopen("map.dat", "wb");
		fwrite(full_map, 1, (size_t)MAP_LENGTH, fp);
		fclose(fp);
		free(full_map);
#else
		printf("Loading cached map\n");
		FILE *fp = fopen("map.dat", "rb");
		fread(ctx[i].d_map, 1, (size_t)MAP_LENGTH, fp);
		fclose(fp);
#endif

		ctx[i].d_found = (int32_t*)malloc(4);
		if (ctx[i].d_found == NULL) {
			printf("Unable to malloc d_found\n");
		}

		ctx[i].d_seed = (uint8_t*)malloc(16);
		if (ctx[i].d_seed == NULL) {
			printf("Unable to malloc d_seed\n");
		}

		ctx[i].d_midstate256 = (uint8_t*)malloc(32);
		if (ctx[i].d_midstate256 == NULL) {
			printf("Unable to malloc d_midstate256\n");
		}

		ctx[i].d_input32 = (uint8_t*)malloc(108);
		if (ctx[i].d_input32 == NULL) {
			printf("Unable to malloc d_input32\n");
		}

		ctx[i].d_blockNumber8 = (uint8_t*)malloc(8);
		if (ctx[i].d_blockNumber8 == NULL) {
			printf("Unable to malloc d_blockNumber8\n");
		}

		memcpy(ctx[i].d_blockNumber8, blockNumber, 8);

		/* Allocate associated device-host memory */
		ctx[i].found = (int*)malloc(4);
		ctx[i].seed = (uint8_t*)malloc(16);
		ctx[i].midstate = (uint32_t*)malloc(32);
		ctx[i].input = (uint32_t*)malloc(32);
		/* Set remaining device memory */
		memset(ctx[i].d_found, 0, 4);
		memset(ctx[i].d_seed, 0, 16);
		memset(ctx[i].d_input32, 0, 108);
		/* Setup variables for "first round" */
		*ctx[i].found = 0;
		printf("\nTrace: CPU %d Initialized.", i);
	}

	return num_devices;
}

int main() {
	uint64_t blockNumber = 1;
	uint8_t *bnum = (uint8_t*)&blockNumber;
	uint8_t diff = 18;
	trigg_init_cl(diff, bnum);
	struct timeval t_start, t_end;

	uint32_t threads = 512*256;
	for (uint32_t i = 0; i < num_devices; i++) {
            	gettimeofday(&t_start, NULL);
		uint32_t max_threads = 131072;
		/*for (int t = 0; t < threads; t++) {
			if (t & 0xff == 0) {
				printf("peach progress: %d / %d\n", t, threads);
			}
			cl_find_peach(max_threads, ctx[i].d_map, ctx[i].d_found, ctx[i].d_seed, ctx[i].d_input32, diff, t);
		}*/
		cl_find_peach(max_threads, ctx[i].d_map, ctx[i].d_found, ctx[i].d_seed, ctx[i].d_input32, diff, 122868);

		printf("Waiting for completion on CPU: %d\n", i);

		gettimeofday(&t_end, NULL);
		uint64_t ustart = 1000000 * t_start.tv_sec + t_start.tv_usec;
		uint64_t uend = 1000000 * t_end.tv_sec + t_end.tv_usec;
		double tdiff = (uend - ustart) / 1000.0 / 1000.0;
		printf("Diff: %f seconds\n", tdiff);
		printf("Hashrate: %f\n", threads / tdiff);

		memcpy(ctx[i].found, ctx[i].d_found, 4);
		printf("Found?: %d\n", *ctx[i].found);

		memcpy(ctx[i].seed, ctx[i].d_seed, 16);
		printf("Seed: ");
		for (int j = 0; j < 16; j++) {
			printf("%02x ", ctx[i].seed[j]);
		}
		printf("\n");

	}

	for (uint32_t i = 0; i < num_devices; i++) {
		free(ctx[i].d_map);
		free(ctx[i].d_phash);
	}
}

