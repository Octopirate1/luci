#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <alloca.h>
#include <errno.h>
#include <arpa/inet.h>	// ntohl etc. FIXME
#include <endian.h> // only for linux

#include "luci.h"

#ifndef NDEBUG
#define LUCI_DEBUG 1
#define	DBG(s)	do { s } while (0)
static void dump_mem(void *memp, size_t offset, int len);
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

static element_t *new_element();
static array_block_t *new_array_block(void *memp, size_t count, size_t size);
static element_t *add_element(element_t *listp, element_t *newp);
static void element_list_dump(element_t *listp);
static void element_list_recurse(element_t *listp, int indent);
static void element_dump(element_t *elemp, int indent);
static void free_elements(element_t *listp);
static element_t *find_element_by_name(element_t *listp, char *namep);
static void prindent(int n);


static bool_t process_file(void *memp, size_t size);	// returns false on error
static size_t process_ubjson_value(void *memp, size_t offset, element_t *elemp);
static size_t process_ubjson_string(void *memp, size_t offset, char **strpp);
//static size_t process_number(void *memp, size_t offset);
static element_t *process_ubjson_object(void *memp, size_t *offsetp);
static array_block_t *process_ubjson_array(void *memp, size_t *offsetp);
static size_t process_ubjson_true(void *memp, size_t offset);
static size_t process_ubjson_false(void *memp, size_t offset);
static size_t process_ubjson_noop(void *memp, size_t offset);
static size_t process_ubjson_string_name(void *memp, size_t offset, char **strpp);
static size_t process_ubjson_integer_value(void *memp, size_t offset, int64_t *bufp);

static void process_raw_data(void *datap, size_t size);


size_t map_and_process(char *filenamep)
{
	size_t ret = -1;
	struct stat sb;
	int res;
	int fh = res = open(filenamep, O_RDONLY);
	if (fh > 0) {
		res = fstat(fh, &sb);
		if (res >= 0 && (sb.st_mode & S_IFREG)) {
			ret = sb.st_size;
			printf("File is %ld bytes\n", (long)ret);

			void *memp = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fh, 0LL);
			if (memp != MAP_FAILED) {
				(void)process_file(memp, sb.st_size);
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

	return (ret);
}



#if LUCI_DEBUG	// {

static void dump_mem(void *memp, size_t offset, int len)
{
	uint8_t *p = memp;
	p += offset;

	for (int i = 0; i<len; i+=16) {
		printf("%08lx :", offset+i);
		char output[17];
		output[16]='\0';
		for (int j=0; j<16; j++) {
			int ch = p[i+j];
			printf(" %02x", ch);
			output[j] = (ch>=21 && ch<127) ? ch : '.';
		}
		printf(" : %s\n", output);
	}
}

#endif	// }


	//
	// returns false on error
	//
static bool_t process_file(void *memp, size_t size)
{
#if LUCI_DEBUG
	dump_mem(memp, 0L, 64);
	printf("\n");
	dump_mem(memp, 0x2b4240L, 64);
#endif

	size_t offset = 0;
	bool_t res = false;

	element_t *listp = process_ubjson_object(memp, &offset);

	element_list_dump(listp);

	element_t *raw_datap = find_element_by_name(listp, "raw");
	if (raw_datap == NULL) {
		printf("Unable to find the \"raw\" data block\n");
	} else {
		array_block_t *rawp = raw_datap->u.arrayp;
		if (rawp->size != 1) {
			printf("Discrepancy raw data should be of byte length\n");
		} else {
			process_raw_data(rawp->datap, rawp->count);
			res = true;
		}
	}

	free_elements(listp);
	return (res);
}




static void process_raw_data(void *ptr, size_t len)
{
	printf("Processing RAW data\n");
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
		if (string_size <= 0) goto fail;

		offset += string_size;
		DBG(printf("Object - get value (0x%lx)\n", offset););
		size_t value_size = process_ubjson_value(memp, offset, elemp);
		if (value_size <=0) goto fail;

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
		type = p[offset];

		switch (type) {
		case '}':
			offset ++;
			goto done;

		case '$':
			if (element_type) {
				DBG(printf("ELement type already specified\n"););
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
			// value = (int64_t)ntohll(*(int64_t*)p);
			value = (int64_t)be64toh(*(int64_t*)p);
			break;
		default:
			DBG(printf("Not an integer type\n"););
			size = -1;
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

	if (size<=0) return (0);

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


static element_t *new_element()
{
	element_t *ptr = calloc(1, sizeof(element_t));
	if (ptr != NULL) {
		ptr->namep = NULL;
		ptr->type = ET_New;
		ptr->nextp = NULL;
	}
	return (ptr);
}

static array_block_t *new_array_block(void *memp, size_t count, size_t size)
{
	array_block_t *ptr = calloc(1, sizeof(array_block_t));
	if (ptr != NULL) {
		ptr->count = count;
		ptr->size = size;
		ptr->datap = memp;
	}
	return (ptr);
}


static element_t *add_element(element_t *listp, element_t *newp)
{
	if (newp!=NULL) newp->nextp = listp;
	return (newp);
}

static void free_array_block(array_block_t *arrayp)
{
	if (arrayp == NULL) return;
	if (arrayp->nextp != NULL) free_array_block(arrayp->nextp);
	arrayp->datap = NULL;	// DO NOT FREE - it's a pointer to mmap data
	arrayp->nextp = NULL;
	free(arrayp);
}


static void free_elements(element_t *listp)
{
	if (listp == NULL) return;
	free_elements(listp->nextp);
	listp->nextp = NULL;

	switch (listp->type) {
	case ET_Object:
		free_elements(listp->u.object_listp);
		listp->u.object_listp = NULL;
		break;
	case ET_Array:
		free_array_block(listp->u.arrayp);
		listp->u.arrayp = NULL;
		break;
	case ET_String:
		free(listp->u.stringp);
		listp->u.stringp = NULL;
	default:
		break;
	}
	free(listp->namep);
	listp->namep = NULL;
	free(listp);
}

static void element_list_dump(element_t *listp)
{
	printf("{\n");
	element_list_recurse(listp, 1);
	printf("}\n");
}


static void element_list_recurse(element_t *listp, int indent)
{
	while (listp != NULL) {
		element_dump(listp, indent);
		listp = listp->nextp;
	}
}

static void element_dump(element_t *elemp, int indent)
{
	prindent(indent);
	printf("\"%s\" = ", elemp->namep);
	switch (elemp->type) {
	case ET_New:
		printf("<uninitialized>");
		break;
	case ET_Char:
		printf("\'%c\'", elemp->u.char_value);
		break;
	case ET_String:
		printf("\"%s\"", elemp->u.stringp);
		break;
	case ET_Int:
		printf("%ld", elemp->u.integer_value);
		break;
	case ET_Float:
		printf("%lf", elemp->u.float_value);
		break;
	case ET_Array:
		printf("[%ld of size 0x%ld at %p]",
			elemp->u.arrayp->count,
			elemp->u.arrayp->size,
			elemp->u.arrayp->datap);
		break;
	case ET_Object:
		printf("{\n");
		element_list_recurse(elemp->u.object_listp, indent+1);
		prindent(indent);
		printf("}");
		break;
	}
	printf("%s\n", elemp->nextp ? ",": "");
}

static void prindent(int n)
{
	for (int i=0; i<n; i++) printf("  ");
}

static element_t *find_element_by_name(element_t *listp, char *namep)
{
	while (listp != NULL) {
		if (strcmp(namep, listp->namep)==0) break;
		listp = listp->nextp;
	}
	return (listp);
}