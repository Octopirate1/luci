#include "luci.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

static size_t process_game_start(uint8_t *p, game_start_t *gamestartp);
static size_t process_pre_frame_update(uint8_t *p, pre_frame_update_t *preframep);
static size_t process_post_frame_update(uint8_t *p, post_frame_update_t *postframep);
static size_t process_event(uint8_t *p);
typedef enum { EVENT_PAYLOADS = 0x35, EVENT_GAME_START = 0x36, EVENT_PRE_FRAME_UPDATE = 0x37, EVENT_POST_FRAME_UPDATE = 0x38, EVENT_GAME_END = 0x39 } event_t;

void process_raw_data(void *ptr, size_t len)
{
	uint8_t *p = (uint8_t*)ptr;
	uint8_t *currentp = p;
	size_t offset = 0;
	event_t type;

	// pre_frame_update_t *prevpreframep = NULL;

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
				game_start_t *gamestartp = (game_start_t *)malloc(sizeof(game_start_t));
				event_size = process_game_start(currentp, gamestartp);
				break;
			case EVENT_PRE_FRAME_UPDATE:;
				pre_frame_update_t *preframep = (pre_frame_update_t *)malloc(sizeof(pre_frame_update_t));
				event_size = process_pre_frame_update(currentp, preframep);
				break;
			case EVENT_POST_FRAME_UPDATE:;
				post_frame_update_t *postframep = (post_frame_update_t *)malloc(sizeof(post_frame_update_t));
				event_size = process_post_frame_update(currentp, postframep);
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

static float ntohf(uint32_t net32)
{
    union {
        float f;
        uint32_t u;
    } value;

    value.u = ntohl(net32);

    return value.f;
}

static size_t process_game_start(uint8_t *p, game_start_t *gamestartp)
{

	gsfullblock_t *ibp = (gsfullblock_t *)p;
	if (ibp->event_type != EVENT_GAME_START) return (0);

	game_info_block_t *gameinfoblockp = (game_info_block_t*)malloc(sizeof(game_info_block_t));

	game_start_port_t *gamestartportp[PORT_COUNT];
	game_info_block_port_t *gameibportp[PORT_COUNT];

	if (gamestartp == NULL || gameinfoblockp == NULL || gamestartportp == NULL || gameibportp == NULL) goto malloc_fail;

	for(int i = 0;i<PORT_COUNT;i++){
		gamestartportp[i] = (game_start_port_t*)malloc(sizeof(game_start_port_t));

		gamestartportp[i]->dashback_fix = (uint32_t)ntohl(ibp->gsb_port[i].dashback_fix);
		gamestartportp[i]->shield_drop_fix = (uint32_t)ntohl(ibp->gsb_port[i].shield_drop_fix);
		for (int j=0; j<NAMETAG_LENGTH; j++) gamestartportp[i]->nametag[j] = (uint16_t)ntohs(ibp->nametag[i][j]);

		gameibportp[i] = (game_info_block_port_t*)malloc(sizeof(game_info_block_port_t));

		gameibportp[i]->extern_char_id = ibp->gib_port[i].extern_char_id;
		gameibportp[i]->player_type = ibp->gib_port[i].player_type;
		gameibportp[i]->stock_start_count = ibp->gib_port[i].stock_start_count;
		gameibportp[i]->costume_index = ibp->gib_port[i].costume_index;
		gameibportp[i]->team_shade = ibp->gib_port[i].team_shade;
		gameibportp[i]->handicap = ibp->gib_port[i].handicap;
		gameibportp[i]->team_id = ibp->gib_port[i].team_id;
		gameibportp[i]->player_bitfield = ibp->gib_port[i].player_bitfield;
		gameibportp[i]->cpu_level = ibp->gib_port[i].cpu_level;
		gameibportp[i]->offense_ratio = ntohf(ibp->gib_port[i].offense_ratio); // FLOATS!! AAAAARGHHH have to do bs ntohf or smth
		gameibportp[i]->defense_ratio = ntohf(ibp->gib_port[i].defense_ratio);
		gameibportp[i]->model_scale = ntohf(ibp->gib_port[i].model_scale);
	};

	memcpy(gamestartp->version, ibp->version, VERSION_LENGTH); // don't need to byte flip bc uint8 so no for loop lulz
	gamestartp->game_info_block = gameinfoblockp;
	gamestartp->random_seed = (uint32_t)ntohl(ibp->random_seed);
	memcpy(gamestartp->ports, gamestartportp, PORT_COUNT);
	gamestartp->pal = (bool_t)ibp->pal;
	gamestartp->frozen_ps = (bool_t)ibp->frozen_ps;

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
	memcpy(gameinfoblockp->ports, gameibportp, PORT_COUNT);

	// printf("Offset for pal = %p\n", &(((gsfullblock_t*)0)->pal)); // oll korrect

	return 0x1a2;

	malloc_fail:; //we have to allocate quite a bit of heap so this goto is useful here for oneline if statements
		DBG(printf("malloc failed\n"););
		// free_elements(elemlistp);
		return (0);
}

static size_t process_pre_frame_update(uint8_t *p, pre_frame_update_t *preframep)
{
	prefifullblock_t *ibp = (prefifullblock_t *)p;
	if (ibp->event_type != EVENT_PRE_FRAME_UPDATE) return (0);

	if (preframep == NULL) goto malloc_fail;

	preframep->frame_number = (int32_t)ntohl(ibp->frame_number);
	preframep->player_index = ibp->player_index;
	preframep->is_follower = (bool_t)ibp->is_follower;
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

	return 0x40;

	malloc_fail:; //we have to allocate quite a bit of heap so this goto is useful here for oneline if statements
		DBG(printf("malloc failed\n"););
		// free_elements(elemlistp);
		return (0);
}

static size_t process_post_frame_update(uint8_t *p, post_frame_update_t *postframep)
{
	postfifullblock_t *ibp = (postfifullblock_t *)p;
	if (ibp->event_type != EVENT_POST_FRAME_UPDATE) return (0);

	if (postframep == NULL) goto malloc_fail;

	postframep->frame_number = (int32_t)ntohl(ibp->frame_number);
	postframep->player_index = ibp->player_index;
	postframep->is_follower = (bool_t)ibp->is_follower;
	postframep->intern_char_id = ibp->intern_char_id;
	postframep->action_state_id = (uint16_t)ntohs(ibp->action_state_id);
	postframep->x_position = ntohf(ibp->x_position);
	postframep->y_position = ntohf(ibp->y_position);
	postframep->facing_direction = ntohf(ibp->facing_direction);
	postframep->damage_percent = ntohf(ibp->damage_percent);
	postframep->shield_size = ntohf(ibp->shield_size);
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

	return 0x49;

	malloc_fail:; //we have to allocate quite a bit of heap so this goto is useful here for oneline if statements
		DBG(printf("malloc failed\n"););
		// free_elements(elemlistp);
		return (0);
}
