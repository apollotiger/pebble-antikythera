#pragma once
#include <pebble.h>

void graphics_draw_arc_cw(GContext *ctx, GPoint center, int radius, int thickness, int start_angle, int end_angle, GColor c) ;
void project_gpoint_to_edges(GPoint *point, Layer *layer, int angle) ;
void graphics_draw_ray(GContext *ctx, Layer *layer, GPoint center, int theta, int thickness_angle, GColor stroke_color, GColor fill_color) ;