#include <pebble.h>
#include "geometry.h"

static int angle_45 = TRIG_MAX_ANGLE / 8;
static int angle_90 = TRIG_MAX_ANGLE / 4;
static int angle_180 = TRIG_MAX_ANGLE / 2;
static int angle_270 = 3 * TRIG_MAX_ANGLE / 4;

// copied (and renamed) the DrawArc function from https://github.com/Jnmattern/Arc_2.0
// who, in turn, copied it from Cameron MacFarland (http://forums.getpebble.com/profile/12561/Cameron%20MacFarland)
void graphics_draw_arc_cw(GContext *ctx, GPoint center, int radius, int thickness, int start_angle, int end_angle, GColor c) {
    int32_t xmin = 65535000, xmax = -65535000, ymin = 65535000, ymax = -65535000;
    int32_t cosStart, sinStart, cosEnd, sinEnd;
    int32_t r, t;

    while (start_angle < 0) start_angle += TRIG_MAX_ANGLE;
    while (end_angle < 0) end_angle += TRIG_MAX_ANGLE;

    start_angle %= TRIG_MAX_ANGLE;
    end_angle %= TRIG_MAX_ANGLE;

    if (end_angle == 0) end_angle = TRIG_MAX_ANGLE;

    if (start_angle > end_angle) {
        graphics_draw_arc_cw(ctx, center, radius, thickness, start_angle, TRIG_MAX_ANGLE, c);
        graphics_draw_arc_cw(ctx, center, radius, thickness, 0, end_angle, c);
    } else {
        // Calculate bounding box for the arc to be drawn
        cosStart = cos_lookup(start_angle);
        sinStart = sin_lookup(start_angle);
        cosEnd = cos_lookup(end_angle);
        sinEnd = sin_lookup(end_angle);

        r = radius;
        // Point 1: radius & start_angle
        t = r * cosStart;
        if (t < xmin) xmin = t;
        if (t > xmax) xmax = t;
        t = r * sinStart;
        if (t < ymin) ymin = t;
        if (t > ymax) ymax = t;

        // Point 2: radius & end_angle
        t = r * cosEnd;
        if (t < xmin) xmin = t;
        if (t > xmax) xmax = t;
        t = r * sinEnd;
        if (t < ymin) ymin = t;
        if (t > ymax) ymax = t;

        r = radius - thickness;
        // Point 3: radius-thickness & start_angle
        t = r * cosStart;
        if (t < xmin) xmin = t;
        if (t > xmax) xmax = t;
        t = r * sinStart;
        if (t < ymin) ymin = t;
        if (t > ymax) ymax = t;

        // Point 4: radius-thickness & end_angle
        t = r * cosEnd;
        if (t < xmin) xmin = t;
        if (t > xmax) xmax = t;
        t = r * sinEnd;
        if (t < ymin) ymin = t;
        if (t > ymax) ymax = t;

        // Normalization
        xmin /= TRIG_MAX_RATIO;
        xmax /= TRIG_MAX_RATIO;
        ymin /= TRIG_MAX_RATIO;
        ymax /= TRIG_MAX_RATIO;

        // Corrections if arc crosses X or Y axis
        if ((start_angle < angle_90) && (end_angle > angle_90)) {
            ymax = radius;
        }

        if ((start_angle < angle_180) && (end_angle > angle_180)) {
            xmin = -radius;
        }

        if ((start_angle < angle_270) && (end_angle > angle_270)) {
            ymin = -radius;
        }

        // Slopes for the two sides of the arc
        float sslope = (float)cosStart/ (float)sinStart;
        float eslope = (float)cosEnd / (float)sinEnd;

        if (end_angle == TRIG_MAX_ANGLE) eslope = -1000000;

        int ir2 = (radius - thickness) * (radius - thickness);
        int or2 = radius * radius;

        graphics_context_set_stroke_color(ctx, c);

        for (int x = xmin; x <= xmax; x++) {
            for (int y = ymin; y <= ymax; y++)
            {
                int x2 = x * x;
                int y2 = y * y;

                if (
                    (x2 + y2 < or2 && x2 + y2 >= ir2) && (
                        (y > 0 && start_angle < angle_180 && x <= y * sslope) ||
                        (y < 0 && start_angle > angle_180 && x >= y * sslope) ||
                        (y < 0 && start_angle <= angle_180) ||
                        (y == 0 && start_angle <= angle_180 && x < 0) ||
                        (y == 0 && start_angle == 0 && x > 0)
                    ) && (
                        (y > 0 && end_angle < angle_180 && x >= y * eslope) ||
                        (y < 0 && end_angle > angle_180 && x <= y * eslope) ||
                        (y > 0 && end_angle >= angle_180) ||
                        (y == 0 && end_angle >= angle_180 && x < 0) ||
                        (y == 0 && start_angle == 0 && x > 0)
                    )
                )
                graphics_draw_pixel(ctx, GPoint(center.x+x, center.y+y));
            }
        }
    }
}

// for completeness -- this isn't used.
void graphics_draw_arc_ccw(GContext *ctx, GPoint center, int radius, int thickness, int start_angle, int end_angle, GColor c) {
  graphics_draw_arc_cw(ctx, center, radius, thickness, end_angle, start_angle, c);
}
 
void project_gpoint_to_edges(GPoint *point, Layer *layer, int angle) {
  GRect bounds = layer_get_bounds(layer);
  const GPoint center = grect_center_point(&bounds);
  angle %= TRIG_MAX_ANGLE;
  
  if (angle <= angle_45 || angle > angle_45 * 7) {
    point->x = center.x + bounds.size.w / 2;
    point->y = center.y + bounds.size.w * sin_lookup(angle) / cos_lookup(angle) / 2;
  } else if (angle > angle_45 * 3 && angle <= angle_45 * 5) {
    point->x = center.x - bounds.size.w / 2;
    point->y = center.y - bounds.size.w * sin_lookup(angle) / cos_lookup(angle) / 2;
  } else if (angle > angle_45 && angle <= angle_45 * 3) {
    point->x = center.x + bounds.size.h * cos_lookup(angle) / sin_lookup(angle) / 2;
    point->y = center.y + bounds.size.h / 2;
  } else if (angle > angle_45 * 5 && angle <= angle_45 * 7) {
    point->x = center.x - bounds.size.h * cos_lookup(angle) / sin_lookup(angle) / 2;
    point->y = center.y - bounds.size.h / 2;
  }
}

void graphics_draw_ray(GContext *ctx, Layer *layer, GPoint center, int theta, int thickness_angle, GColor stroke_color, GColor fill_color) {
  GRect bounds = layer_get_bounds(layer);
  GPoint overshot = GPoint(0,0);
  GPoint undershot = GPoint(0,0);
  
  thickness_angle = thickness_angle * TRIG_MAX_ANGLE / 360;
  project_gpoint_to_edges(&undershot, layer, theta - thickness_angle);
  project_gpoint_to_edges(&overshot, layer, theta + thickness_angle);
  graphics_context_set_stroke_color(ctx, stroke_color);
  graphics_context_set_fill_color(ctx, fill_color);
  
  GPathInfo ray_info = {
    .num_points = 5,
    .points = (GPoint[]) {
      center,
      undershot,
      undershot,
      overshot,
      center
    }
  };
  // cornering =|
  if (undershot.x != overshot.x || undershot.y != overshot.y) {
    // ew
    if (undershot.x == bounds.size.w || overshot.x == bounds.size.w)
      ray_info.points[2].x = bounds.size.w;
    else
      ray_info.points[2].x = bounds.origin.x;
    // ew ew ew
    if (undershot.y == bounds.size.h || overshot.y == bounds.size.h)
      ray_info.points[2].y = bounds.size.h;
    else
      ray_info.points[2].y = bounds.origin.y;
  }
  GPath *ray = NULL;
  ray = gpath_create(&ray_info);
  gpath_draw_filled(ctx, ray);
  gpath_draw_outline(ctx, ray);
}