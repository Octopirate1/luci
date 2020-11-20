#include "../luci.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "endianag.h"

static size_t process_game_start(uint8_t *p, game_start_t *gamestartp);
static size_t process_pre_frame_update(uint8_t *p, frame_t *framearrayp);
static size_t process_post_frame_update(uint8_t *p, frame_t *framearrayp);
static size_t process_game_end(uint8_t *p, game_end_t *gameendp);
static size_t process_item_update(uint8_t *p, frame_t *framearrayp);
static size_t process_event(uint8_t *p);

uint8_t version[4];

game_t *process_raw_data(void *ptr, size_t len, int versionctrl[])
{
	uint8_t *p = (uint8_t*)ptr;
	uint8_t *currentp = p;
	size_t offset = 0;
	event_t type;

	int gs_count = 0;
	int ge_count = 0;

	frame_t *framearrayp = (frame_t *)calloc(MAX_FRAMES, sizeof(frame_t));

	DBG(printf("size of frames array: %lu, size of frames_obj: %lu \n", sizeof(frame_t) * MAX_FRAMES, sizeof(frame_t)););

	game_t *gamep = (game_t *)malloc(sizeof(game_t));

	gamep->framearrayp = framearrayp;

	size_t event_size;

	if (framearrayp == NULL) goto malloc_fail;

	do {

		if (offset >= len) {
			DBG(printf("Raw block exceeds expected raw size, program aborting. Game end block does not exist \n"););
			goto fail;
		}

		type = p[offset];
		currentp = p + offset;

		switch(type) {
			case EVENT_PAYLOADS:;
				event_size = process_event(currentp); // should be first object, then never encountered again
				DBG(if (event_size <= 0) printf("error reading event payloads \n"););
				break;
			case EVENT_GAME_START:;
				gs_count++;
				if (gs_count >= 2) goto count_fail;
				game_start_t *gamestartp = (game_start_t *)malloc(sizeof(game_start_t));
				event_size = process_game_start(currentp, gamestartp);
				DBG(if (event_size <= 0) printf("error reading game start \n"););
				gamep->gamestartp = gamestartp;
				DBG(printf("version: %d.%d.%d \n", version[0], version[1], version[2]););
				DBG(printf("versionctrl: %d.%d.%d \n", versionctrl[0], versionctrl[1], versionctrl[2]););
				if (version[0] < versionctrl[0] || (version[0] == versionctrl[0] && version[1] < versionctrl[1]) || (version[0] == versionctrl[0] && version[1] == versionctrl[1] && version[2] < versionctrl[2])) fprintf(stderr, "Version of file (%d.%d.%d) is below version specified (%d.%d.%d)\n", version[0], version[1], version[2], versionctrl[0], versionctrl[1], versionctrl[2]);
				break;
			case EVENT_PRE_FRAME_UPDATE:;
				event_size = process_pre_frame_update(currentp, framearrayp);
				DBG(if (event_size <= 0) printf("error reading preframe update \n"););
				break;
			case EVENT_POST_FRAME_UPDATE:;
				event_size = process_post_frame_update(currentp, framearrayp);
				DBG(if (event_size <= 0) printf("error reading postframe update \n"););
				break;
			case EVENT_GAME_END:;
				ge_count++;
				if (ge_count >= 2) goto count_fail;
				game_end_t *gameendp = (game_end_t *)malloc(sizeof(post_frame_update_t));
				event_size = process_game_end(currentp, gameendp);
				DBG(if (event_size <= 0) printf("error reading game end \n"););
				gamep->gameendp = gameendp;
				goto success;
				break;
			case EVENT_FRAME_START:;
				event_size = framestartsize;
				break;
			case EVENT_ITEM_UPDATE:;
				event_size = process_item_update(currentp, framearrayp);
				DBG(if (event_size <= 0) printf("error reading item update \n"););
				break;
			case EVENT_FRAME_BOOKEND:;
				event_size = frameendsize;
				break;
			case EVENT_MESSAGE_SPLITTER:;
				event_size = messagesplitsize;
				break;
			default:;
				DBG(printf("failed at %zu: %X\n", offset, type););
				event_size = 0;
				break;
		}


		if (event_size <= 0) goto fail;

		offset += event_size;
	} while (true);

	success:;
		DBG(printf("end of object \n"););
		DBG(fflush(stdout););
		return gamep;

	fail:;
		DBG(printf("raw operation failed\n"););
		DBG(fflush(stdout););
		return (NULL);

	malloc_fail:;
		DBG(printf("memory allocation failure\n"););
		goto fail;

	count_fail:;
		DBG(printf("game end or start counted twice\n"););
		goto fail;
}



static size_t process_event(uint8_t *p)
{
	if (p[0] != EVENT_PAYLOADS) return (0);
	for (int i = 2;i < p[1];i+=3) {
		uint16_t *sizep = (uint16_t *)&(p[i + 1]);
		size_t payloadsize = (size_t)ntohs(*sizep) + 1;
		DBG(printf("event_type: %x, size: %lx \n", p[i], payloadsize););
		switch(p[i]) {
			case EVENT_GAME_START:;
				gamestartsize = payloadsize;
				break;
			case EVENT_PRE_FRAME_UPDATE:;
				preframesize = payloadsize;
				break;
			case EVENT_POST_FRAME_UPDATE:;
				postframesize = payloadsize;
				break;
			case EVENT_ITEM_UPDATE:;
				itemupdatesize = payloadsize;
				break;
			case EVENT_FRAME_START:;
				framestartsize = payloadsize;
				break;
			case EVENT_FRAME_BOOKEND:;
				frameendsize = payloadsize;
				break;
			case EVENT_GAME_END:;
				gameendsize = payloadsize;
				break;
			case EVENT_GECKO_LIST:;
				geckolistsize = payloadsize;
				break;
			case EVENT_MESSAGE_SPLITTER:;
				messagesplitsize = payloadsize;
				break;
			default:;
				DBG(printf("undocumented command byte in payload event payload \n"););
				return (0);
		}
	}
	return (p[1]+1);
}



static size_t process_game_start(uint8_t *p, game_start_t *gamestartp)
{

	gsfullblock_t *ibp = (gsfullblock_t *)p;
	if (ibp->event_type != EVENT_GAME_START) return (0);

	game_info_block_t *gameinfoblockp = (game_info_block_t*)malloc(sizeof(game_info_block_t));

	if (gamestartp == NULL || gameinfoblockp == NULL) goto malloc_fail;

	for(int i = 0;i<PORT_COUNT;i++){
		game_info_block_port_t *gip = (game_info_block_port_t*)malloc(sizeof(game_info_block_port_t));
		game_start_port_t *gsp = (game_start_port_t*)malloc(sizeof(game_start_port_t));

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
		gip->offense_ratio = ntohf(ibp->gib_port[i].offense_ratio);
		gip->defense_ratio = ntohf(ibp->gib_port[i].defense_ratio);
		gip->model_scale = ntohf(ibp->gib_port[i].model_scale);

		gameinfoblockp->ports[i] = gip;
		gamestartp->ports[i] = gsp;
	};

	memcpy(gamestartp->version, ibp->version, VERSION_LENGTH);
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
	gameinfoblockp->damage_ratio = ntohf(ibp->damage_ratio);

	memcpy(&version, &gamestartp->version, sizeof(gamestartp->version)); // set global version

	return gamestartsize;

	malloc_fail:;
		DBG(printf("gamestart malloc failed\n"););
		return (0);
}



static size_t process_pre_frame_update(uint8_t *p, frame_t *framearrayp)
{
	prefifullblock_t *ibp = (prefifullblock_t *)p;
	if (ibp->event_type != EVENT_PRE_FRAME_UPDATE) return (0);

	int32_t framecount = (int32_t)ntohl(ibp->frame_number);
	uint8_t port = ibp->player_index;
	uint8_t char_count = ibp->is_follower;

	if (framecount + FIRST_FRAME >= MAX_FRAMES) {
		DBG(printf("more frames than full 8 minutes \n"););
		return (0);
	} else if (framecount < -FIRST_FRAME) {
		DBG(printf("negative amount of frames \n"););
		return (0);
	}

	pre_frame_update_t *preframep = &(framearrayp[framecount + FIRST_FRAME].ports[port].char_frames[char_count].preframe);

	preframep->valid = true;
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



static size_t process_post_frame_update(uint8_t *p, frame_t *framearrayp)
{
	postfifullblock_t *ibp = (postfifullblock_t *)p;
	if (ibp->event_type != EVENT_POST_FRAME_UPDATE) return (0);

	int32_t framecount = (int32_t)ntohl(ibp->frame_number);
	uint8_t port = ibp->player_index;
	uint8_t char_count = ibp->is_follower;

	if (framecount + FIRST_FRAME >= MAX_FRAMES) {
		DBG(printf("more frames than full 8 minutes \n"););
		return (0);
	} else if (framecount < -FIRST_FRAME) {
		DBG(printf("negative amount of frames \n"););
		return (0);
	}

	post_frame_update_t *postframep = &(framearrayp[framecount + FIRST_FRAME].ports[port].char_frames[char_count].postframe);

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



static size_t process_item_update(uint8_t *p, frame_t *framearrayp)
{
	itemupdatefullinfoblock_t *ibp = (itemupdatefullinfoblock_t *)p;
	if (ibp->event_type != EVENT_ITEM_UPDATE) return (0);

	int32_t framecount = (int32_t)ntohl(ibp->frame_number);

	if (framecount + FIRST_FRAME >= MAX_FRAMES) {
		DBG(printf("more frames than full 8 minutes \n"););
		return (0);
	} else if (framecount < -FIRST_FRAME) {
		DBG(printf("negative amount of frames \n"););
		return (0);
	}

	int i;

	for (i = 0;i < MAX_ITEMS;i++) if (framearrayp[framecount + FIRST_FRAME].itemupdatearray[i].valid == 0) break;

	item_update_t *itemupdatep = &(framearrayp[framecount + FIRST_FRAME].itemupdatearray[i]);

	itemupdatep->valid = true;
	itemupdatep->frame_number = framecount;
	itemupdatep->type_id = (uint16_t)ntohs(ibp->type_id);
	itemupdatep->state = ibp->state;
	itemupdatep->facing_direction = ntohf(ibp->facing_direction);
	itemupdatep->x_position = ntohf(ibp->x_position);
	itemupdatep->y_position = ntohf(ibp->y_position);
	itemupdatep->x_velocity = ntohf(ibp->x_velocity);
	itemupdatep->y_velocity = ntohf(ibp->y_velocity);
	itemupdatep->damage_taken = (uint16_t)ntohs(ibp->damage_taken);
	itemupdatep->expiration_timer = ntohf(ibp->expiration_timer);
	itemupdatep->spawn_id = (uint32_t)ntohl(ibp->spawn_id);
	itemupdatep->misc_1 = ibp->misc_1;
	itemupdatep->misc_2 = ibp->misc_2;
	itemupdatep->misc_3 = ibp->misc_3;
	itemupdatep->misc_4 = ibp->misc_4;
	itemupdatep->owner = ibp->owner;


	return itemupdatesize;
}



static size_t process_game_end(uint8_t *p, game_end_t *gameendp)
{
	gameendfullblock_t *ibp = (gameendfullblock_t *)p;
	if (ibp->event_type != EVENT_GAME_END) return (0);

	if (gameendp == NULL) goto malloc_fail;

	gameendp->game_end_method = ibp->game_end_method;
	gameendp->lras_init = ibp->lras_init;

	return gameendsize;

	malloc_fail:;
		DBG(printf("gameend malloc failed\n"););
		return (0);
}
