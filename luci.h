#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

size_t map_and_process(char *filenamep);
int process_raw_data(void *ptr, size_t len); // change to pointer to game object return type


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


typedef float float_t;
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



#define VERSION_LENGTH	4
#define PORT_COUNT			4
#define NAMETAG_LENGTH	8
#define CHAR_COUNT			2
#define MAX_FRAMES			28800 // 8 minutes*60 seconds*60 frames. Don't need to worry about rollback frames as we overwrite them.
#define FIRST_FRAME			123
#define MAX_ITEMS				15



typedef struct GAME_START game_start_t;
typedef struct GAME_START_PORT game_start_port_t;
typedef struct GAME_INFO_BLOCK game_info_block_t; // technically a uint8[312], but struct makes it far more clear what everything is, + can be passed into python as an object
typedef struct GAME_INFO_BLOCK_PORT game_info_block_port_t;

struct GAME_START {
	uint8_t version[VERSION_LENGTH];
	game_info_block_t *game_info_block;
	uint32_t random_seed;
	game_start_port_t *ports[PORT_COUNT]; // pointer to each port, null if port is not used
	bool_t pal;
	bool_t frozen_ps;
	uint8_t minor_scene;
	uint8_t major_scene;
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
	float_t damage_ratio;
	game_info_block_port_t *ports[PORT_COUNT];
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
	float_t offense_ratio;
	float_t defense_ratio;
	float_t model_scale;
};

typedef struct __attribute__((__packed__)) GSFULLBLOCK  {
	uint8_t event_type;	// offset 0x0
	uint8_t version[VERSION_LENGTH]; // offset 0x1
	uint8_t	game_bitfield_1;	// offset 0x5
	uint8_t	game_bitfield_2;	// 0x6
	uint8_t game_bitfield_3;	// 0x7
	uint8_t _pad0[5]; // 5 byte skip
	uint8_t is_teams;	// 0xD
	uint8_t _pad1[2]; // 2 byte skip
	int8_t item_freq; // 0x10
	int8_t sd_value; // 0x11
	uint8_t _pad; // 1 byte skip
	uint16_t stage; // 0x13
	uint32_t timer_value; // 0x15
	uint8_t _pad2[15]; // 15 byte skip
	uint8_t item_spawn_bitfield_1; // 0x29
	uint8_t item_spawn_bitfield_2;
	uint8_t item_spawn_bitfield_3;
	uint8_t item_spawn_bitfield_4;
	uint8_t item_spawn_bitfield_5; // 0x2C
	uint8_t _pad3[8]; // 8 byte skip
	uint32_t damage_ratio; // 0x35
	uint8_t _pad4[0x2C]; // 0x2C byte skip
	struct __attribute__((__packed__)) {
		uint8_t extern_char_id; // 0x65 + 0x24i
		uint8_t player_type; // 0x66 + 0x24i
		uint8_t stock_start_count; // 0x67 + 0x24i
		uint8_t costume_index; // 0x68 + 0x24i
		uint8_t _pad5[3]; // 3 byte skip
		uint8_t team_shade; // 0x6C + 0x24i
		uint8_t handicap; // 0x6D + 0x24i
		uint8_t team_id; // 0x6E + 0x24i
		uint8_t _pad6[2]; // 2 byte skip
		uint8_t player_bitfield; // 0x71 + 0x24i
		uint8_t _pad7[2]; // 2 byte skip
		uint8_t cpu_level; // 0x74 + 0x24i
		uint8_t _pad8[4]; // 4 byte skip
		uint32_t offense_ratio; // 0x79 + 0x24i
		uint32_t defense_ratio; // 0x7D + 0x24i
		uint32_t model_scale; // 0x81 + 0x24i
		uint8_t _pad9[4]; // need to get to 24 bytes within port struct, 4 byte skip
	} gib_port[PORT_COUNT]; // ends at 0xF5
	uint8_t _pad10[2*0x24]; // infospace for 6 but slippi supports 4 because whatever so byte skip
	uint32_t random_seed;	// offset 0x13D
	struct __attribute__((__packed__)) {
		uint32_t dashback_fix;	// offset 0x141 + 0x8i
		uint32_t shield_drop_fix;	// offset 0x145 + 0x8i
	} gsb_port[PORT_COUNT]; // ends at 0x161
	uint16_t nametag[PORT_COUNT][NAMETAG_LENGTH];	// offset 0x161, 0x10*PORT_COUNT length
	uint8_t pal; // 0x1A1
	uint8_t frozen_ps; // 0x1A2
	uint8_t minor_scene; // 0x1A3
	uint8_t major_scene; // 0x1A4
} gsfullblock_t;



typedef struct GAME_END game_end_t;

struct GAME_END {
	uint8_t game_end_method;
	int8_t lras_init;
};

typedef struct __attribute__((__packed__)) GAMEENDFULLBLOCK  {
	uint8_t	event_type; // offset 0x0
	uint8_t game_end_method; // offset 0x1
	int8_t lras_init; // offset 0x2
} gameendfullblock_t;



typedef struct PRE_FRAME_UPDATE pre_frame_update_t;

struct PRE_FRAME_UPDATE {
	int32_t frame_number;
	uint8_t player_index;
	bool_t is_follower;
	uint32_t random_seed;
	uint16_t action_state_id;
	float_t x_position;
	float_t y_position;
	float_t facing_direction;
	float_t ctrlstick_x;
	float_t ctrlstick_y;
	float_t cstick_x;
	float_t cstick_y;
	float_t trigger;
	uint32_t processed_buttons;
	uint16_t physical_buttons;
	float_t physical_l_trigger;
	float_t physical_r_trigger;
	uint8_t ucf_x_analog;
	float_t damage_percent;
};

typedef struct __attribute__((__packed__)) PREFIFULLBLOCK  {
	uint8_t event_type;	// offset 0x0
	int32_t frame_number; // 0x1
	uint8_t player_index; // 0x5
	uint8_t is_follower; // 0x6
	uint32_t random_seed; // 0x7
	uint16_t action_state_id; // 0xB
	uint32_t x_position; // 0xD
	uint32_t y_position; // 0x11
	uint32_t facing_direction; // 0x15
	uint32_t ctrlstick_x; // 0x19
	uint32_t ctrlstick_y; // 0x1D
	uint32_t cstick_x; // 0x21
	uint32_t cstick_y; // 0x25
	uint32_t trigger; // 0x29
	uint32_t processed_buttons; // 0x2D
	uint16_t physical_buttons; // 0x31
	uint32_t physical_l_trigger; // 0x33
	uint32_t physical_r_trigger; // 0x37
	uint8_t ucf_x_analog; // 0x3B
	uint32_t damage_percent; // 0x3C
} prefifullblock_t;



typedef struct POST_FRAME_UPDATE post_frame_update_t;

struct POST_FRAME_UPDATE {
	int32_t frame_number;
	uint8_t player_index;
	bool_t is_follower;
	uint8_t intern_char_id;
	uint16_t action_state_id;
	float_t x_position;
	float_t y_position;
	float_t facing_direction;
	float_t damage_percent;
	float_t shield_strength;
	uint8_t last_hit_id;
	uint8_t current_combo_count;
	uint8_t last_hit_by;
	uint8_t stocks_remaining;
	float_t action_state_frames;
	uint8_t state_bitflags_1;
	uint8_t state_bitflags_2;
	uint8_t state_bitflags_3;
	uint8_t state_bitflags_4;
	uint8_t state_bitflags_5;
	float_t misc_as;
	bool_t ground_state;
	uint16_t last_ground_id;
	uint8_t jumps_remaining;
	uint8_t lcancel_status;
	uint8_t hurtbox_collision_state;
	float_t si_air_x_speed;
	float_t si_air_y_speed;
	float_t attack_air_x_speed;
	float_t attack_air_y_speed;
	float_t si_ground_x_speed;

};

typedef struct __attribute__((__packed__)) POSTFIFULLBLOCK  {
	uint8_t event_type;	// offset 0x0
	int32_t frame_number; // 0x1
	uint8_t player_index; // 0x5
	uint8_t is_follower; // 0x6
	uint8_t intern_char_id; // 0x7
	uint16_t action_state_id; // 0x8
	uint32_t x_position; // 0xA
	uint32_t y_position; // 0xE
	uint32_t facing_direction; // 0x12
	uint32_t damage_percent; // 0x16
	uint32_t shield_strength; // 0x1A
	uint8_t last_hit_id; // 0x1E
	uint8_t current_combo_count; // 0x1F
	uint8_t last_hit_by; // 0x20
	uint8_t stocks_remaining; // 0x21
	uint32_t action_state_frames; // 0x22
	uint8_t state_bitflags_1; // 0x26
	uint8_t state_bitflags_2;
	uint8_t state_bitflags_3;
	uint8_t state_bitflags_4;
	uint8_t state_bitflags_5; // 0x2A
	uint32_t misc_as; // 0x2B
	uint8_t ground_state; // 0x2F
	uint16_t last_ground_id; // 0x30
	uint8_t jumps_remaining; // 0x32
	uint8_t lcancel_status; // 0x33
	uint8_t hurtbox_collision_state; // 0x34
	uint32_t si_air_x_speed; // 0x35
	uint32_t si_air_y_speed; // 0x39
	uint32_t attack_air_x_speed; // 0x3D
	uint32_t attack_air_y_speed; // 0x41
	uint32_t si_ground_x_speed; // 0x45
} postfifullblock_t;



typedef struct ITEM_UPDATE item_update_t;

struct ITEM_UPDATE {
	bool_t valid;
	int32_t frame_number;
	uint16_t type_id;
	uint8_t state;
	float_t facing_direction;
	float_t x_velocity;
	float_t y_velocity;
	float_t x_position;
	float_t y_position;
	uint16_t damage_taken;
	float_t expiration_timer;
	uint32_t spawn_id;
	uint8_t misc_1;
	uint8_t misc_2;
	uint8_t misc_3;
	uint8_t misc_4;
	int8_t owner;
};

typedef struct __attribute__((__packed__)) ITEMUPDATEFULLINFOBLOCK  {
	uint8_t event_type;	// offset 0x0
	int32_t frame_number; // 0x1
	uint16_t type_id; // 0x5
	uint8_t state; // 0x7
	uint32_t facing_direction; // 0x8
	uint32_t x_velocity; // 0xC
	uint32_t y_velocity; // 0x10
	uint32_t x_position; // 0x14
	uint32_t y_position; // 0x18
	uint16_t damage_taken; // 0x1C
	uint32_t expiration_timer; // 0x1E
	uint32_t spawn_id; // 0x22
	uint8_t misc_1; // 0x23
	uint8_t misc_2; // 0x24
	uint8_t misc_3; // 0x25
	uint8_t _pad0; // 0x26
	uint8_t misc_4; // 0x27
	int8_t owner; // 0x28
} itemupdatefullinfoblock_t;

typedef struct FRAME_OBJ frame_obj_t;

struct FRAME_OBJ {
	struct {
		struct {
			pre_frame_update_t preframe;
			post_frame_update_t postframe;
		} char_frames[CHAR_COUNT]; // 0 is leader, 1 is follower or NULL values if no follower
	} ports[PORT_COUNT]; // NULL values if no player on port
	item_update_t itemupdatearray[MAX_ITEMS];
};



typedef struct GAME_OBJ game_obj_t;

struct GAME_OBJ {
	game_start_t *gamestartp;
	frame_obj_t *framearrayp;
	game_end_t *gameendp;
};
