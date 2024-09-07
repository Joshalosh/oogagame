
#include "range.c"

#define TILE_WIDTH  8
#define TILE_HEIGHT 8

int world_to_tile_pos(float world_pos){
	return world_pos / (float)TILE_WIDTH;

}

bool almost_equals(float a, float b, float epsilon) {
	return fabs(a - b) <= epsilon;
}

// NOTE: This is a very cool and smooth function for interpolating to a target,
// me likey. I guess it's a lerp with smoothing/damping 
bool animate_f32_to_target(float *value, float target, float delta_t, float rate) {
	if (almost_equals(*value, target, 0.001f)) {
		*value = target;
		return true; // NOTE: The target has been reached
	}
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	return false; // NOTE: The target still has not been reached
}

// NOTE: This is just a vectorised version of cool function because c doesn't
// have any operator overloading
void animate_v2_to_target(Vector2 *value, Vector2 target, float delta_t, float rate) {
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);

}

typedef enum Sprite_ID {
	SpriteID_none,
	SpriteID_player,
	SpriteID_tree0,
	SpriteID_tree1,
	SpriteID_rock0,
	SpriteID_count,
} Sprite_ID;
typedef struct Sprite {
	Gfx_Image *image;
	// TODO: Not sure if I like the fact that size is attached
	// to the sprite and not to the entity. Perhaps I should
	// move size onto the entity later if it feels more right.
	Vector2    size; 
} Sprite;
Sprite sprites[SpriteID_count];

Sprite *get_sprite(Sprite_ID id) {
	Sprite *result = &sprites[0];
	if (id >= 0 && id < SpriteID_count) {
		result = &sprites[id];
	}
	return result;
}

typedef enum Entity_Type {
	EntityType_none,
	EntityType_tree,
	EntityType_rock,
	EntityType_player,
} Entity_Type;

typedef struct Entity {
	bool is_valid;
	Entity_Type type;
	Vector2 pos;

	//bool render_sprite;
	Sprite_ID sprite_id;
} Entity;

#define MAX_ENTITY_COUNT 1024
typedef struct World {
	Entity entities[MAX_ENTITY_COUNT];
} World;
World *world = 0;

Entity *create_entity() {
	Entity *found_entity = 0;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity *existing_entity = &world->entities[i];
		if (!existing_entity->is_valid){
			found_entity = existing_entity;
			break;
		}
	}
	assert(found_entity, "No more free entity spots in array");
	found_entity->is_valid = true;
	return found_entity;
}

void destroy_entity(Entity *entity) {
	memset(entity, 0, sizeof(Entity));
}

void rock_init(Entity *entity) {
	entity->type 	  = EntityType_rock;
	entity->sprite_id = SpriteID_rock0;
}

void tree_init(Entity *entity) {
	entity->type = EntityType_tree;
	entity->sprite_id = SpriteID_tree0;
}

void player_init(Entity *entity) {
	entity->type = EntityType_player;
	entity->pos = v2(0,0);
	entity->sprite_id = SpriteID_player;
}

Vector2 mouse_screen_to_world() {
	float mouse_x 		= input_frame.mouse_x;
	float mouse_y 		= input_frame.mouse_y;
	Matrix4 proj  		= draw_frame.projection;
	Matrix4 view  		= draw_frame.camera_xform;
	float window_width  = window.width;
	float window_height = window.height;

	// NOTE: Normalise the mouse coordinates
	float ndc_x = (mouse_x / (window_width  * 0.5)) - 1.0f;
	float ndc_y = (mouse_y / (window_height * 0.5)) - 1.0f;

	// NOTE: Transform the world coordinates
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);

	// NOTE: Return as 2D vector
	return (Vector2){world_pos.x, world_pos.y};
}

int entry(int argc, char **argv) {
	
	// This is how we (optionally) configure the window.
	// To see all the settable window properties, ctrl+f "struct Os_Window" in os_interface.c
	window.title = STR("Abyx game");
	window.width = 1280;
	window.height = 720;
	window.x = 200;
	window.y = 200;
	window.clear_color = hex_to_rgba(0x2a2d3aff);

	world = alloc(get_heap_allocator(), sizeof(World));

	sprites[SpriteID_player] = (Sprite){ .image=load_image_from_disk(STR("player.png"), get_heap_allocator()), .size=v2( 8,  8) };
	sprites[SpriteID_tree0]  = (Sprite){ .image=load_image_from_disk(STR("tree00.png"), get_heap_allocator()), .size=v2(16, 16) };
	sprites[SpriteID_tree1]  = (Sprite){ .image=load_image_from_disk(STR("tree01.png"), get_heap_allocator()), .size=v2(16, 16) };
	sprites[SpriteID_rock0]  = (Sprite){ .image=load_image_from_disk(STR("rock00.png"), get_heap_allocator()), .size=v2(16, 16) };

	Entity *player_entity = create_entity();
	player_init(player_entity);

	for (int i = 0; i < 10; i++) {
		Entity *entity = create_entity();
		rock_init(entity);
		f32 random_spray_size = 150;
		f32 random_pos_x = get_random_float32_in_range(-random_spray_size, random_spray_size);
		f32 random_pos_y = get_random_float32_in_range(-random_spray_size, random_spray_size);
		entity->pos = v2(random_pos_x, random_pos_y);
	}

	for (int i = 0; i < 10; i++) {
		Entity *entity = create_entity();
		tree_init(entity);
		f32 random_spray_size = 150;
		f32 random_pos_x = get_random_float32_in_range(-random_spray_size, random_spray_size);
		f32 random_pos_y = get_random_float32_in_range(-random_spray_size, random_spray_size);
		entity->pos = v2(random_pos_x, random_pos_y);
	}

	f64 seconds_counter = 0.0;
	s32 frame_count 	= 0;

	float zoom = 5.3; 
	Vector2 camera_pos = v2(0, 0);

	f64 last_time = os_get_elapsed_seconds();
	while (!window.should_close) {
		reset_temporary_storage();

		f64 now = os_get_elapsed_seconds();
		f64 delta_t = now - last_time;
		last_time = now;
        os_update(); 

		draw_frame.projection = m4_make_orthographic_projection(window.width  * -0.5, window.width  * 0.5, 
																window.height * -0.5, window.height * 0.5, -1, 10);
		//
		// CAMERA
		//
		{
			Vector2 target_pos = player_entity->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 15.0f);

			draw_frame.camera_xform = m4_make_scale(v3(1, 1, 1));
			draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));
		}

		//
		// Mouse position in world space test
		//
		{
			Vector2 mouse_pos = mouse_screen_to_world();

			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity *entity = &world->entities[i];
				if (entity->is_valid) {
					Sprite *sprite = get_sprite(entity->sprite_id);
					Range2f bounds = range2f_make_bottom_center(sprite->size);
					bounds = range2f_offset(bounds, entity->pos);

					Vector4 color = COLOR_GREEN;
					color.a = 0.4;
					if (range2f_contains(bounds, mouse_pos)) {
						color.a = 1.0;
					}

					draw_rect(bounds.min, range2f_size(bounds), color);
				}
			}
		}

		//
		// Tile grid shenanigans
		//
		{
			int player_tile_x = world_to_tile_pos(player_entity->pos.x);
			int player_tile_y = world_to_tile_pos(player_entity->pos.y);
			int tile_radius_x = 40;
			int tile_radius_y = 30;

			for(int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++) {
				for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++) {
					if ((x + (y % 2 == 0)) % 2 == 0) {
						float x_pos = x * TILE_WIDTH;
						float y_pos = y * TILE_HEIGHT;
						draw_rect(v2(x_pos, y_pos), v2(TILE_WIDTH, TILE_HEIGHT), v4(1.0, 1.0, 1.0, 0.1));
					}
				}
			}
		}

		// NOTE: This is where we draw the entities, setup happens earlier
		// but the actual draw happens now, that's why sprite info is needed
		// here now and not earlier.
		for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
			Entity *entity = &world->entities[i];
			if (entity->is_valid) {
				switch (entity->type) {
					default: {
						Sprite *sprite 		= get_sprite(entity->sprite_id);
						Matrix4 rect_xform  = m4_scalar(1.0);
						rect_xform          = m4_translate(rect_xform, v3(entity->pos.x, entity->pos.y, 0));
						rect_xform          = m4_translate(rect_xform, v3(sprite->size.x * -0.5, 0, 0));
						draw_image_xform(sprite->image, rect_xform, sprite->size, COLOR_WHITE);
					} break;
				}
			}
		}

		if (is_key_just_pressed(KEY_ESCAPE)) {
			window.should_close = true;
		}

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A')) {
			input_axis.x = -1.0;
		}
		if (is_key_down('D')) {
			input_axis.x = 1.0;
		}
		if (is_key_down('S')) {
			input_axis.y = -1.0;
		}
		if (is_key_down('W')) {
			input_axis.y = 1.0;
		}
		input_axis = v2_normalize(input_axis);

		player_entity->pos = v2_add(player_entity->pos, v2_mulf(input_axis, 100.0f * delta_t));
		
		//draw_rect(v2(sin(now)*window.width*0.4-60, -60), v2(120, 120), COLOR_RED);
		
		gfx_update();
		seconds_counter += delta_t;
		frame_count		+= 1;
		if (seconds_counter > 1) {
			log("fps: %i", frame_count);
			seconds_counter = 0;
			frame_count = 0;
		}
	}

	return 0;
}