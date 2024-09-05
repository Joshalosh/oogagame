
int entry(int argc, char **argv) {
	
	// This is how we (optionally) configure the window.
	// To see all the settable window properties, ctrl+f "struct Os_Window" in os_interface.c
	window.title = fixed_string("Abyx game");
	window.width = 1280;
	window.height = 720;
	window.x = 200;
	window.y = 200;
	window.clear_color = hex_to_rgba(0x2a2d3aff);

	Gfx_Image *player = load_image_from_disk(fixed_string("player.png"), get_heap_allocator());
	assert(player, "Can't find the player image");

	Vector2 player_pos 	= v2(0, 0);
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

		player_pos = v2_add(player_pos, v2_mulf(input_axis, 100.0f * delta_t));
		
		
		Vector2 player_size = v2(16.0, 16.0);
		Matrix4 rect_xform = m4_scalar(1.0);
		rect_xform         = m4_translate(rect_xform, v3(player_pos.x, player_pos.y, 0));
		rect_xform         = m4_translate(rect_xform, v3(player_size.x * -0.5, 0, 0));
		draw_image_xform(player, rect_xform, player_size, COLOR_WHITE);
		
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