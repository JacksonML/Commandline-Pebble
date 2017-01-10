#include <pebble.h>

/*
TODO: Make the ">" its own text_layer so that it can be easily customized
      and moved up and down simply.
TODO: Make it customizable
TODO: Add more things to display
*/

Window *my_window;
TextLayer *text_layer;
TextLayer *day_layer;

// TEXT LAYER
static TextLayer *s_time_layer;
bool underlineEnabled = false;
bool minutesAreUpdating = false;
static char s_buffer[10];
static char new_buffer[13];
static char day_buffer[13];
static char new_day_buffer[13];
int lineStatus[2] = {0,0};
static bool cursor_enabled = true;

static int oldminutes = -1,oldhours = -1,newminutes = -1,newhours = -1;
static int charsChanged = 0;

static void delete_time(void *_position) {
  static int position = 0;
  position = (int)_position;
  s_buffer[6-position] = '_';
  s_buffer[7-position] = 0;
  text_layer_set_text(s_time_layer, s_buffer);
}

static void create_time(void *_position) {
  static int position = 0;
  position = 5 - (int)_position; // We subtract it from 5 because of the timing issues seen in the update_time() function
  s_buffer[6-position] = new_buffer[6-position];
  s_buffer[7-position] = '_';
  s_buffer[8-position] = 0;
  text_layer_set_text(s_time_layer, s_buffer);
}

static void create_day(void *_position){
  cursor_enabled = false;
  s_buffer[7] = 0;
  text_layer_set_text(s_time_layer, s_buffer);
  //day_buffer[0] = '>';
  static int position = 0;
  position = 9 - (int)_position;
  day_buffer[11-position] = new_day_buffer[11-position];
  day_buffer[12-position] = '_';
  day_buffer[13-position] = 0;
  text_layer_set_text(day_layer, day_buffer);
}

static void delete_day(void *_position){
  static int position = 0;
  position = (int)_position;
  day_buffer[11-position] = '_';
  day_buffer[12-position] = 0;
  text_layer_set_text(day_layer, day_buffer);
}

static void switch_to_time(){
  day_buffer[11] = 0;
  //day_buffer[0] = ' ';
  //day_buffer[1] = ' ';
  text_layer_set_text(day_layer, day_buffer);
  cursor_enabled = true;
}

static void update_day() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  strftime(new_day_buffer, sizeof(day_buffer), "   %A", tick_time);
  
  //text_layer_set_text(day_layer, day_buffer);
  for (int i = 8; i > 0; i--){ // When the watchface first opens.
      app_timer_register(1500 + 178*(i), create_day, (void*)i);
  }
  app_timer_register(3000, switch_to_time, NULL);
}

static void update_time(bool start) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  //strncpy(old_buffer,s_buffer,10);
  strftime(new_buffer, sizeof(new_buffer), clock_is_24h_style() ? "> %H:%M" : "> %I:%M", tick_time);
  
  oldminutes = newminutes;
  oldhours = newhours;
  
  newminutes = tick_time->tm_min;
  newhours = tick_time->tm_hour;
  
  charsChanged = 1;
  
  if (newminutes - oldminutes != 0) {
    charsChanged = 1;
  }
  if ((newminutes - oldminutes) / 10 != 0){
    charsChanged = 2;
  }
  if (newhours - oldhours != 0) {
    charsChanged = 3;
  }
  if ((newhours - oldhours) / 10 != 0) {
    charsChanged = 4;
  }
  if (start) {
    charsChanged = 4;
  }
  if (!start){ // If the time is changing
    for (int i = 0; i < charsChanged; i++){
      app_timer_register(200*i,delete_time, (void*)i);
    }
    for (int i = 5; i > (5-charsChanged); i--){
      app_timer_register(charsChanged * 200 + 275*i - 100, create_time, (void*)i);
    }
  } else {
    for (int i = 5; i > (4-charsChanged); i--){ // When the watchface first opens.
      app_timer_register(275*i, create_time, (void*)i);
    }
  }
}

static void update_cursor() {
  if (cursor_enabled){
    underlineEnabled = !underlineEnabled;
    if (underlineEnabled == false){
      s_buffer[7] = 0;
    }
    else if (underlineEnabled == true){
      s_buffer[7] = '_';
    }
  }
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & SECOND_UNIT){
    update_cursor();
  }
  if (units_changed & MINUTE_UNIT){
    update_time(false);
  }
  if (units_changed & DAY_UNIT){
    strftime(day_buffer, sizeof(day_buffer), "   %A", tick_time);
    text_layer_set_text(day_layer, day_buffer);
  }
}

static void main_window_load(Window *window) {
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  GRect bounds = layer_get_bounds(window_get_root_layer(my_window));
  s_time_layer = text_layer_create(GRect(0,0,bounds.size.w,bounds.size.h));
  
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorGreen);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text(s_time_layer, "> ");
  layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(s_time_layer));
  
  text_layer_set_background_color(day_layer, GColorBlack);
  text_layer_set_text_color(day_layer, GColorGreen);
  text_layer_set_font(day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text(day_layer, "   ");
  layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(day_layer));
}

static void main_window_unload(Window *window) {
  
}

void handle_init(void) {
  my_window = window_create();
  window_set_window_handlers(my_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  text_layer = text_layer_create(GRect(0, 0, 144, 20));
  day_layer = text_layer_create(GRect(0,30,144,50));
  window_stack_push(my_window, true);
  
  // Other initialization stuff.
  update_time(true);
  strncpy(s_buffer, new_buffer, 10);
  update_day();
  strncpy(day_buffer, new_day_buffer, 10);
  //strncpy(day_buffer, new_day_buffer, sizeof(day_buffer));
}

void handle_deinit(void) {
  text_layer_destroy(text_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(day_layer);
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
