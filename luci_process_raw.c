#include "luci.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

static size_t process_game_start(uint8_t *p, game_start_t *gamestartp);
static size_t process_event(uint8_t *p);
typedef enum { EVENT_PAYLOADS = 0x35, EVENT_GAME_START=0x36 } event_t;

void process_raw_data(void *ptr, size_t len)
{
	uint8_t *p = (uint8_t*)ptr;
	uint8_t *currentp = p;
	size_t offset = 0;
	event_t type;

	// DBG(printf("Object offset=0x%lx : %c\n", offset, type););

	element_t *elemlistp = NULL;

	// offset ++;
	do {
		type = p[offset];
		currentp = p + offset;

		if (offset == len) {
			DBG(printf("end of object\n"););
			// offset ++;
			break;
		}

		size_t event_size;

		switch(type) {
			case EVENT_PAYLOADS:;
				event_size = process_event(currentp); // should be first object, then never encountered again
				break;
			case EVENT_GAME_START:;
				game_start_t *gamestartp = malloc(sizeof(game_start_t));
				event_size = process_game_start(currentp, gamestartp);
				break;
			default:;
				event_size = 1;
				break;
		}

		if (event_size <= 0) goto fail;

		offset += event_size;
	} while (true);

	DBG(fflush(stdout););
	// return (elemlistp);

	fail:;
		DBG(printf("raw operation failed\n"););
		free_elements(elemlistp);
		// return (NULL);
}

static size_t process_event(uint8_t *p)
{
	return (p[1]+1);
}



typedef struct __attribute__((__packed__)) INFOBLOCK  {
	uint8_t event_type;	// offset 0
	uint8_t _pad0[4];
	uint8_t	bitfield_1;	// offset 5
	uint8_t	bitfield_2;	// 6
	uint8_t bitfield_3;	// 7
	uint8_t _pad1[4];
	uint8_t isteams;	// 0xD
	uint8_t _pad2[0x12F];
	uint32_t random_seed;	// offset 0x13D
	...
} infoblock_t;



static size_t process_game_start(uint8_t *p, game_start_t *gamestartp)
{

	if (p[0] != EVENT_GAME_START) {
		return 0;
	}

	game_info_block_t *gameinfoblockp = malloc(sizeof(game_info_block_t));
	gamestartp->game_info_block = gameinfoblockp;

	gameinfoblockp->game_bitfield_1 = p[0x5]
	gameinfoblockp->game_bitfield_2 = p[0x6]
	gameinfoblockp->game_bitfield_3 = p[0x8]

	gameinfoblockp->is_teams = (bool_t)p[0xD];



	// new code


	infoblock_t *ibp = (infoblock_t *)p;
	if (ibp->event_type != EVENT_GAME_START) return (0);

	game_info_block_t *gameinfoblockp = (game_info_block_t*)malloc(sizeof(game_info_block_t));
	if (gameinfoblockp == NULL) {
		fprintf(stderr,"Failed allocating memory\n");
		return (0);
	}
	gamestartp->game_info_block = gameinfoblockp;

	gameinfoblockp->game_bitfield_1 = ibp->bitfield_1;
	gameinfoblockp->game_bitfield_2 = ibp->bitfield_2;
	gameinfoblockp->game_bitfield_3 = ibp->bitfield_3;

	gameinfoblockp->is_teams = (bool_t)ibp->is_teams;

	gamestartp->random_seed = (uint32_t)ntohl(ibp->random_seed);


	printf("Offset for random_seed = 0x%p\n", &(((infoblock_t*)0)->random_seed));

	// new code end




	// int8_t *item_freq = &(p[0x10]);
	// int8_t *sd_value = &(p[0x11]);

	gameinfoblockp->item_freq = *(int8_t *)&(p[0x10]);
	gameinfoblockp->sd_value = *(int8_t *)&(p[0x11]);

	game_start_port_t *gamestartports[4];
	game_info_block_port_t *gameinfoblockports[4];

	memcpy(gamestartp->version, &(p[1]), VERSION_LENGTH); //make sure all values p[1]-p[4] are counted

	gamestartp->random_seed = (uint32_t)ntohl(*(uint32_t *)&(p[0x13D]));

	gamestartp->pal = (bool_t)p[0x1A1];
	gamestartp->frozen_ps = (bool_t)p[0x1A2];

	return 0x1a2;
}
