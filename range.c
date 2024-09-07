
typedef struct Range1f {
	float min;
	float max;
} Range1f;

typedef struct Range2f {
	Vector2 min;
	Vector2 max;
} Range2f;

inline Range2f range2f_make(Vector2 min, Vector2 max) { return (Range2f) {min, max}; }

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