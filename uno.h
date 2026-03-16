#pragma once

#include "types.h"
#include "doc.h"

void uno_begin_vertical(node_info info);
void uno_end_vertical();

void uno_begin_horizontal(node_info info);
void uno_end_horizontal();

void uno_begin_depth(node_info info);
void uno_end_depth();

void uno_create_empty_view(node_info info);
document_node* uno_create_view(node_info info, string_slice content);

void uno_text_field(int tag, node_info info, string *content, string_slice placeholder);

void set_document_view(void (*view_builder)(), gpu_rect canvas);
void uno_refresh();
void uno_refresh_layout();

void uno_draw(draw_ctx *ctx);

void uno_focus(int tag);
bool uno_dispatch_kbd(kbd_event ev);

#define VERTICAL(info, children) uno_begin_vertical((info)); children; uno_end_vertical();
#define HORIZONTAL(info, children) uno_begin_horizontal((info)); children; uno_end_horizontal();
#define DEPTH(info, children) uno_begin_depth((info)); children; uno_end_depth();

extern document_data default_doc_data;