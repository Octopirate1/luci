#include "luci.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static size_t process_raw_event(void *ptr, size_t offset, element_t *elemp);

void process_raw_data(void *ptr, size_t len)
{
	uint8_t *p = (uint8_t*)ptr;
	size_t offset = 0;
	int type;

	// DBG(printf("Object offset=0x%lx : %c\n", offset, type););

	element_t *elemlistp = NULL;

	// offset ++;
	do {
		type = p[offset];

		if (offset == len) {
			DBG(printf("end of object\n"););
			// offset ++;
			break;
		}

		element_t *elemp = new_element();
		elemlistp = add_element(elemlistp, elemp);

		DBG(printf("Object - get event type (0x%lx)\n", offset););
		size_t event_size = process_raw_event(ptr, offset, elemp);
		DBG(printf("event_size = %zu\n", event_size););
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

static size_t process_raw_event(void *ptr, size_t offset, element_t *elemp)
{
	uint8_t *p = (uint8_t*)ptr;
	uint16_t *payloadp = (uint16_t*)ptr;
	p += offset;

	if (p[0] != 0x35) {
		return 0;
	}

	uint8_t payload_size = p[1];
	int num_cmds = (payload_size-1)/3;
	uint8_t cmds[num_cmds];
	uint16_t payloadsizes[num_cmds];

	for(int i = 0;i < num_cmds;i++) {
		cmds[i] = p[] // make cmds[i] equal to command it needs to be, do same for payloadsizes
	}

	DBG(printf("payload_size = %d\n", payload_size););
	DBG(printf("ocb = %d\n", ocb););
	switch (ocb) {
		case 0x36:
			DBG(printf("processing Game Start\n"););
			for(int loop = 3;loop < 7;loop++) {
				DBG(printf("first %d Game Start payload bytes = %d\n", loop, p[loop]););
			}
			break;
		case 0x37:
			DBG(printf("processing Preframe bullshit"););
			break;
	}

	return 1;
}
