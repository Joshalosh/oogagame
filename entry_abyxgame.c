
#include "range.c"

#define INVENTORY_BG_COL v4(0, 0, 0, 0.5)
#define TILE_WIDTH       16
#define TILE_HEIGHT      16
#define CLICK_RADIUS     32.0
#define PICKUP_RADIUS 	 40.0
#define ROCK_HP 	     3
#define TREE_HP 	  	 3
#define FONT_HEIGHT      48
#define SCREEN_WIDTH     240.0
#define SCREEN_HEIGHT    135.0
#define UI_SORT_LAYER    20
#define WORLD_SORT_LAYER 10
#define MAX_INGREDIENTS  8

//
// Generic Utilities
//

Draw_Quad quad_in_screen_space(Draw_Quad quad) {
	// NOTE: I Assume these matricies are needed for screen space
	Matrix4 proj               = draw_frame.projection;
	Matrix4 view 			   = draw_frame.camera_xform;

	Matrix4 screen_space_xform = m4_scalar(1.0);
	screen_space_xform 		   = m4_mul(screen_space_xform, m4_inverse(proj));
	screen_space_xform 		   = m4_mul(screen_space_xform, view);

	quad.bottom_left 		   = m4_transform(screen_space_xform, v4(v2_expand(quad.bottom_left), 0, 1)).xy;
	quad.bottom_right 		   = m4_transform(screen_space_xform, v4(v2_expand(quad.bottom_right), 0, 1)).xy;
	quad.top_left              = m4_transform(screen_space_xform, v4(v2_expand(quad.top_left), 0, 1)).xy;
	quad.top_right 	   		   = m4_transform(screen_space_xform, v4(v2_expand(quad.top_right), 0, 1)).xy;
	return quad;
}

// NOTE: this sin_bob function goes between 0 and 1, when normally
// without intervention it would go between -1 and 1
float sin_bob(float time, float rate) {
	float result = (sin(time * rate) + 1) / 2.0;
	return result;
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

Range2f quad_to_range(Draw_Quad *quad) {
	Range2f result = {quad->bottom_left, quad->top_right};
	return result;
}

//
//
//

int world_pos_to_tile_pos(float world_pos){
	int result = roundf(world_pos / (float)TILE_WIDTH);
	return result;
}

float tile_pos_to_world_pos(int tile_pos) {
	float result = ((float)tile_pos * (float)TILE_WIDTH);
	return result;
}

Vector2 round_v2_to_tile(Vector2 world_pos){
	world_pos.x = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.x));
	world_pos.y = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.y));
	return world_pos;
}

// :Sprite
typedef enum Sprite_ID {
	SpriteID_none,
	SpriteID_player,
	SpriteID_tree0,
	SpriteID_tree1,
	SpriteID_rock0,
	SpriteID_stone,
	SpriteID_wood,
	SpriteID_oven,
	SpriteID_alter,
	SpriteID_count,
} Sprite_ID;
typedef struct Sprite {
	Gfx_Image *image;
} Sprite;
Sprite sprites[SpriteID_count];

Sprite *get_sprite_from_sprite_id(Sprite_ID id) {
	Sprite *result = &sprites[0];
	if (id >= 0 && id < SpriteID_count) {
		result = &sprites[id];
		if (!result->image) {
			result = &sprites[0];
		}
	}
	return result;
}

Vector2 get_sprite_size(Sprite *sprite) {
	Vector2 result = v2(sprite->image->width, sprite->image->height);
	return result;
}

typedef enum Resource_ID {
	ResourceID_none,
	ResourceID_stone,
	ResourceID_wood,
	ResourceID_count,
} Resource_ID;

typedef struct Inventory_Resource_Data {
	int count;
} Inventory_Resource_Data;

typedef struct Resource_Count {

} Resource_Count;

typedef enum Entity_Type {
	EntityType_none,
	EntityType_tree,
	EntityType_rock,
	EntityType_player,
	EntityType_resource,
	EntityType_oven,
	EntityType_alter,
	EntityType_count,
} Entity_Type;

typedef struct Entity {
	bool is_valid;
	Entity_Type type;
	Resource_ID resource_id;
	Vector2 pos;
	//bool render_sprite;
	Sprite_ID sprite_id;
	int health;
	bool destructable;
	bool is_resource;
} Entity;

// :resource
typedef struct Resource_Data {
	string name;
	Entity_Type type;
	Resource_Count recipes[MAX_INGREDIENTS];
} Resource_Data;
Resource_Data resource_data[ResourceID_count];
Resource_Data get_resource_from_id(Resource_ID id) {
	Resource_Data result = resource_data[id];
	return result;
}

#if 0
int get_recipe_count(Resource_Data resource_data) {
	int count = 0;
	for (int index = 0; index < MAX_INGREDIENTS; index++) {
		if (recipe_data.recipes[index].id == 0) {
			break;
		}
		count += 1;
	}
	return count;
}
#endif

// :structure
typedef enum Structure_ID{
	StructureID_none,
	StructureID_oven,
	StructureID_alter,
	StructureID_count,
} Structure_ID;
typedef struct Structure_Data {
	Entity_Type entity_type;
	Sprite_ID sprite_id;
} Structure_Data;
Structure_Data structures[StructureID_count];
Structure_Data *get_structure_from_structure_id(Structure_ID id) {
	Structure_Data *result = &structures[id];
	return result;
}

// :mode
typedef enum Game_Mode {
	GameMode_none,
	GameMode_inventory,
	GameMode_structures,
	GameMode_place,
} Game_Mode;

#define MAX_ENTITY_COUNT 1024
typedef struct World {
	Entity 		  entities[MAX_ENTITY_COUNT];
	Inventory_Resource_Data inventory[ResourceID_count];
	Game_Mode 	  game_mode;
	float 		  inventory_alpha;
	float  		  inventory_target_alpha;
	float 		  structure_alpha;
	float 		  structure_target_alpha;
	Structure_ID  structure_id;
} World;
World *world = 0;

typedef struct World_Frame {
	Entity *selected_entity;
	Matrix4 projection_space;
	Matrix4 view_space;
	bool 	hover_consumed;
} World_Frame;
World_Frame world_frame;

Sprite_ID get_sprite_id_from_resource_id(Resource_ID id) {
	switch (id) {
		case ResourceID_wood:  return SpriteID_wood;  break;		
		case ResourceID_stone: return SpriteID_stone; break;		
		default: 			   return SpriteID_none;  break;
	}
}

string entity_type_name(Entity_Type type) {
	switch (type) {
		case EntityType_oven:  return STR("Oven");  break;
		case EntityType_alter: return STR("Alter"); break;
		default: 			   return STR("None"); break;
	}
}

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

void entity_destroy(Entity *entity) {
	memset(entity, 0, sizeof(Entity));
}

// :Init structs
void player_init(Entity *entity) {
	entity->type 	  	 = EntityType_player;
	entity->pos 	     = v2(0,0);
	entity->sprite_id 	 = SpriteID_player;
	entity->destructable = false;
	entity->is_resource	 = false;
}

void oven_init(Entity *entity) {
	entity->type 		 = EntityType_oven;
	entity->sprite_id    = SpriteID_oven;
}

void alter_init(Entity *entity) {
	entity->type 		 = EntityType_alter;
	entity->sprite_id    = SpriteID_alter;
}

void rock_init(Entity *entity) {
	entity->type 	  	 = EntityType_rock;
	entity->sprite_id    = SpriteID_rock0;
	entity->health    	 = ROCK_HP;
	entity->destructable = true;
	entity->is_resource  = false;
}

void tree_init(Entity *entity) {
	entity->type 	  	 = EntityType_tree;
	entity->sprite_id 	 = SpriteID_tree0;
	entity->health    	 = TREE_HP;
	entity->destructable = true;
	entity->is_resource  = false;
}

void resource_init(Entity *entity, Resource_ID id) {
	entity->type 		 = EntityType_resource;
	entity->sprite_id    = get_sprite_id_from_resource_id(id);
	entity->is_resource  = true;
	entity->resource_id  = id;
}


void entity_init(Entity *entity, Entity_Type type) {
	switch(type) {
		case EntityType_oven:  oven_init(entity);  break;
		case EntityType_alter: alter_init(entity); break;
		default: log_error("missing entity_init case entry"); break;
	}
}

Vector2 mouse_pos_in_ndc() {
	float mouse_x 		= input_frame.mouse_x;
	float mouse_y 		= input_frame.mouse_y;
	float window_width  = window.width;
	float window_height = window.height;

	// NOTE: Normalise the mouse coordinate
	float ndc_x = (mouse_x / (window_width  * 0.5)) - 1.0f;
	float ndc_y = (mouse_y / (window_height * 0.5)) - 1.0f;

	Vector2 result = v2(ndc_x, ndc_y);
	return result;
}

Vector2 mouse_screen_to_world() {
	Matrix4 proj  		  = draw_frame.projection;
	Matrix4 view  		  = draw_frame.camera_xform;
	Vector2 mouse_ndc_pos = mouse_pos_in_ndc();

	// NOTE: Transform the world coordinates
	Vector4 world_pos = v4(mouse_ndc_pos.x, mouse_ndc_pos.y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);

	// NOTE: Return as 2D vector
	Vector2 result = v2(world_pos.x, world_pos.y);
	return result;
}

void set_screen_space() {
	draw_frame.camera_xform = m4_scalar(1.0);
	draw_frame.projection 	= m4_make_orthographic_projection(0.0, SCREEN_WIDTH, 0.0, SCREEN_HEIGHT, -1, 10);
}

void set_world_space() {
	draw_frame.camera_xform = world_frame.view_space;
	draw_frame.projection 	= world_frame.projection_space;
}

void draw_ui(Gfx_Font *font, float64 delta_t) {
	set_screen_space();

	push_z_layer(50);

	// Inventory UI
	{
		if (is_key_just_pressed(KEY_TAB)) {
			consume_key_just_pressed(KEY_TAB);
			world->game_mode = world->game_mode == GameMode_inventory ? GameMode_none : GameMode_inventory;
		}

		world->inventory_target_alpha = world->game_mode == GameMode_inventory ? 1.0 : 0.0;
		animate_f32_to_target(&world->inventory_alpha, world->inventory_target_alpha, delta_t, 15.0);

		bool inventory_enabled        = world->inventory_target_alpha == 1.0;
		if (world->inventory_target_alpha != 0.0)
		{
			Vector2 element_size 		   = v2(16, 16);
			float padding 				   = 4.0;

			#define INVENTORY_BAR_COUNT 8

			float inventory_width = INVENTORY_BAR_COUNT * element_size.x;

			float inventory_pos_x = (SCREEN_WIDTH/2) - (inventory_width/2);
			float inventory_pos_y = 30.0f;
			
			// Inventory bar rendering
			{
				Matrix4 xform = m4_scalar(1.0);
				xform		  = m4_translate(xform, v3(inventory_pos_x, inventory_pos_y, 0));
				draw_rect_xform(xform, v2(inventory_width, element_size.y), INVENTORY_BG_COL);
			}

			int element_count = 0;
			for (int resource_type = 0; resource_type < ResourceID_count; resource_type++) {
				Resource_Data resource_data       = get_resource_from_id(resource_type);
				Inventory_Resource_Data *resource = &world->inventory[resource_type];
				if (resource->count > 0) {
					float new_element_offset = element_count * element_size.x; 

					Matrix4 xform = m4_scalar(1.0);
					xform		  = m4_translate(xform, v3(inventory_pos_x + new_element_offset, inventory_pos_y, 0));

					float is_element_hovered = 0.0;

					Draw_Quad *quad = draw_rect_xform(xform, element_size, v4(1, 1, 1, 0.2));
					Range2f element_range = quad_to_range(quad);
					Vector2 mouse_ndc_pos = mouse_pos_in_ndc();
					if (inventory_enabled && range2f_contains(element_range, mouse_ndc_pos)) {
						is_element_hovered = 1.0;
					}

					Matrix4 element_bottom_right_xform = xform;

					xform 		   = m4_translate(xform, v3(element_size.x/2, element_size.y/2, 0));
					if (is_element_hovered == 1.0) {
						{
							// TODO: Juice dat inventory selection yo 
							float scale_adjust = 0.3;//0.1 * sin_bob(now, 20.0f);
							xform			   = m4_scale(xform, v3(1+scale_adjust, 1+scale_adjust, 0));
						}
#if 0
						{
							// NOTE: could also rotate?
							float32 rotate_adjust = (((PI32/4)) * sin_bob(now, 1.0f)) - PI32/4;
							xform 				  = m4_rotate_z(xform, rotate_adjust);
						}
#endif
					}

					Sprite *sprite = get_sprite_from_sprite_id(get_sprite_id_from_resource_id(resource_type));
					xform 		   = m4_translate(xform, v3(-get_sprite_size(sprite).x/2, -get_sprite_size(sprite).y/2, 0));

					draw_image_xform(sprite->image, xform, get_sprite_size(sprite), COLOR_WHITE);

					//draw_text_xform(font, STR("5"), FONT_HEIGHT, element_bottom_right_xform, v2(0.1, 0.1), COLOR_WHITE);

					// Tooltip
					if (is_element_hovered == 1.0f)
					{
						Draw_Quad screen_quad = quad_in_screen_space(*quad);
						Range2f screen_range  = quad_to_range(&screen_quad);
						Vector2 element_mid   = range2f_get_mid(screen_range);

						Vector2 tooltip_size  = v2(30, 12.5);
						Matrix4 tooltip_xform = m4_scalar(1.0);

						//float32 tooltip_start_offset = (-element_size.x*0.5) - (element_size.x*element_count);

						tooltip_xform = m4_translate(tooltip_xform, v3(element_mid.x, element_mid.y, 0));
						tooltip_xform = m4_translate(tooltip_xform, v3(-element_size.x*0.5, -tooltip_size.y - element_size.y*0.5, 0));

						draw_rect_xform(tooltip_xform, tooltip_size, INVENTORY_BG_COL);

	
						Vector2 draw_pos = element_mid;
						{
							string title = resource_data.name;
							Gfx_Text_Metrics metrics = measure_text(font, title, FONT_HEIGHT, v2(0.1, 0.1));
							draw_pos = v2_sub(draw_pos, metrics.visual_pos_min);
							draw_pos = v2_add(draw_pos, v2_mul(metrics.visual_size, v2(-0.5, -0.5)));
							
							// NOTE: Use this offset in the below draw_pos if you want the text centred
							// in the middle of the tooltip box.
							//float x_offset = (tooltip_size.x * 0.5) - (element_size.x * 0.5);
							draw_pos = v2_add(draw_pos, v2(0, (-element_size.y*0.5)-4.0));

							draw_text(font, title, FONT_HEIGHT, draw_pos, v2(0.1, 0.1), COLOR_WHITE);
						}
						{
							string text = STR("x%i");
							text = sprint(get_temporary_allocator(), text, resource->count);

							Gfx_Text_Metrics metrics = measure_text(font, text, FONT_HEIGHT, v2(0.1, 0.1));
							draw_pos = v2_sub(draw_pos, metrics.visual_pos_min);
							draw_pos = v2_add(draw_pos, v2_mul(metrics.visual_size, v2(0, -1.5)));
							
							draw_text(font, text, FONT_HEIGHT, draw_pos, v2(0.1, 0.1), COLOR_WHITE);
						}
					}

					element_count++;
				}
			}
		}
	}

	// Structures UI
	{
		if (is_key_just_pressed('C')) {
			consume_key_just_pressed('C');
			world->game_mode = world->game_mode == GameMode_structures ? GameMode_none : GameMode_structures;
		}

		world->structure_target_alpha = world->game_mode == GameMode_structures ? 1.0 : 0.0;
		animate_f32_to_target(&world->structure_alpha, world->structure_target_alpha, delta_t, 15.0);
		bool structures_enabled = world->structure_target_alpha == 1.0;

		// Draw a row of icons for building structures
		if (world->structure_target_alpha) {
			int structure_count = StructureID_count - 1;
			Vector2 element_size = v2(16, 16);
			float32 padding = 4.0;
			float32 total_box_width = element_size.x * structure_count;
			total_box_width += padding * (structure_count-1);

			float32 structure_box_start_pos = (SCREEN_WIDTH*0.5) - (total_box_width*0.5);
			for (Structure_ID structure_id = 1; structure_id < StructureID_count; structure_id++) {
				float32 element_offset = (structure_id-1) * (element_size.x + padding);
				Matrix4 xform = m4_scalar(1.0);
				xform = m4_translate(xform, v3(structure_box_start_pos + element_offset, 10, 0));

				Structure_Data *structure = &structures[structure_id];
				Sprite *sprite = get_sprite_from_sprite_id(structure->sprite_id);
				
				// TODO: get this drawing the sprite with a correct stretch
				Draw_Quad *quad = draw_image_xform(sprite->image, xform, element_size, COLOR_RED);
				Range2f structure_box = quad_to_range(quad);
				if (range2f_contains(structure_box, mouse_pos_in_ndc())) {
				// TODO: Follow mouse around a lil on hover
				world_frame.hover_consumed = true;

				// NOTE: When click go into place mode
				if(is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
					consume_key_just_pressed(MOUSE_BUTTON_LEFT);
					world->structure_id = structure_id;
					world->game_mode 	= GameMode_place;
				}
				}
			}
		}

		// :Placement mode
		{
			if (world->game_mode == GameMode_place) {
				set_world_space();
				{
					Structure_Data *structure = get_structure_from_structure_id(world->structure_id);
					Sprite *sprite = get_sprite_from_sprite_id(structure->sprite_id);

					Vector2 snap_pos = mouse_screen_to_world();
					snap_pos 		 = round_v2_to_tile(snap_pos);

					Matrix4 xform = m4_scalar(1.0);
					xform = m4_translate(xform, v3(snap_pos.x, snap_pos.y, 0));

					// NOTE: This is also changed when rendering all entities so and offset
					// here is needed to counteract that
					xform = m4_translate(xform, v3(0, -TILE_WIDTH*0.5, 0));
					xform = m4_translate(xform, v3(-get_sprite_size(sprite).x * 0.5, 0, 0));

					draw_image_xform(sprite->image, xform, get_sprite_size(sprite), v4(1, 1, 1, 0.2));

					if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
						consume_key_just_pressed(MOUSE_BUTTON_LEFT);
						Entity *entity = create_entity();
						entity_init(entity, structure->entity_type);
						entity->pos = snap_pos;
						world->game_mode = GameMode_none;
					}
				}
				set_screen_space();
			}
		}
	}
	set_world_space();
	pop_z_layer();
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

	sprites[0]               	 = (Sprite){ .image=load_image_from_disk(STR("res/sprites/missing_texture.png"), get_heap_allocator()) };
	sprites[SpriteID_player] 	 = (Sprite){ .image=load_image_from_disk(STR("res/sprites/player.png"),    	 	 get_heap_allocator()) };
	sprites[SpriteID_tree0]  	 = (Sprite){ .image=load_image_from_disk(STR("res/sprites/tree00.png"),       	 get_heap_allocator()) };
	sprites[SpriteID_tree1]   	 = (Sprite){ .image=load_image_from_disk(STR("res/sprites/tree01.png"),          get_heap_allocator()) };
	sprites[SpriteID_rock0]  	 = (Sprite){ .image=load_image_from_disk(STR("res/sprites/rock00.png"),          get_heap_allocator()) };
	sprites[SpriteID_wood]  	 = (Sprite){ .image=load_image_from_disk(STR("res/sprites/item_wood00.png"),     get_heap_allocator()) };
	sprites[SpriteID_stone]  	 = (Sprite){ .image=load_image_from_disk(STR("res/sprites/item_rock00.png"),     get_heap_allocator()) };
	sprites[SpriteID_oven]   	 = (Sprite){ .image=load_image_from_disk(STR("res/sprites/oven.png"),            get_heap_allocator()) };
	sprites[SpriteID_alter]  	 = (Sprite){ .image=load_image_from_disk(STR("res/sprites/alter.png"),           get_heap_allocator()) };

	// TODO: @ship get rid of this debug code
	{
		for (Sprite_ID sprite_id = 0; sprite_id < SpriteID_count; sprite_id++) {
			Sprite *sprite = &sprites[sprite_id];
			assert(sprite->image, "Sprite image was not found");
		}

	}

	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading font arial.ttf, %d", GetLastError());

	{
		//structures[0] = ;
		structures[StructureID_oven] =  (Structure_Data){ .entity_type=EntityType_oven,  .sprite_id=SpriteID_oven  };
		structures[StructureID_alter] = (Structure_Data){ .entity_type=EntityType_alter, .sprite_id=SpriteID_alter };
	}

	// Resource data setup
	{
		resource_data[ResourceID_stone] = (Resource_Data){ .name=STR("Stone"),};
		resource_data[ResourceID_wood]  = (Resource_Data){ .name=STR("Wood"),};
	}

	// Resource testing
	// TODO: @ship get rid of this debug code
	{
		//world->inventory[ResourceID_wood].count  = 5;
		//world->inventory[ResourceID_stone].count = 5;
		Entity *entity = create_entity();
		oven_init(entity);
	}

	Entity *player_entity = create_entity();
	player_init(player_entity);

	for (int i = 0; i < 10; i++) {
		Entity *entity = create_entity();
		rock_init(entity);
		f32 random_spray_size = 150;
		f32 random_pos_x = get_random_float32_in_range(-random_spray_size, random_spray_size);
		f32 random_pos_y = get_random_float32_in_range(-random_spray_size, random_spray_size);
		entity->pos = v2(random_pos_x, random_pos_y);
		entity->pos = round_v2_to_tile(entity->pos);
		//entity->pos.y -= TILE_HEIGHT * 0.5;
	}

	for (int i = 0; i < 10; i++) {
		Entity *entity = create_entity();
		tree_init(entity);
		f32 random_spray_size = 150;
		f32 random_pos_x = get_random_float32_in_range(-random_spray_size, random_spray_size);
		f32 random_pos_y = get_random_float32_in_range(-random_spray_size, random_spray_size);
		entity->pos = v2(random_pos_x, random_pos_y);
		entity->pos = round_v2_to_tile(entity->pos);
		//entity->pos.y -= TILE_HEIGHT * 0.5;
	}

	f64 seconds_counter = 0.0;
	s32 frame_count 	= 0;

	float zoom = 5.3;//2.65; 
	Vector2 camera_pos = v2(0, 0);

	f64 last_time = os_get_elapsed_seconds();
	while (!window.should_close) {
		reset_temporary_storage();
		world_frame = (World_Frame){0};

		f64 now = os_get_elapsed_seconds();
		f64 delta_t = now - last_time;
		last_time = now;
        os_update(); 

		// :update
		draw_frame.enable_z_sorting = true;

		world_frame.projection_space = m4_make_orthographic_projection(window.width  * -0.5, window.width  * 0.5, 
																	   window.height * -0.5, window.height * 0.5, -1, 10);
		//
		// :CAMERA
		//
		{
			Vector2 target_pos = player_entity->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 15.0f);

			world_frame.view_space = m4_make_scale(v3(1, 1, 1));
			world_frame.view_space = m4_mul(world_frame.view_space, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			world_frame.view_space = m4_mul(world_frame.view_space, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));
		}
		set_world_space();
		push_z_layer(WORLD_SORT_LAYER);

		//
		// Mouse position in world space test
		//
		Vector2 mouse_world_pos = mouse_screen_to_world();
		int mouse_tile_x  = world_pos_to_tile_pos(mouse_world_pos.x);
		int mouse_tile_y  = world_pos_to_tile_pos(mouse_world_pos.y);

		draw_ui(font, delta_t);

		// :select entity
		if (!world_frame.hover_consumed) {
			f32 current_selection_distance = INFINITY;
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity *entity = &world->entities[i];
				if (entity->is_valid && entity->destructable) {
					Sprite *sprite = get_sprite_from_sprite_id(entity->sprite_id);

					int entity_tile_x = world_pos_to_tile_pos(entity->pos.x);
					int entity_tile_y = world_pos_to_tile_pos(entity->pos.y);

					f32 mouse_distance_to_entity = v2_length(v2_sub(entity->pos, mouse_world_pos));

					if (mouse_distance_to_entity < CLICK_RADIUS) {
						if (!world_frame.selected_entity || (mouse_distance_to_entity < current_selection_distance)) {
							world_frame.selected_entity = entity;
							current_selection_distance = mouse_distance_to_entity;
						}
					}
					//log("%f, %f", mouse_world_pos.x, mouse_world_pos.y);
				}
			}
		}

		//
		// Tile grid shenanigans
		//
		{
			float half_tile_width   = (float)TILE_WIDTH   * 0.5;
			float half_tile_height  = (float)TILE_HEIGHT  * 0.5;
			int player_tile_x  		= world_pos_to_tile_pos(player_entity->pos.x);
			int player_tile_y  		= world_pos_to_tile_pos(player_entity->pos.y);
			int tile_radius_x  		= 40;
			int tile_radius_y 		= 30;

			for(int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++) {
				for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++) {
					if ((x + (y % 2 == 0)) % 2 == 0) {
						float x_pos = x * TILE_WIDTH;
						float y_pos = y * TILE_HEIGHT;
						draw_rect(v2(x_pos - half_tile_width, y_pos - half_tile_height), v2(TILE_WIDTH, TILE_HEIGHT), 
								  v4(1.0, 1.0, 1.0, 0.1));
					}
				}
			}

#if 0
			draw_rect(v2(tile_pos_to_world_pos(mouse_tile_x) + negative_half_tile_width, tile_pos_to_world_pos(mouse_tile_y) + negative_half_tile_height),
						 v2(TILE_WIDTH, TILE_HEIGHT), v4(0.5, 0.5, 0.5, 0.5));
#endif
		}

		//
		// Update Entities
		//

		for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
			Entity *entity = &world->entities[i];
			if (entity->is_valid) {
				if (entity->is_resource) {
					f32 distance_to_player = v2_length(v2_sub(entity->pos, player_entity->pos));
					if (fabsf(distance_to_player) < PICKUP_RADIUS) {
						world->inventory[entity->resource_id].count += 1;
						entity_destroy(entity);
					}
				}
			}
		}

		//
		// Click to consume resources
		//
		{
			Entity *selected_entity = world_frame.selected_entity;
			if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				if (selected_entity) {
					selected_entity->health -= 1;
					if (selected_entity->health <= 0) {

						switch (selected_entity->type) {
							case EntityType_tree: {
								Entity *resource = create_entity();
								resource_init(resource, ResourceID_wood);
								resource->pos = selected_entity->pos;
							} break;

							case EntityType_rock: {
								Entity *resource = create_entity();
								resource_init(resource, ResourceID_stone);
								resource->pos = selected_entity->pos;
							} break;

							default: {
							} break;

						}
						entity_destroy(selected_entity);
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
						Sprite *sprite 		= get_sprite_from_sprite_id(entity->sprite_id);
						Matrix4 rect_xform  = m4_scalar(1.0);
						if (entity->is_resource) {
							rect_xform = m4_translate(rect_xform, v3(0, 2.0*sin_bob(now, 5.0f), 0));
						} 
						rect_xform 			= m4_translate(rect_xform, v3(0, -TILE_HEIGHT*0.5, 0));
						rect_xform          = m4_translate(rect_xform, v3(entity->pos.x, entity->pos.y, 0));
						rect_xform          = m4_translate(rect_xform, v3(sprite->image->width * -0.5, 0, 0));

						Vector4 col = COLOR_WHITE;
						if (world_frame.selected_entity == entity) {
							col = COLOR_GREEN;
						}
						draw_image_xform(sprite->image, rect_xform, get_sprite_size(sprite), col);
					} break;
				}
			}
		}
		

		// TODO: @ship get rid of this esc key functionality
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
	
#if 0
		Matrix4 rect_xform = m4_scalar(1.0);
		rect_xform         = m4_translate(rect_xform, v3(world_pos_to_tile_pos(window.width), world_pos_to_tile_pos(-window.height), 0));
		rect_xform         = m4_rotate_z(rect_xform, (f32)now);
		rect_xform         = m4_translate(rect_xform, v3(25*-0.5, 25*-0.5, 0));
		draw_rect_xform(rect_xform, v2(25, 25), COLOR_GREEN);
#endif
		
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