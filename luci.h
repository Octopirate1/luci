#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

size_t map_and_process(char *filenamep);
void process_raw_data(void *ptr, size_t len);


#ifdef DEBUG
#define LUCI_DEBUG 1
#define	DBG(s)	do { s } while (0)
void dump_mem(void *memp, size_t offset, int len);
#else
#define LUCI_DEBUG	0
#define	DBG(s)	do { } while (0)
#endif
	//
	// This is a two step parse, but fast.
	// The first is to unpack a UBJSON binary file
	// The second step is to extract the contents of the "raw" element in its uppermost
	// object.
	//


typedef bool bool_t;
typedef enum ELEMENT_TYPE { ET_New = 0, ET_Char, ET_String, ET_Int, ET_Float, ET_Array, ET_Object } element_type_t;

typedef struct ELEMENT element_t;
typedef struct ARRAY_BLOCK array_block_t;

struct ELEMENT {
	element_type_t type;
	char *namep;
	union {
		int char_value;
		char *stringp;
		int64_t integer_value;
		double float_value;
		array_block_t *arrayp;
		element_t *object_listp;
	} u;
	element_t *nextp;
};

	// Only supports fixed size elements
	// build a chain of these for multi-sized array elements.
struct ARRAY_BLOCK {
	size_t count;
	size_t size;
	void *datap;
	array_block_t *nextp;
};


element_t *new_element();
array_block_t *new_array_block(void *memp, size_t count, size_t size);
element_t *add_element(element_t *listp, element_t *newp);
void element_list_dump(element_t *listp);
void element_dump(element_t *elemp, int indent);
void element_list_recurse(element_t *listp, int indent);
void free_elements(element_t *listp);
element_t *find_element_by_name(element_t *listp, char *namep);

typedef struct GAME_START game_start_t;
typedef struct GAME_START_PORT game_start_port_t;
typedef struct GAME_INFO_BLOCK game_info_block_t; // technically a uint8[312], but struct makes it far more clear what everything is, + can be passed into python as an object
typedef struct GAME_INFO_BLOCK_PORT game_info_block_port_t; // technically a uint8[312], but struct makes it far more clear what everything is, + can be passed into python as an object

#define VERSION_LENGTH	4
#define PORT_AMOUNT		4
#define NAMETAG_LENGTH	8
struct GAME_START {
	uint8_t version[VERSION_LENGTH];
	game_info_block_t *game_info_block;
	uint32_t random_seed;
	game_start_port_t *ports[PORT_AMOUNT]; // pointer to each port, null if port is not used
	bool_t pal;
	bool_t frozen_ps;
};

struct GAME_START_PORT {
	uint32_t dashback_fix;
	uint32_t shield_drop_fix;
	uint16_t nametag[NAMETAG_LENGTH];
};

struct GAME_INFO_BLOCK {
	uint8_t game_bitfield_1;
	uint8_t game_bitfield_2;
	uint8_t game_bitfield_3;
	bool_t is_teams;
	int8_t item_freq;
	int8_t sd_value;
	uint16_t stage;
	uint32_t timer_value;
	uint8_t item_spawn_bitfield_1;
	uint8_t item_spawn_bitfield_2;
	uint8_t item_spawn_bitfield_3;
	uint8_t item_spawn_bitfield_4;
	uint8_t item_spawn_bitfield_5;
	float damage_ratio;
	game_info_block_port_t *ports[PORT_AMOUNT];
};

struct GAME_INFO_BLOCK_PORT {
	uint8_t extern_char_id;
	uint8_t player_type;
	uint8_t stock_start_count;
	uint8_t costume_index;
	uint8_t team_shade;
	uint8_t handicap;
	uint8_t team_id;
	uint8_t player_bitfield;
	uint8_t cpu_level;
	float offense_ratio;
	float defense_ratio;
	float model_scale;
};
