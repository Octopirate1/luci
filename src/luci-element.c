#include "../luci.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


static void prindent(int n);


element_t *new_element()
{
	element_t *ptr = calloc(1, sizeof(element_t));
	if (ptr != NULL) {
		ptr->namep = NULL;
		ptr->type = ET_New;
		ptr->nextp = NULL;
	}
	return (ptr);
}


array_block_t *new_array_block(void *memp, size_t count, size_t size)
{
	array_block_t *ptr = calloc(1, sizeof(array_block_t));
	if (ptr != NULL) {
		ptr->count = count;
		ptr->size = size;
		ptr->datap = memp;
	}
	return (ptr);
}


element_t *add_element(element_t *listp, element_t *newp)
{
	if (newp!=NULL) newp->nextp = listp;
	return (newp);
}


void free_array_block(array_block_t *arrayp)
{
	if (arrayp == NULL) return;
	if (arrayp->nextp != NULL) free_array_block(arrayp->nextp);
	arrayp->datap = NULL;	// DO NOT FREE - it's a pointer to mmap data
	arrayp->nextp = NULL;
	free(arrayp);
}


void free_elements(element_t *listp)
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


void element_list_dump(element_t *listp)
{
	printf("{\n");
	element_list_recurse(listp, 1);
	printf("}\n");
}


void element_dump(element_t *elemp, int indent)
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
		printf("%lld", elemp->u.integer_value);
		break;
	case ET_Float:
		printf("%lf", elemp->u.float_value);
		break;
	case ET_Array:
		printf("[%zu of size 0x%zu at %p]",
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


element_t *find_element_by_name(element_t *listp, char *namep)
{
	while (listp != NULL) {
		if (strcmp(namep, listp->namep)==0) break;
		listp = listp->nextp;
	}
	return (listp);
}


void element_list_recurse(element_t *listp, int indent)
{
	while (listp != NULL) {
		element_dump(listp, indent);
		listp = listp->nextp;
	}
}


static void prindent(int n){
  for(int i=0;i<n;i++) printf(" ");
}
