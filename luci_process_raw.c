#include "luci.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>

static size_t process_game_start(uint8_t *p, game_start_t *gamestartp);
static size_t process_pre_frame_update(uint8_t *p, frame_obj_t *framearray);
static size_t process_post_frame_update(uint8_t *p, frame_obj_t *framearray);
static size_t process_game_end(uint8_t *p, game_end_t *gameendp);
static size_t process_event(uint8_t *p);
typedef enum { EVENT_PAYLOADS = 0x35, EVENT_GAME_START = 0x36, EVENT_PRE_FRAME_UPDATE = 0x37, EVENT_POST_FRAME_UPDATE = 0x38, EVENT_GAME_END = 0x39, EVENT_FRAME_START = 0x3A, EVENT_ITEM_UPDATE = 0x3B, EVENT_FRAME_BOOKEND = 0x3C, EVENT_GECKO_LIST = 0x3D, EVENT_MESSAGE_SPLITTER = 0x10 } event_t;
uint8_t version[4]; // very important for all functions, hence global

size_t gamestartsize;
size_t preframesize;
size_t postframesize;
size_t itemupdatesize;
size_t framestartsize;
size_t frameendsize;
size_t gameendsize;



int process_raw_data(void *ptr, size_t len)
{
	uint8_t *p = (uint8_t*)ptr;
	uint8_t *currentp = p;
	size_t offset = 0;
	event_t type;

	int gs_count = 0;
	int ge_count = 0;

	frame_obj_t *framearray = (frame_obj_t *)malloc(MAX_FRAMES * sizeof(frame_obj_t)); // first frame object pointer, set null values here

	game_obj_t *game_obj = (game_obj_t *)malloc(sizeof(game_obj_t));

	size_t event_size;

	if (framearray == NULL) goto fail;

	do {
		type = p[offset];
		currentp = p + offset;

		switch(type) {
			case EVENT_PAYLOADS:;
				event_size = process_event(currentp); // should be first object, then never encountered again
				break;
			case EVENT_GAME_START:;
				gs_count++;
				if (gs_count >= 2) goto fail;
				game_start_t *gamestartp = (game_start_t *)malloc(sizeof(game_start_t));
				event_size = process_game_start(currentp, gamestartp);
				game_obj->gamestartp = gamestartp;
				printf("version: %d.%d.%d \n", version[0], version[1], version[2]); // make version control
				break;
			case EVENT_PRE_FRAME_UPDATE:;
				event_size = process_pre_frame_update(currentp, framearray);
				break;
			case EVENT_POST_FRAME_UPDATE:;
				event_size = process_post_frame_update(currentp, framearray);
				break;
			case EVENT_GAME_END:;
				ge_count++;
				if (ge_count >= 2) goto fail;
				game_end_t *gameendp = (game_end_t *)malloc(sizeof(post_frame_update_t));
				event_size = process_game_end(currentp, gameendp);
				game_obj->gameendp = gameendp;
				goto success;
				break;
			case EVENT_FRAME_START:;
				event_size = 0x9;
				break;
			case EVENT_ITEM_UPDATE:;
				event_size = 0x29; // no
				break;
			case EVENT_FRAME_BOOKEND:;
				event_size = 0x9;
				break;
			case EVENT_MESSAGE_SPLITTER:;
				event_size = 0x205;
				break;
			default:;
				printf("failed at %lx: %X\n", offset, type);
				event_size = 0;
				break;
		}

		// if (last_frame == -123) {
		// 	game_obj->frames = framep;
		// }

		if (event_size <= 0) goto fail;

		offset += event_size;
	} while (true);

	success:;
		DBG(printf("end of object \n"););
		DBG(fflush(stdout););
		return 0;

	fail:;
		DBG(printf("raw operation failed\n"););
		// free_elements(elemlistp);
		return (0);
}

// utils

// static int count_chars(game_info_block_port_t *ports[PORT_COUNT])
// {
// 	int char_count = 0;
// 	for (int i = 0;i < PORT_COUNT;i++){
// 		if (ports[i]->player_type != 3) {
// 			if (ports[i]->extern_char_id == 0x0E) {char_count += 2;} else {char_count += 1;}
// 		}
// 	}
// 	return char_count;
// }

static float ntohf(uint32_t net32)
{
    union {
        float f;
        uint32_t u;
    } value;

    value.u = ntohl(net32);

    return value.f;
}

// process functions



static size_t process_event(uint8_t *p)
{
	for (int i = 0;i < (p[1]-1);i+=3) {
		switch(p[i + 1]) {
			case EVENT_GAME_START:;
				gamestartsize = (size_t)ntohs(p[2 + i]);
				break;
			case EVENT_PRE_FRAME_UPDATE:;
				preframesize = (size_t)ntohs(p[2 + i]);
				break;
			case EVENT_POST_FRAME_UPDATE:;
				postframesize = (size_t)ntohs(p[2 + i]);
				break;
			case EVENT_ITEM_UPDATE:;
				itemupdatesize = (size_t)ntohs(p[2 + i]);
				break;
			case EVENT_FRAME_START:;
				framestartsize = (size_t)ntohs(p[2 + i]);
				break;
			case EVENT_FRAME_BOOKEND:;
				frameendsize = (size_t)ntohs(p[2 + i]);
				break;
			case EVENT_GAME_END:;
				gameendsize = (size_t)ntohs(p[2 + i]);
				break;
		}
	}
	return (p[1]+1);
}



static size_t process_game_start(uint8_t *p, game_start_t *gamestartp)
{

	gsfullblock_t *ibp = (gsfullblock_t *)p;
	if (ibp->event_type != EVENT_GAME_START) return (0);

	game_info_block_t *gameinfoblockp = (game_info_block_t*)malloc(sizeof(game_info_block_t));

	//game_start_port_t *gamestartportp[PORT_COUNT];
	// game_info_block_port_t *gameibportp[PORT_COUNT];
	// assert(gamestartp != NULL);
	// assert(gameinfoblockp != NULL);
	if (gamestartp == NULL || gameinfoblockp == NULL) goto malloc_fail;

	for(int i = 0;i<PORT_COUNT;i++){
		game_info_block_port_t *gip = (game_info_block_port_t*)malloc(sizeof(game_info_block_port_t));
		game_start_port_t *gsp = (game_start_port_t*)malloc(sizeof(game_start_port_t));

		// assert(gip != NULL);
		// assert(gsp != NULL);
		if (gip == NULL || gsp == NULL) goto malloc_fail;

		gsp->dashback_fix = (uint32_t)ntohl(ibp->gsb_port[i].dashback_fix);
		gsp->shield_drop_fix = (uint32_t)ntohl(ibp->gsb_port[i].shield_drop_fix);
		for (int j=0; j<NAMETAG_LENGTH; j++) gsp->nametag[j] = (uint16_t)ntohs(ibp->nametag[i][j]);

		gip->extern_char_id = ibp->gib_port[i].extern_char_id;
		gip->player_type = ibp->gib_port[i].player_type;
		gip->stock_start_count = ibp->gib_port[i].stock_start_count;
		gip->costume_index = ibp->gib_port[i].costume_index;
		gip->team_shade = ibp->gib_port[i].team_shade;
		gip->handicap = ibp->gib_port[i].handicap;
		gip->team_id = ibp->gib_port[i].team_id;
		gip->player_bitfield = ibp->gib_port[i].player_bitfield;
		gip->cpu_level = ibp->gib_port[i].cpu_level;
		gip->offense_ratio = ntohf(ibp->gib_port[i].offense_ratio); // FLOATS!! AAAAARGHHH have to do bs ntohf or smth
		gip->defense_ratio = ntohf(ibp->gib_port[i].defense_ratio);
		gip->model_scale = ntohf(ibp->gib_port[i].model_scale);

		gameinfoblockp->ports[i] = gip;
		gamestartp->ports[i] = gsp;
	};

	memcpy(gamestartp->version, ibp->version, VERSION_LENGTH); // don't need to byte flip bc uint8 so no for loop lulz
	gamestartp->game_info_block = gameinfoblockp;
	gamestartp->random_seed = (uint32_t)ntohl(ibp->random_seed);
	gamestartp->pal = (bool_t)ibp->pal;
	gamestartp->frozen_ps = (bool_t)ibp->frozen_ps;
	gamestartp->minor_scene = ibp->minor_scene;
	gamestartp->major_scene = ibp->major_scene;

	gameinfoblockp->game_bitfield_1 = ibp->game_bitfield_1;
	gameinfoblockp->game_bitfield_2 = ibp->game_bitfield_2;
	gameinfoblockp->game_bitfield_3 = ibp->game_bitfield_3;
	gameinfoblockp->is_teams = (bool_t)ibp->is_teams;
	gameinfoblockp->item_freq = (int8_t)ibp->item_freq;
	gameinfoblockp->sd_value = (int8_t)ibp->sd_value;
	gameinfoblockp->stage = (uint16_t)ntohs(ibp->stage);
	gameinfoblockp->timer_value = (uint32_t)ntohl(ibp->timer_value);
	gameinfoblockp->item_spawn_bitfield_1 = ibp->item_spawn_bitfield_1;
	gameinfoblockp->item_spawn_bitfield_2 = ibp->item_spawn_bitfield_2;
	gameinfoblockp->item_spawn_bitfield_3 = ibp->item_spawn_bitfield_3;
	gameinfoblockp->item_spawn_bitfield_4 = ibp->item_spawn_bitfield_4;
	gameinfoblockp->item_spawn_bitfield_5 = ibp->item_spawn_bitfield_5;
	gameinfoblockp->damage_ratio = ntohf(ibp->damage_ratio); //ntohf
//	printf("%p, %p, %p, %p \n", gameibportp[0], gameibportp[1], gameibportp[2], gameinfoblockp->ports[3]);

	// printf("Offset for pal = %p\n", &(((gsfullblock_t*)0)->pal)); // oll korrect

	memcpy(&version, &gamestartp->version, sizeof(gamestartp->version)); // set global version

	return gamestartsize;

	malloc_fail:; //we have to allocate quite a bit of heap so this goto is useful here for oneline if statements
		DBG(printf("malloc failed\n"););
		// free_elements(elemlistp);
		return (0);
}



static size_t process_pre_frame_update(uint8_t *p, frame_obj_t *framearray)
{
	prefifullblock_t *ibp = (prefifullblock_t *)p;
	if (ibp->event_type != EVENT_PRE_FRAME_UPDATE) return (0);

	int32_t framecount = (int32_t)ntohl(ibp->frame_number);
	uint8_t port = ibp->player_index;
	uint8_t char_count = ibp->is_follower;

	pre_frame_update_t *preframep = &(framearray[framecount].ports[port].char_frames[char_count].preframe);

	preframep->frame_number = framecount;
	preframep->player_index = port;
	preframep->is_follower = (bool_t)char_count;
	preframep->random_seed = (uint32_t)ntohl(ibp->random_seed);
	preframep->action_state_id = (uint16_t)ntohs(ibp->action_state_id);
	preframep->x_position = ntohf(ibp->x_position);
	preframep->y_position = ntohf(ibp->y_position);
	preframep->facing_direction = ntohf(ibp->facing_direction);
	preframep->ctrlstick_x = ntohf(ibp->ctrlstick_x);
	preframep->ctrlstick_y = ntohf(ibp->ctrlstick_y);
	preframep->cstick_x = ntohf(ibp->cstick_x);
	preframep->cstick_y = ntohf(ibp->cstick_y);
	preframep->trigger = ntohf(ibp->trigger);
	preframep->processed_buttons = (uint32_t)ntohl(ibp->processed_buttons);
	preframep->physical_buttons = (uint16_t)ntohs(ibp->physical_buttons);
	preframep->physical_l_trigger = ntohf(ibp->physical_l_trigger);
	preframep->physical_r_trigger = ntohf(ibp->physical_r_trigger);
	preframep->ucf_x_analog = ibp->ucf_x_analog;
	preframep->damage_percent = ntohf(ibp->damage_percent);

	return preframesize;
}



static size_t process_post_frame_update(uint8_t *p, frame_obj_t *framearray)
{
	postfifullblock_t *ibp = (postfifullblock_t *)p;
	if (ibp->event_type != EVENT_POST_FRAME_UPDATE) return (0);

	int32_t framecount = (int32_t)ntohl(ibp->frame_number);
	uint8_t port = ibp->player_index;
	uint8_t char_count = ibp->is_follower;

	post_frame_update_t *postframep = &(framearray[framecount].ports[port].char_frames[char_count].postframe);

	postframep->frame_number = framecount;
	postframep->player_index = port;
	postframep->is_follower = (bool_t)char_count;
	postframep->intern_char_id = ibp->intern_char_id;
	postframep->action_state_id = (uint16_t)ntohs(ibp->action_state_id);
	postframep->x_position = ntohf(ibp->x_position);
	postframep->y_position = ntohf(ibp->y_position);
	postframep->facing_direction = ntohf(ibp->facing_direction);
	postframep->damage_percent = ntohf(ibp->damage_percent);
	postframep->shield_strength = ntohf(ibp->shield_strength);
	postframep->last_hit_id = ibp->last_hit_id;
	postframep->current_combo_count = ibp->current_combo_count;
	postframep->last_hit_by = ibp->last_hit_by;
	postframep->stocks_remaining = ibp->stocks_remaining;
	postframep->action_state_frames = ntohf(ibp->action_state_frames);
	postframep->state_bitflags_1 = ibp->state_bitflags_1;
	postframep->state_bitflags_2 = ibp->state_bitflags_2;
	postframep->state_bitflags_3 = ibp->state_bitflags_3;
	postframep->state_bitflags_4 = ibp->state_bitflags_4;
	postframep->state_bitflags_5 = ibp->state_bitflags_5;
	postframep->misc_as = ntohf(ibp->misc_as);
	postframep->ground_state = (bool_t)ibp->ground_state;
	postframep->last_ground_id = (uint16_t)ntohs(ibp->last_ground_id);
	postframep->jumps_remaining = ibp->jumps_remaining;
	postframep->lcancel_status = ibp->lcancel_status;
	postframep->hurtbox_collision_state = ibp->hurtbox_collision_state;
	postframep->si_air_x_speed = ntohf(ibp->si_air_x_speed);
	postframep->si_air_y_speed = ntohf(ibp->si_air_y_speed);
	postframep->attack_air_x_speed = ntohf(ibp->attack_air_x_speed);
	postframep->attack_air_y_speed = ntohf(ibp->attack_air_y_speed);
	postframep->si_ground_x_speed = ntohf(ibp->si_ground_x_speed);


	return postframesize;
}



static size_t process_game_end(uint8_t *p, game_end_t *gameendp) // making a function for this is silly? no u
{
	gameendfullblock_t *ibp = (gameendfullblock_t *)p;
	if (ibp->event_type != EVENT_GAME_END) return (0);

	if (gameendp == NULL) goto malloc_fail;

	gameendp->game_end_method = ibp->game_end_method;
	gameendp->lras_init = ibp->lras_init;

	return gameendsize;

	malloc_fail:; //we have to allocate quite a bit of heap so this goto is useful here for oneline if statements
		DBG(printf("malloc failed\n"););
		// free_elements(elemlistp);
		return (0);
}