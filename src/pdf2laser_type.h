#ifndef __PDF2LASER_TYPE_H__
#define __PDF2LASER_TYPE_H__ 1

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef enum {
	RASTER_MODE_COLOR = 'c',
	RASTER_MODE_GREY_SCALE = 'g',
	RASTER_MODE_MONO = 'm',
	RASTER_MODE_NONE = 'n',
} raster_mode;

typedef struct _vector vector_t;
struct _vector {
	vector_t * next;
	vector_t ** prev;
	int x1;
	int y1;
	int x2;
	int y2;
	int p;
};

typedef struct _vectors vectors_t ;
struct _vectors {
	vector_t *vectors;
};


#ifdef __cplusplus
};
#endif

#endif
