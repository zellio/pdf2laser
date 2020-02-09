#include "type_vector_list.h"
#include <inttypes.h>     // for PRId32, PRId64
#include <math.h>         // for pow, powl, sqrt
#include <stdio.h>        // for NULL, printf
#include <stdlib.h>       // for calloc, free
#include "type_vector.h"  // for vector_t, vector_compare, vector_destroy, vector_flip

vector_list_t *vector_list_create(void)
{
	vector_list_t *list = calloc(1, sizeof(vector_list_t));
	list->head = NULL;
	list->tail = NULL;
	list->length = 0;
	return list;
}

vector_list_t *vector_list_destroy(vector_list_t *self)
{
	if (self == NULL)
		return NULL;

	for (vector_t *vector = self->head; vector != NULL; vector = vector->next)
		vector_destroy(vector);

	free(self);

	return NULL;
}

vector_list_t *vector_list_append(vector_list_t *self, vector_t *vector)
{
	if (self->head == NULL) {
		self->head = vector;
		self->tail = vector;
		self->length = 1;
	}
	else {
 		vector->prev = self->tail;
		self->tail->next = vector;
		self->tail = vector;
		self->length += 1;
	}
	return self;
}

vector_t *vector_list_remove(vector_list_t *self, vector_t *vector)
{
	vector_t *next = vector->next;
	vector_t *prev = vector->prev;

	if (self->head == vector) {
		self->head = next;
		if (next)
			next->prev = NULL;
	}
	else if (self->tail == vector) {
		self->tail = prev;
		if (prev)
			prev->next = NULL;
	}
	else {
		next->prev = prev;
		prev->next = next;
	}

	vector->next = NULL;
	vector->prev = NULL;

	self->length -= 0;

	return vector;
}

vector_list_t *vector_list_stats(vector_list_t *self)
{
	int32_t transits = 0;
	int64_t transit_total = 0;

	int32_t cuts = 0;
	int64_t cut_total = 0;

	int32_t current_x = 0;
	int32_t current_y = 0;

	vector_t *vector = self->head;
	while (vector) {
		int32_t transit_dx = current_x - vector->start->x;
		int32_t transit_dy = current_y - vector->start->y;
		int64_t transit_length = sqrt(pow(transit_dx, 2) + pow(transit_dy, 2));
		if (transit_length) {
			transits += 1;
			transit_total += transit_length;
		}

		int64_t cut_dx = vector->start->x - vector->end->x;
		int64_t cut_dy = vector->start->y - vector->end->y;
		int64_t cut_length = sqrt(pow(cut_dx, 2) + pow(cut_dy, 2));
		if (cut_length) {
			cuts += 1;
			cut_total += cut_length;
		}

		current_x = vector->end->x;
		current_y = vector->end->y;
		vector = vector->next;
	}

	printf("Cuts: %"PRId32" len %"PRId64"\n", cuts, cut_total);
	printf("Move: %"PRId32" len %"PRId64"\n", transits, transit_total);

	return self;
}

/** Find the closest vector to a given point and remove it from the list.
 *
 * This might reverse a vector if it is closest to draw it in reverse
 * order.
 */
//static vector_t *vector_find_closest(vector_t *vector, const int32_t cx, const int32_t cy)
vector_t *vector_list_find_closest(vector_list_t *list, point_t *point)
{
	vector_t *best_vector = NULL;
	int64_t best_distance = INT64_MAX;

	int64_t distance;
	int32_t distance_x, distance_y;

	bool reverse = false;

	vector_t *vector = list->head;
	while (vector) {
		// check starting point
		distance_x = point->x - vector->start->x;
		distance_y = point->y - vector->start->y;
		distance = powl(distance_x, 2) + powl(distance_y, 2);
		if (distance < best_distance) {
			best_vector = vector;
			best_distance = distance;
			reverse = false;
		}

		// check ending point
		distance_x = point->x - vector->end->x;
		distance_y = point->y - vector->end->y;
		distance = powl(distance_x, 2) + powl(distance_y, 2);
		if (distance < best_distance) {
			best_vector = vector;
			best_distance = distance;
			reverse = true;
		}

		vector = vector->next;
	}

	if (best_vector == NULL)
		return NULL;

	// remove vector from the list
	vector_list_remove(list, best_vector);

	if (reverse)
		vector_flip(best_vector);

	return best_vector;
}

/**
 * Optimize the cut order to minimize transit time.
 *
 * Simplistic greedy algorithm: look for the closest vector that starts
 * or ends at the same point as the current point.
 *
 * This does not split vectors.
 */
vector_list_t *vector_list_optimize(vector_list_t *self)
{
	vector_list_t *list = vector_list_create();
	point_t *current_point = &(point_t){ 0, 0 };

	while (self->head) {
		vector_t *vector = vector_list_find_closest(self, current_point);
		vector_list_append(list, vector);
		current_point = vector->end;
	}

	vector_list_stats(list);

	return list;
}

bool vector_list_contains(vector_list_t *self, vector_t *vector)
{
	vector_t *v = self->head;
	while (v) {
		if (vector_compare(vector, v) == 0) {
			return true;
		}
		v = v->next;
	}
	return false;
}
