
typedef struct Range1f {
	float min;
	float max;
} Range1f;

typedef struct Range2f {
	Vector2 min;
	Vector2 max;
} Range2f;

inline Range2f range2f_make(Vector2 min, Vector2 max) { 
	Range2f result = {min, max};
	return result;
}

Range2f range2f_offset(Range2f range, Vector2 offset) {
	range.min = v2_add(range.min, offset);
	range.max = v2_add(range.max, offset);
	return range;
}

Range2f range2f_make_bottom_center(Vector2 size) {
	Range2f range = {};
	range.max     = size;
	range		  = range2f_offset(range, v2(size.x * -0.5, 0.0));
	return range;
}

Vector2 range2f_size(Range2f range) {
	Vector2 size = {};
	size 		 = v2_sub(range.min, range.max);
	size.x 		 = fabsf(size.x);
	size.y 		 = fabsf(size.y);
	return size;
}

bool range2f_contains(Range2f range, Vector2 vec) {
	bool result = (vec.x >= range.min.x && vec.x <= range.max.x && 
				   vec.y >= range.min.y && vec.y <= range.max.y);
	return result;
}

Vector2 range2f_get_mid(Range2f range) {
	float32 mid_distance_x = (range.max.x - range.min.x) * 0.5; 
	float32 mid_distance_y = (range.max.y - range.min.y) * 0.5; 
	Vector2 result = v2(range.min.x + mid_distance_x, range.min.y + mid_distance_y); 
	return result;
}