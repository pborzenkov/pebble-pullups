#include <pebble.h>

enum {
	ROW_TIME = 0,
	ROW_REPS,
	ROW_STEP,
};

struct setting {
	union u_ {
		struct s_ {
			int time;
			int reps;
			int step;
		} s;
		int data[3];
	} u;
};
char *names[3] = {"Interval", "Initial reps", "Step"};

static struct setting cur_setting = {{{60, 1, 1}}};
static struct setting cur_tmp_setting;
static int cur_time, cur_reps, cur_index;
static int ticking;

static Window *main_win, *settings_win, *tune_win;

static ActionBarLayer *buttons;
static Layer *main_layer, *tune_layer;
static MenuLayer *settings_layer;
static InverterLayer *inv_layer;

static GBitmap *check_button, *settings_button, *down, *up;

char str_reps[16], str_time[8];

static void pullups_tick_handler(struct tm *time, TimeUnits unit);

static void pullups_play_pause_handler(ClickRecognizerRef rec, void *context)
{
	if (ticking)
		return;
	cur_reps += cur_setting.u.s.step;
	if (cur_reps <= 0) {
		cur_reps = cur_setting.u.s.reps;
		return;
	}
	ticking = 1;
	tick_timer_service_subscribe(SECOND_UNIT, pullups_tick_handler);
}

static void pullups_settings_handler(ClickRecognizerRef rec, void *context)
{
	window_stack_push(settings_win, true);
}

static void pullups_main_layer_update(Layer *layer, GContext *ctx)
{
	snprintf(str_reps, sizeof(str_reps),
			"reps: %d\n", cur_reps);
	snprintf(str_time, sizeof(str_time),
			"%0d:%02d\n", cur_time / 60, cur_time % 60);

	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_text(ctx, str_reps,
		fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
		(GRect){
			.origin = (GPoint){10, 10},
			.size = layer_get_frame(layer).size,
		},
		GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, str_time,
		fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS),
		(GRect){
			.origin = (GPoint){10, 50},
			.size = layer_get_frame(layer).size,
		},
		GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void click_config_provider(void *context)
{
	window_single_click_subscribe(BUTTON_ID_DOWN,
			(ClickHandler)pullups_play_pause_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT,
			(ClickHandler)pullups_settings_handler);
}
static void pullups_action_bar_add(Window *win)
{
	check_button = gbitmap_create_with_resource(RESOURCE_ID_CHECK_BUTTON);
	settings_button = gbitmap_create_with_resource(RESOURCE_ID_SETTINGS_BUTTON);
	buttons = action_bar_layer_create();
	action_bar_layer_add_to_window(buttons, win);
	action_bar_layer_set_click_config_provider(buttons, click_config_provider);
	action_bar_layer_set_icon(buttons, BUTTON_ID_DOWN, check_button);
	action_bar_layer_set_icon(buttons, BUTTON_ID_SELECT, settings_button);
}
static void pullups_action_bar_del(Window *win)
{
	action_bar_layer_destroy(buttons);
	gbitmap_destroy(settings_button);
	gbitmap_destroy(check_button);
}

static void pullups_main_layer_add(Window *win)
{
	Layer *root = window_get_root_layer(win);
	GRect bound = layer_get_frame(root);

	main_layer = layer_create((GRect){
			.origin = {0, 0},
			.size = {bound.size.w - ACTION_BAR_WIDTH, bound.size.h}
	});
	layer_set_update_proc(main_layer, pullups_main_layer_update);
	layer_add_child(root, main_layer);
	layer_mark_dirty(main_layer);
}
static void pullups_main_layer_del(Window *win)
{
	layer_destroy(main_layer);
}

static void main_window_load(Window *window)
{
	pullups_action_bar_add(window);
	pullups_main_layer_add(window);
}

static void main_window_unload(Window *window)
{
	pullups_main_layer_del(window);
	pullups_action_bar_del(window);
}

static void main_window_appear(Window *window)
{
	cur_time = cur_setting.u.s.time;
	cur_reps = cur_setting.u.s.reps;
}

static void main_window_disappear(Window *window)
{
	tick_timer_service_unsubscribe();
}

static uint16_t pullups_settings_get_num_rows(struct MenuLayer *layer,
		uint16_t section, void *context)
{
	return 3;
};
static void pullups_settings_draw_raw(GContext *ctx, const Layer *layer,
		MenuIndex *index, void *context)
{
	char num[8];
	struct setting *set = context;

	if (index->row == ROW_TIME)
		snprintf(num, sizeof(num), "%0d:%02d", set->u.data[index->row] / 60,
				set->u.data[index->row] % 60);
	else
		snprintf(num, sizeof(num), "%d", set->u.data[index->row]);
	menu_cell_basic_draw(ctx, layer, names[index->row], num, NULL);
}
static void pullups_settings_select_click(struct MenuLayer *layer,
		MenuIndex *index, void *context)
{
	cur_index = index->row;
	window_stack_push(tune_win, true);
}

MenuLayerCallbacks settings_callbacks = {
	.get_num_rows = pullups_settings_get_num_rows,
	.draw_row = pullups_settings_draw_raw,
	.select_click = pullups_settings_select_click,
};

static void settings_window_load(Window *window)
{
	Layer *layer = window_get_root_layer(window);

	up = gbitmap_create_with_resource(RESOURCE_ID_UP_ARROW);
	down = gbitmap_create_with_resource(RESOURCE_ID_DOWN_ARROW);

	settings_layer = menu_layer_create(layer_get_bounds(layer));
	menu_layer_set_callbacks(settings_layer, &cur_setting, settings_callbacks);
	menu_layer_set_click_config_onto_window(settings_layer, window);

	layer_add_child(layer, menu_layer_get_layer(settings_layer));
}

static void settings_window_unload(Window *window)
{
	menu_layer_destroy(settings_layer);

	gbitmap_destroy(up);
	gbitmap_destroy(down);
}

static void pullups_tune_layer_update(Layer *layer, GContext *ctx)
{
	GSize ls;
	char text[8];
	int width = 64;

	ls = layer_get_frame(layer).size;
	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_text(ctx, names[cur_index],
		fonts_get_system_font(FONT_KEY_GOTHIC_24),
		(GRect){
			.origin = (GPoint){0, 10},
			.size = (GSize){ls.w, 28}
		},
		GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

	graphics_fill_rect(ctx,
		(GRect){
			.origin = (GPoint){ls.w / 2 - width / 2, 58},
			.size = (GSize){ width, 36 }
		}, 5, GCornersAll);

	snprintf(text, sizeof(text), "%d", cur_tmp_setting.u.data[cur_index]);
	graphics_context_set_text_color(ctx, GColorWhite);
	graphics_draw_text(ctx, text,
		fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS),
		(GRect){
			.origin = (GPoint){ls.w / 2 - width / 2, 58},
			.size = (GSize){ width, 36 }
		},
		GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	graphics_draw_bitmap_in_rect(ctx, up,
		(GRect){
			.origin = (GPoint){ls.w / 2 - 9, 42},
			.size = (GSize){ 18, 16 }
		});
	graphics_draw_bitmap_in_rect(ctx, down,
		(GRect){
			.origin = (GPoint){ls.w / 2 - 9, 96},
			.size = (GSize){ 18, 16 }
		});
}

static void tune_window_load(Window *window)
{
	Layer *layer = window_get_root_layer(window);
	tune_layer = layer_create(layer_get_frame(layer));
	inv_layer = inverter_layer_create(layer_get_frame(layer));
	layer_set_update_proc(tune_layer, pullups_tune_layer_update);
	layer_add_child(layer, tune_layer);
	layer_add_child(tune_layer, inverter_layer_get_layer(inv_layer));
}

static void tune_window_unload(Window *window)
{
	layer_destroy(inverter_layer_get_layer(inv_layer));
	layer_destroy(tune_layer);
}

static void tune_window_appear(Window *window)
{
	layer_mark_dirty(tune_layer);
}

static void pullups_tune_handler(ClickRecognizerRef rec, void *context)
{
	int step = (int)context;

	cur_tmp_setting.u.data[cur_index] += step;
	layer_mark_dirty(tune_layer);
}

static void pullups_tune_apply(ClickRecognizerRef rec, void *context)
{
	cur_setting = cur_tmp_setting;
	window_stack_pop(true);
}

static void tune_click_provider(void *context)
{
	window_set_click_context(BUTTON_ID_UP, (void *)1);
	window_set_click_context(BUTTON_ID_DOWN, (void *)-1);
	window_single_click_subscribe(BUTTON_ID_UP, pullups_tune_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, pullups_tune_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, pullups_tune_apply);
}

static void init(void)
{
	cur_time = cur_setting.u.s.time;
	cur_reps = cur_setting.u.s.reps;
	cur_tmp_setting = cur_setting;

	main_win = window_create();
	window_set_window_handlers(main_win, (WindowHandlers){
			.load = main_window_load,
			.unload = main_window_unload,
			.appear = main_window_appear,
			.disappear = main_window_disappear,
	});

	settings_win = window_create();
	window_set_window_handlers(settings_win, (WindowHandlers){
			.load = settings_window_load,
			.unload = settings_window_unload,
	});

	tune_win = window_create();
	window_set_window_handlers(tune_win, (WindowHandlers){
			.load = tune_window_load,
			.unload = tune_window_unload,
			.appear = tune_window_appear,
	});
	window_set_click_config_provider(tune_win, tune_click_provider);

	window_stack_push(main_win, true);
}

static void deinit(void)
{
	window_destroy(tune_win);
	window_destroy(settings_win);
	window_destroy(main_win);
}

static void pullups_vibe(void)
{
	switch(cur_time) {
		case 10: case 3:  case 2: case 1:
			vibes_short_pulse(); break;
		case 0:
			vibes_long_pulse(); break;
		default:
			break;
	}
}

static void pullups_one_tick()
{
	cur_time--;
	pullups_vibe();
	if (cur_time)
		return;
	tick_timer_service_unsubscribe();
	cur_time = cur_setting.u.s.time;
	ticking = 0;
}

static void pullups_tick_handler(struct tm *time, TimeUnits unit)
{
	if (unit != SECOND_UNIT)
		return;

	pullups_one_tick();
	layer_mark_dirty(main_layer);
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
	return 0;
}
