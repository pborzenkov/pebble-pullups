#include <pebble.h>

static Window *main_win;
static ActionBarLayer *buttons;
static Layer *main_layer;
static GBitmap *play_button;
char str_reps[16], str_time[16];

static void pullups_tick_handler(struct tm *time, TimeUnits unit);
static unsigned pullups_get_current_reps(void);
static unsigned pullups_get_current_time(void);

static void pullups_main_layer_update(Layer *layer, GContext *ctx)
{
	unsigned time = pullups_get_current_time();

	snprintf(str_reps, sizeof(str_reps),
			"reps: %u\n", pullups_get_current_reps());
	snprintf(str_time, sizeof(str_time),
			"%02u:%02u\n", time / 60, time % 60);

	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_text(ctx, str_reps,
		fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
		(GRect){
			.origin = GPoint(10, 10),
			.size = layer_get_frame(layer).size,
		},
		GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, str_time,
		fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS),
		(GRect){
			.origin = GPoint(10, 50),
			.size = layer_get_frame(layer).size,
		},
		GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void pullups_action_bar_add(Window *win)
{
	play_button = gbitmap_create_with_resource(RESOURCE_ID_PLAY_BUTTON);
	buttons = action_bar_layer_create();
	action_bar_layer_add_to_window(buttons, win);
	action_bar_layer_set_icon(buttons, BUTTON_ID_DOWN, play_button);
}
static void pullups_action_bar_del(Window *win)
{
	action_bar_layer_destroy(buttons);
	gbitmap_destroy(play_button);
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

static void main_window_disappear(Window *window)
{
	tick_timer_service_unsubscribe();
}

static void init(void)
{
	main_win = window_create();
	window_set_window_handlers(main_win, (WindowHandlers){
			.load = main_window_load,
			.unload = main_window_unload,
			.disappear = main_window_disappear,
	});
	window_stack_push(main_win, true);
}

static void deinit(void)
{
	window_destroy(main_win);
}

static void pullups_tick_handler(struct tm *time, TimeUnits unit)
{
}

static unsigned pullups_get_current_reps(void)
{
	return 0;
}
static unsigned pullups_get_current_time(void)
{
	return 0;
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
	return 0;
}
