
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

	f64 last_time = os_get_elapsed_seconds();
	while (!window.should_close) {
		reset_temporary_storage();

		draw_frame.projection = m4_make_orthographic_projection(window.width  * -0.5, window.width  * 0.5, 
																window.height * -0.5, window.height * 0.5, -1, 10);
		float zoom = 5.3; 
		draw_frame.camera_xform = m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0));

		f64 now = os_get_elapsed_seconds();
		f64 delta_t = now - last_time;
		last_time = now;

        os_update(); 

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