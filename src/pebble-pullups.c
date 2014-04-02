#include <pebble.h>

static Window *main_win;
static Layer *sbar_lay;
char sbar_time[8];

static void pullups_tick_handler(struct tm *time, TimeUnits unit);

static void pullups_status_bar_update(Layer *lay, GContext *ctx)
{
	clock_copy_time_string(sbar_time, sizeof(sbar_time));

	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_text(ctx, sbar_time,
		fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
		(GRect){
			.origin = GPoint(0, 0),
			.size = layer_get_frame(lay).size
		},
		GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}
static void pullups_status_bar_add(Window *win)
{
	Layer *root_lay = window_get_root_layer(main_win);
	GRect bounds = layer_get_bounds(root_lay);

	sbar_lay = layer_create((GRect){
			.origin = {0, 0},
			.size = {bounds.size.w, 24}
	});
	layer_set_update_proc(sbar_lay, pullups_status_bar_update);
	layer_add_child(root_lay, sbar_lay);
	tick_timer_service_subscribe(SECOND_UNIT, pullups_tick_handler);
}
static void pullups_status_bar_del(Window *win)
{
	layer_destroy(sbar_lay);
}

static void main_window_load(Window *window)
{
	pullups_status_bar_add(window);
}

static void main_window_unload(Window *window)
{
	pullups_status_bar_del(window);
}

static void main_window_disappear(Window *window)
{
	tick_timer_service_unsubscribe();
}

static void init(void)
{
	main_win = window_create();
	window_set_fullscreen(main_win, true);
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
	if (unit == SECOND_UNIT)
		layer_mark_dirty(sbar_lay);
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
	return 0;
}
