#pragma once

#include "types.h"
#include "doc.h"
#include "uno_helpers.h"

void uno_begin_vertical(node_info info);
void uno_end_vertical();

void uno_begin_horizontal(node_info info);
void uno_end_horizontal();

void uno_begin_depth(node_info info);
void uno_end_depth();

void uno_create_empty_view(node_info info);
void uno_create_view(node_info info, string_slice content);

void set_document_view(void (*view_builder)(), gpu_rect canvas);
void uno_refresh();
void uno_refresh_layout();

void uno_draw(draw_ctx *ctx);

#define VERTICAL(info, children) uno_begin_vertical((info)); children; uno_end_vertical();
#define HORIZONTAL(info, children) uno_begin_horizontal((info)); children; uno_end_horizontal();
#define DEPTH(info, children) uno_begin_depth((info)); children; uno_end_depth();

extern document_data default_doc_data;