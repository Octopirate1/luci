#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <alloca.h>
#include <errno.h>
#include "endianag.h"

#include "../luci.h"


	//
	// This is a two step parse, but fast.
	// The first is to unpack a UBJSON binary file
	// The second step is to extract the contents of the "raw" element in its uppermost
	// object.
	//


static slp_file_t *process_file(void *memp, size_t size);	// returns false on error
static size_t process_ubjson_value(void *memp, size_t offset, element_t *elemp);
static size_t process_ubjson_string(void *memp, size_t offset, char **strpp);
static element_t *process_ubjson_object(void *memp, size_t *offsetp);
static array_block_t *process_ubjson_array(void *memp, size_t *offsetp);
static size_t process_ubjson_true(void *memp, size_t offset);
static size_t process_ubjson_false(void *memp, size_t offset);
static size_t process_ubjson_noop(void *memp, size_t offset);
static size_t process_ubjson_string_name(void *memp, size_t offset, char **strpp);
static size_t process_ubjson_integer_value(void *memp, size_t offset, int64_t *bufp);
static metadata_t process_metadata_struct(element_t *listp);


size_t filelen;
int versionctrl[3];


slp_file_t *map_and_process(char *filenamep, int *versionp)
{
	memcpy(&versionctrl, versionp, sizeof(int)*(VERSION_LENGTH-1));
	slp_file_t *slpfilep = NULL;
	size_t ret = 0;
	struct stat sb;
	int res;
	int fh = res = open(filenamep, O_RDONLY);
	if (fh > 0) {
		res = fstat(fh, &sb);
		if (res >= 0 && (sb.st_mode & S_IFREG)) {
			ret = sb.st_size;
			filelen = (size_t)ret;

			void *memp = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fh, 0LL);
			if (memp != MAP_FAILED) {
				slpfilep = process_file(memp, sb.st_size);
			} else {
				res = -1;
			}
		}
	}
	if (res < 0) {
		printf( "Error: %d : %s\n", res, strerror(res));
		printf( "Failed to open : %s\n", filenamep);
	}
	close(fh);

	return slpfilep;
}


static slp_file_t *process_file(void *memp, size_t size)
{

	size_t offset = 0;
	game_t *gamep = NULL;
	slp_file_t *slpfilep = (slp_file_t *)malloc(sizeof(slp_file_t));

	element_t *listp = process_ubjson_object(memp, &offset); // begin metadata processing

	DBG(element_list_dump(listp););

	metadata_t md = process_metadata_struct(listp);

	fflush(stdout);
	element_t *raw_datap = find_element_by_name(listp, "raw");
	if (raw_datap == NULL) {
		printf("Unable to find the \"raw\" data block\n");
	} else {
		array_block_t *rawp = raw_datap->u.arrayp;
		if (rawp->size != 1) {
			printf("Raw data should be of byte length\n");
		} else {
			gamep = process_raw_data(rawp->datap, rawp->count, md, versionctrl); // where the real magic happens
		}
	}

	slpfilep->gamep = gamep;
	slpfilep->metadatap = listp;

	return slpfilep;
}


static element_t *process_ubjson_object(void *memp, size_t *offsetp)
{
	uint8_t *p = (uint8_t*)memp;
	size_t start_offset = *offsetp;
	size_t offset = start_offset;
	int type = p[offset];

	DBG(printf("Object offset=0x%lx : %c\n", offset, type););

	if (type != '{') {
		DBG(printf("Not an object\n"););
		return (NULL);
	}

	element_t *elemlistp = NULL;

	offset ++;
	do {

		if (offset >= filelen) {
			printf("file is corrupted, aborting program \n");
			return 0;
		}

		type = p[offset];

		if (type == '}') {
			DBG(printf("end of object\n"););
			offset ++;
			break;
		}

		element_t *elemp = new_element();
		elemlistp = add_element(elemlistp, elemp);

		DBG(printf("Object - get name (0x%lx)\n", offset););
		size_t string_size = process_ubjson_string_name(memp, offset, &(elemp->namep));
		if (string_size == 0) {
			DBG(printf("Unable to extract object name - corrupt data file?\n"););
			goto fail;
		}

		offset += string_size;
		DBG(printf("Object - get value (0x%lx)\n", offset););
		size_t value_size = process_ubjson_value(memp, offset, elemp);
		if (value_size == 0) {
			DBG(printf("Unable to extract object value - corrupt data file?\n"););
			goto fail;
		}

		offset += value_size;
	} while (true);

	DBG(fflush(stdout););
	*offsetp = offset;
	return (elemlistp);

fail:;
	free_elements(elemlistp);
	return (NULL);
}


static array_block_t *process_ubjson_array(void *memp, size_t *offsetp)
{
	uint8_t *p = (uint8_t*)memp;
	size_t offset = *offsetp;
	int type = p[offset];
	size_t element_size = 0;
	char element_type = '\0';
	int64_t element_count = 0;
	array_block_t *array_listp = NULL;

	DBG(printf("array offset=0x%lx : %c\n", offset, type););

	if (type != '[') {
		DBG(printf("Not an array\n"););
		goto done;
	}

	// size_t index = 0;
	// size_t start_offset = offset;
	offset ++;
	while (true) {

		if (offset >= filelen) {
			printf("file is corrupted, aborting program \n");
			return 0;
		}

		type = p[offset];

		switch (type) {
		case ']':
			offset ++;
			goto done;

		case '$':
			if (element_type) {
				DBG(printf("Element type already specified\n"););
				return (0);
			}

			offset++;
			element_type = p[offset];
			offset++;
			DBG(printf("all same type: %c\n", element_type););
				// array elements ...
				// complicated based on type ..
			switch (element_type) {
			case 'i':
			case 'U':
				element_size = 1;
				break;
			case 'I':
				element_size = 2;
				break;
			case 'l':
				element_size = 4;
				break;
			case 'L':
				element_size = 8;
				break;
			case 'd':
				element_size = 4;
				break;
			case 'D':
				element_size = 8;
				break;
			case 'H':
				element_size = 16;
				break;
			default:
				DBG(printf("per-element type fail\n"););
				return (0);
				break;
			}
			break;

		case '#':
			if (element_count) {
				DBG(printf("Element count already specified\n"););
				return (0);
			}
			offset++;
			size_t size = process_ubjson_integer_value(memp, offset, &element_count);
			DBG(printf("%d elements\n", (int)element_count););
			offset+=size;
			break;

		default:
			goto extract_elements;
		}
	}

extract_elements:;
		// array elements ...
		// complicated based on type ..
	if (element_type && element_count) {
		size_t total = element_size * element_count;

		DBG(printf("array bytes = %ld\n", (element_size * element_count)););

		array_listp = new_array_block(&(p[offset]), element_count, element_size);
		offset += total;
		goto done;
	}

	DBG(printf("don't yet know how to extract variable sized objects\n"););
	exit(1);

done:;
	DBG(fflush(stdout););
	*offsetp = offset;
	return (array_listp);
}


static size_t process_ubjson_integer_value(void *memp, size_t offset, int64_t *bufp)
{
	uint8_t *p = (uint8_t*)memp;
	p += offset;

	int type = p[0];
	int64_t value;

	DBG(printf("integer offset=0x%lx\n", offset););
	size_t size = 0;
	p+=1;
	switch (type) {
		case 'i':	DBG(printf("int8\n"););
			size = 1+1;
			value = (int64_t)(*(int8_t*)p);
			break;
		case 'U':	DBG(printf("uint8\n"););
			size = 1+1;
			value = (int64_t)(*(uint8_t*)p);
			break;
		case 'I':	DBG(printf("int16\n"););
			size = 1+2;
			value = (int64_t)ntohs(*(int16_t*)p);
			break;
		case 'l':	DBG(printf("int32\n"););
			size = 1+4;
			value = (int64_t)ntohl(*(int32_t*)p);
			break;
		case 'L':	DBG(printf("int64\n"););
			size = 1+8;
			value = (int64_t)ntohll(*(int64_t*)p);
			// value = (int64_t)be64toh(*(int64_t*)p); //crash2 and crash3 return very high values here, investigate later
			break;
		default:
			DBG(printf("Not an integer type\n"););
			size = 0;
			value = -1;
			break;
	}

	if (bufp) *bufp = value;

	DBG(fflush(stdout););
	return (size);
}


static size_t process_ubjson_string(void *memp, size_t offset, char **strpp)
{
	uint8_t *p = memp;

	if (p[offset]!='S') {
		DBG(printf("*** string not present\n"););
		return (0);
	}

	size_t size = process_ubjson_string_name(memp, offset+1, strpp);
	if (size <= 0) return (size);
	return (size+1);
}


static size_t process_ubjson_string_name(void *memp, size_t offset, char **strpp)
{
	int64_t value;
	size_t size = process_ubjson_integer_value(memp, offset, &value);

	if (size<=0) {
		DBG(printf("Unable to extract length of string object name - corrupt data file?\n"););
		return (0);
	}

	size_t len = (size_t)value;
	DBG(printf("String len = %ld\n", len););
	char * buffer;

	if (strpp) {
		buffer = malloc(len+1);
		if (buffer == NULL) {
			DBG(printf("Malloc failed : %s\n", strerror(errno)););
			return (0);
		}
		*strpp = buffer;
	} else {
		buffer = alloca(len+1);
	}
	uint8_t *bytesp = memp;
	memcpy(buffer, &bytesp[offset+size], len);
	buffer[len] = '\0';
	DBG(printf("String name = %s\n", buffer););
	size += len;

	return (size);
}


static size_t process_ubjson_noop(void *memp, size_t offset)
{
	return (1);
}


static size_t process_ubjson_true(void *memp, size_t offset)
{
	return (1);
}


static size_t process_ubjson_false(void *memp, size_t offset)
{
	return (1);
}


static size_t process_ubjson_value(void *memp, size_t offset, element_t *elemp)
{
	uint8_t *p = (uint8_t*)memp;
	p += offset;

	int type = p[0];

	DBG(printf("offset=0x%lx\n", offset););
	size_t size = 0;
	switch (type) {
		case 'Z':	DBG(printf("Null\n"););
				// FIXME: is this what to do with a null ??
			size = process_ubjson_noop(memp, offset);
			break;
		case 'N':	DBG(printf("No-op\n"););
			size = process_ubjson_noop(memp, offset);
			break;
		case 'T':	DBG(printf("True\n"););
			size = process_ubjson_true(memp, offset);;
			break;
		case 'F':	DBG(printf("False\n"););
			size = process_ubjson_false(memp, offset);;
			break;
		case 'i':	DBG(printf("int8\n"););
				// dump the value for the moment
			elemp->type = ET_Int;
			size = process_ubjson_integer_value(memp, offset, &(elemp->u.integer_value));
			break;
		case 'U':	DBG(printf("uint8\n"););
			elemp->type = ET_Int;
			size = process_ubjson_integer_value(memp, offset, &(elemp->u.integer_value));
			break;
		case 'I':	DBG(printf("int16\n"););
			elemp->type = ET_Int;
			size = process_ubjson_integer_value(memp, offset, &(elemp->u.integer_value));
			break;
		case 'l':	DBG(printf("int32\n"););
			elemp->type = ET_Int;
			size = process_ubjson_integer_value(memp, offset, &(elemp->u.integer_value));
			break;
		case 'L':	DBG(printf("int64\n"););
			elemp->type = ET_Int;
			size = process_ubjson_integer_value(memp, offset, &(elemp->u.integer_value));
			break;
		case 'd':	DBG(printf("float32\n"););
			size = 1+4;
			break;
		case 'D':	DBG(printf("float64\n"););
			size = 1+8;
			break;
		case 'H':	DBG(printf("high-precision!!!\n"););
			size = 1+16;
			break;
		case 'C':	DBG(printf("character\n"););
			size = 1+1;
			break;
		case 'S':	DBG(printf("string\n"););
			size = process_ubjson_string(memp, offset, &(elemp->u.stringp));
			elemp->type = ET_String;
			break;
		case '[':	DBG(printf("array\n"););
			{
			size_t idx = offset;
			array_block_t *arrayptr;
			arrayptr = process_ubjson_array(memp, &idx);
			size = idx - offset;	// FIXME
			elemp->type = ET_Array;
			elemp->u.arrayp = arrayptr;
			}
			break;
		case ']':	DBG(printf("end array\n"););
			size = 1;
			break;
		case '{':	DBG(printf("object\n"););
			{
			size_t idx = offset;
			element_t *objp = process_ubjson_object(memp, &idx);
			size = idx - offset;	// FIXME
			elemp->type = ET_Object;
			elemp->u.object_listp = objp;
			}
			break;
		case '}':	DBG(printf("end object\n"););
			size = 1;
			break;
		default:
			DBG(printf("Unkown type\n"););
			size = -1;
			break;
	}

	DBG(fflush(stdout););
	return (size);
}


static metadata_t process_metadata_struct(element_t *listp)
{
	DBG(printf("Processing metadata struct...\n"););
	metadata_t md;
	element_t *inmdp = listp->u.object_listp; // enter metadata element to find as finding is not recursive

	md.lastFrame = (int32_t)find_element_by_name(inmdp, "lastFrame")->u.integer_value;

	element_t *players = find_element_by_name(inmdp, "players");
	element_t *playerobjp = players->u.object_listp;

	// counting ports
	size_t portcount = 0;
	while (playerobjp != NULL) {
		portcount++;
		playerobjp = playerobjp->nextp;
	}

	md.portNum = portcount;

	return md;
}
