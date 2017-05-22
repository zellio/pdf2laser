#include "pdf2laser_type.h"


vector_t *vector_create(uint32_t x1, uint32_t y1,
						uint32_t x2, uint32_t y2,
						uint32_t p)
{
	vector_t *vector = calloc(1, sizeof(vector_t));

	vector->start = calloc(1, sizeof(pair_t));
	vector->start->x = x1;
	vector->start->y = y1;

	vector->end = calloc(1, sizeof(pair_t));
	vector->end->x = x2;
	vector->end->y = y2;

	vector->p = p;

	return vector;
}


/* #ifndef __PDF2LASER_TYPE_H__ */
/* #define __PDF2LASER_TYPE_H__ 1 */

/* #ifdef __cplusplus */
/* extern "C" { */
/* #endif */
/* #if 0 */
/* } */
/* #endif */

/* typedef enum { */
/* 	COLOR = 'c', */
/* 	GREY_SCALE = 'g', */
/* 	MONO = 'm', */
/* 	NONE = 'n', */
/* } raster_mode; */

/* typedef struct pair pair_t; */
/* struct pair { */
/* 	uint32_t x; */
/* 	uint32_t y; */
/* }; */

/* typedef struct vector vector_t; */
/* struct vector { */
/* 	pair_t start; */
/* 	part_t end; */
/* 	uint32_t p; */
/* }; */

/* typedef struct list list_t; */
/* struct list { */
/* 	vector_t *vector; */
/* 	list_t *next; */
/* 	list_t *prev; */
/* }; */

/* #ifdef __cplusplus */
/* }; */
/* #endif */

/* #endif */
