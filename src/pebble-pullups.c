#include <pebble.h>

#define STATE_PAUSE 0
#define STATE_PLAY 1

#define DEFAULT_TIME 30

int cur_time = DEFAULT_TIME;
int cur_reps = 1;

static Window *main_win;
static ActionBarLayer *buttons;
static Layer *main_layer;
static GBitmap *play_button, *pause_button;
static int state = STATE_PAUSE;
char str_reps[16], str_time[16];

static void pullups_tick_handler(struct tm *time, TimeUnits unit);
static unsigned pullups_get_current_reps(void);
static unsigned pullups_get_current_time(void);

static void pullups_set_state()
{
	if (state == STATE_PAUSE) {
		if (cur_time == DEFAULT_TIME) cur_reps++;
		tick_timer_service_subscribe(SECOND_UNIT, pullups_tick_handler);
		action_bar_layer_set_icon(buttons, BUTTON_ID_DOWN, pause_button);
	} else {
		tick_timer_service_unsubscribe();
		action_bar_layer_set_icon(buttons, BUTTON_ID_DOWN, play_button);
	}
	state = !state;
}

static void pullups_play_pause_handler(ClickRecognizerRef rec, void *context)
{
	pullups_set_state();
}

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

static void click_config_provider(void *context)
{
	window_single_click_subscribe(BUTTON_ID_DOWN,
			(ClickHandler)pullups_play_pause_handler);
}
static void pullups_action_bar_add(Window *win)
{
	play_button = gbitmap_create_with_resource(RESOURCE_ID_PLAY_BUTTON);
	pause_button = gbitmap_create_with_resource(RESOURCE_ID_PAUSE_BUTTON);
	buttons = action_bar_layer_create();
	action_bar_layer_add_to_window(buttons, win);
	action_bar_layer_set_click_config_provider(buttons, click_config_provider);
	action_bar_layer_set_icon(buttons, BUTTON_ID_DOWN, play_button);
}
static void pullups_action_bar_del(Window *win)
{
	action_bar_layer_destroy(buttons);
	gbitmap_destroy(pause_button);
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
	cur_time = DEFAULT_TIME;
	pullups_set_state(STATE_PAUSE);
}

static void pullups_tick_handler(struct tm *time, TimeUnits unit)
{
	if (unit != SECOND_UNIT)
		return;

	pullups_one_tick();
	layer_mark_dirty(main_layer);
}

static unsigned pullups_get_current_reps(void)
{
	return cur_reps;
}
static unsigned pullups_get_current_time(void)
{
	return cur_time;
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
	return 0;
}
