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



typedef struct __attribute__((__packed__)) GSFULLBLOCK  {
	uint8_t event_type;	// offset 0x0
	uint8_t version[VERSION_LENGTH]; // offset 0x1
	uint8_t	game_bitfield_1;	// offset 0x5
	uint8_t	game_bitfield_2;	// 0x6
	uint8_t game_bitfield_3;	// 0x7
	uint8_t _pad1[4]; // 5 byte skip
	uint8_t isteams;	// 0xD
	uint8_t item_freq; // 0x10
	uint8_t sd_value; // 0x11
	uint8_t _pad2; // 1 byte skip
	uint16_t stage; // 0x13
	uint32_t timer_value; // 0x15
	uint8_t item_spawn_bitfield_1; // 0x29
	uint8_t item_spawn_bitfield_2;
	uint8_t item_spawn_bitfield_3;
	uint8_t item_spawn_bitfield_4;
	uint8_t item_spawn_bitfield_5; // 0x2C
	uint8_t _pad3[7]; // 8 byte skip
	float damage_ratio; // 0x35
	uint8_t _pad4[0x2C]; // 0x2C byte skip
	struct __attribute__((__packed__)) gib_port {
		uint8_t extern_char_id; // 0x65 + 0x24i
		uint8_t player_type; // 0x66 + 0x24i
		uint8_t stock_start_count; // 0x67 + 0x24i
		uint8_t costume_index; // 0x68 + 0x24i
		uint8_t _pad5[2]; // 3 byte skip
		uint8_t team_shade; // 0x6C + 0x24i
		uint8_t handicap; // 0x6D + 0x24i
		uint8_t team_id; // 0x6E + 0x24i
		uint8_t _pad6[1]; // 2 byte skip
		uint8_t player_bitfield; // 0x71 + 0x24i
		uint8_t _pad7[1]; // 2 byte skip
		uint8_t cpu_level; // 0x74 + 0x24i
		uint8_t _pad8[3]; // 4 byte skip
		float offense_ratio; // 0x79 + 0x24i
		float defense_ratio; // 0x7D + 0x24i
		float model_scale; // 0x81 + 0x24i
		uint8_t _pad9[3]; // need to get to 24 bytes within port struct
	}[PORT_COUNT]; // ends at 0x119
	uint8_t _pad9[0x24]; // uhhhhhh byte skip but maybe unneccesary?
	uint32_t random_seed;	// offset 0x13D
	struct __attribute__((__packed__)) gsb_port {
		uint32_t dashback_fix;	// offset 0x141 + 0x8i
		uint32_t shield_drop_fix;	// offset 0x145 + 0x8i
		uint32_t nametag[NAMETAG_LENGTH];	// offset 0x161 + 0x10i
	}[PORT_COUNT];
	uint8_t pal;
	uint8_t frozen_ps;
} gsfullblock_t;



static size_t process_game_start(uint8_t *p, game_start_t *gamestartp)
{

	gsfullblock_t *ibp = (gsfullblock_t *)p;
	if (ibp->event_type != EVENT_GAME_START) return (0);

	game_info_block_t *gameinfoblockp = (game_info_block_t*)malloc(sizeof(game_info_block_t));
	game_info_block_port_t *gameibportp = (game_info_block_port_t*)malloc(sizeof(game_info_block_port_t));

	game_start_port_t *gamestartportp[PORT_COUNT];
	game_info_block_port_t *gameibportp[PORT_COUNT];

	if (gameinfoblockp == NULL || gamestartportp == NULL || gameibportp == NULL) goto malloc_fail;

	gamestartp->game_info_block = gameinfoblockp;


	gamestartp->version = ibp->version;

	gameinfoblockp->game_bitfield_1 = ibp->game_bitfield_1;
	gameinfoblockp->game_bitfield_2 = ibp->game_bitfield_2;
	gameinfoblockp->game_bitfield_3 = ibp->game_bitfield_3;
	gameinfoblockp->is_teams = (bool_t)ibp->is_teams;
	gameinfoblockp->item_freq = ibp->item_freq;
	gameinfoblockp->sd_value = ibp->sd_value;
	gameinfoblockp->stage = (uint16_t)ntohs(ibp->stage);
	gameinfoblockp->timer_value = (uint32_t)ntohl(ibp->timer_value);
	gameinfoblockp->item_spawn_bitfield_1 = ibp->item_spawn_bitfield_1;
	gameinfoblockp->item_spawn_bitfield_2 = ibp->item_spawn_bitfield_2;
	gameinfoblockp->item_spawn_bitfield_3 = ibp->item_spawn_bitfield_3;
	gameinfoblockp->item_spawn_bitfield_4 = ibp->item_spawn_bitfield_4;
	gameinfoblockp->item_spawn_bitfield_5 = ibp->item_spawn_bitfield_5;

	for(int i = 0;i<PORT_COUNT;i++){
		gamestartportp[i] = (game_start_port_t*)malloc(sizeof(game_start_port_t));

		gamestartportp[i]->dashback_fix = (uint32_t)ntohl(ibp->gsb_port[i].dashback_fix);
		gamestartportp[i]->shield_drop_fix = (uint16_t)ntohs(ibp->gsb_port[i].shield_drop_fix);
		gamestartportp[i]->nametag = (uint32_t)ntohl(ibp->gsb_port[i].dashback_fix); //fix bc array

		gameibportp[i] = (game_info_block_port_t*)malloc(sizeof(game_info_block_port_t));
	};



	gamestartp->random_seed = (uint32_t)ntohl(ibp->random_seed);


	printf("Offset for random_seed = 0x%p\n", &(((infoblock_t*)0)->random_seed));

	return 0x1a2;

	malloc_fail:; //we have to allocate quite a bit of heap so this goto is useful here for oneline if statements
		DBG(printf("malloc failed\n"););
		// free_elements(elemlistp);
		return (0);
}
