#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

size_t map_and_process(char *filenamep);
void process_raw_data(void *ptr, size_t len);


#ifndef NDEBUG
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

void element_list_dump(element_t *listp);
void element_dump(element_t *elemp, int indent);
void free_elements(element_t *listp);
element_t *find_element_by_name(element_t *listp, char *namep);

