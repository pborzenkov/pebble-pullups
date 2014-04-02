#include <pebble.h>

static Window *main_win;
static ActionBarLayer *buttons;
static GBitmap *play_button;

static void pullups_tick_handler(struct tm *time, TimeUnits unit);

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
}

static void main_window_load(Window *window)
{
	pullups_action_bar_add(window);
}

static void main_window_unload(Window *window)
{
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

int main(void)
{
	init();
	app_event_loop();
	deinit();
	return 0;
}
