#include <pebble.h>
#include "geometry.h"

#define HOUR_RADIUS 10
#define HOUR_THICKNESS 3
#define MINUTE_RADIUS 18
#define MINUTE_THICKNESS 3
#define SECOND_RADIUS 25
#define SECOND_THICKNESS 3
#define HMS_RADIUS 27
#define SUN 0
#define MOON 1
#define VENUS 2
#define MARS 3
#define JUPITER 4
#define SUN_THICKNESS 10
#define SUN_STROKE GColorBlack
#define SUN_FILL GColorWhite
#define MOON_THICKNESS 10
#define MOON_STROKE GColorWhite
#define MOON_FILL GColorBlack
#define VENUS_SIZE 7
#define VENUS_RADIUS 32
#define MARS_SIZE 7
#define MARS_RADIUS 63
#define JUPITER_SIZE 7
#define JUPITER_RADIUS 75

enum {
  SUNRISE_KEY = 0x01,
  SUNSET_KEY = 0x02,
  SUN_KEY = 0x03,
  MOON_KEY = 0x04,
  MARS_KEY = 0x05,
  VENUS_KEY = 0x06,
  JUPITER_KEY = 0x07,
};

static Window *window;
static Layer *timeLayer;
static Layer *astroLayer;
static Layer *rootLayer;

static int angle_45 = TRIG_MAX_ANGLE / 8;
static int angle_90 = TRIG_MAX_ANGLE / 4;
static int angle_180 = TRIG_MAX_ANGLE / 2;
static int angle_270 = 3 * TRIG_MAX_ANGLE / 4;

static bool astro_initialized = 0;
static int sun_angle;
static int moon_angle;
static int venus_angle;
static int mars_angle;
static int jupiter_angle;
static int sunrise_angle = TRIG_MAX_ANGLE * 4;
static int sunset_angle = TRIG_MAX_ANGLE * 3 / 4;

// Mars is just Venus rotated 180
static const GPathInfo VENUS_PATH_INFO = {
  .num_points = 3,
  .points = (GPoint[]) {{-VENUS_SIZE,-VENUS_SIZE*2},{VENUS_SIZE,-VENUS_SIZE*2},{0,0}}
};

static void bg_update(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}


static void draw_astro(Layer *layer, GContext *ctx) {
  if (astro_initialized) {
      // draw dawn & dusk lines (background)
      GRect bounds = layer_get_bounds(layer);
      const GPoint center = grect_center_point(&bounds);
      GPathInfo horizon_info = {
        .num_points = 5,
        .points = (GPoint []) {{bounds.origin.x, center.y - (bounds.size.w * sin_lookup(sunrise_angle) / (2 * cos_lookup(sunrise_angle)))},
                               {center.x, center.y},
                               {bounds.size.w, center.y - (bounds.size.w * sin_lookup(sunset_angle) / (2 * cos_lookup(sunset_angle)))},
                               {bounds.size.w, bounds.size.h},
                               {bounds.origin.x, bounds.size.h}
                              }
      };
      GPath *horizon = NULL;
      horizon = gpath_create(&horizon_info);
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_context_set_stroke_color(ctx, GColorBlack);
      gpath_draw_filled(ctx, horizon);
      gpath_draw_outline(ctx, horizon);

      // draw sun and moon rays
      graphics_draw_ray(ctx, layer, center, sun_angle, SUN_THICKNESS, SUN_STROKE, SUN_FILL);
      graphics_draw_ray(ctx, layer, center, moon_angle, MOON_THICKNESS, MOON_STROKE, MOON_FILL);

      // draw planets
      GPath *venus = NULL;
      GPath *mars = NULL;
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_context_set_stroke_color(ctx, GColorBlack);
      venus = gpath_create(&VENUS_PATH_INFO);
      gpath_move_to(venus, GPoint(center.x + VENUS_RADIUS * cos_lookup(venus_angle) / TRIG_MAX_RATIO,
                                  center.y + VENUS_RADIUS * sin_lookup(venus_angle) / TRIG_MAX_RATIO));
      gpath_draw_filled(ctx, venus);
      gpath_draw_outline(ctx, venus);
      mars = gpath_create(&VENUS_PATH_INFO);
      gpath_move_to(mars, GPoint(center.x + MARS_RADIUS * cos_lookup(mars_angle) / TRIG_MAX_RATIO,
                                  center.y + MARS_RADIUS * sin_lookup(mars_angle) / TRIG_MAX_RATIO));
      gpath_rotate_to(mars, TRIG_MAX_ANGLE / 2);
      gpath_draw_filled(ctx, mars);
      gpath_draw_outline(ctx, mars);

      GRect jupiter = GRect(center.x + JUPITER_RADIUS * cos_lookup(jupiter_angle) / TRIG_MAX_RATIO,
                            center.y + JUPITER_RADIUS * sin_lookup(jupiter_angle) / TRIG_MAX_RATIO,
                            JUPITER_SIZE * 2,
                            JUPITER_SIZE * 2);
      graphics_fill_rect(ctx, jupiter, 0, 0);
      graphics_draw_rect(ctx, jupiter);
  }
}

static void draw_time(Layer *layer, GContext *ctx) {
  // draw the time in an analog clock at the center
  GRect bounds = layer_get_bounds(layer);
  const GPoint center = grect_center_point(&bounds);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int secondMarkers = t->tm_sec / 2;
  int i;

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, HMS_RADIUS);
  graphics_context_set_fill_color(ctx, GColorBlack);
  // draw the seconds
  if (secondMarkers % 30 != 0) {
    if (secondMarkers <= 15) {
      for (i=1; i<=secondMarkers; i++) {
        int angle = i * 4;
        GPoint marker = GPoint(center.x + SECOND_RADIUS * sin_lookup(angle * angle_180 / 30) / TRIG_MAX_RATIO, center.y - SECOND_RADIUS * cos_lookup(angle * angle_180 / 30) / TRIG_MAX_RATIO);
        graphics_fill_circle(ctx, marker, SECOND_THICKNESS);
      }
    } else {
      for (i=(secondMarkers % 15 + 1); i<=15; i++) {
        int angle = i * 4;
        GPoint marker = GPoint(center.x + SECOND_RADIUS * sin_lookup(angle * angle_180 / 30) / TRIG_MAX_RATIO, center.y - SECOND_RADIUS * cos_lookup(angle * angle_180 / 30) / TRIG_MAX_RATIO);
        graphics_fill_circle(ctx, marker, SECOND_THICKNESS);
      }
    }
  }
  // draw hour hand
  graphics_draw_arc_cw(ctx, center, HOUR_RADIUS, HOUR_THICKNESS, -angle_90, ((t->tm_hour * angle_180 / 6) - angle_90), GColorBlack);

  // draw minute hand
  graphics_draw_arc_cw(ctx, center, MINUTE_RADIUS, MINUTE_THICKNESS, -angle_90, ((t->tm_min * angle_180 / 30) - angle_90), GColorBlack);
}

static void update_astro(Layer *layer, GContext *ctx) {
  bg_update(layer, ctx);
  draw_astro(layer, ctx);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(timeLayer);
}

static void init(void) {
  window = window_create();
  window_set_background_color(window, GColorWhite);
  window_stack_push(window, true);

  rootLayer = window_get_root_layer(window);
  astroLayer = layer_create(GRect(0, 0, 144, 168));
  timeLayer = layer_create(GRect(0, 0, 144, 168));
  layer_set_update_proc(timeLayer, draw_time);
  layer_set_update_proc(astroLayer, update_astro);
  layer_add_child(rootLayer, astroLayer);
  layer_add_child(rootLayer, timeLayer);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  layer_destroy(timeLayer);
  layer_destroy(astroLayer);
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
