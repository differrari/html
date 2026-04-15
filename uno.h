#pragma once

#include "types.h"
#include "doc.h"
#include "files/buffer.h"

void uno_begin_vertical(node_info info);
void uno_end_vertical();

void uno_begin_horizontal(node_info info);
void uno_end_horizontal();

void uno_begin_depth(node_info info);
void uno_end_depth();

void uno_create_empty_view(node_info info);
document_node* uno_create_view(node_info info, string_slice content);

typedef struct {
    buffer *content;
    string_slice placeholder;
    bool multiline;
    bool modifier;
    color cursor_color;
    gpu_point offset;
} text_field_info;

void uno_text_field(int tag, node_info info, text_field_info *text_info);
//TODO: these should be moved elsewhere, just not sure where
u32 lin_col_to_pos(i32 line, i32 col, string_slice content);
void pos_to_lin_col(u32 pos, string_slice content, i32 *lin, i32 *col);
void uno_text_field_scroll(int tag, i32 x_shift, i32 y_shift);
void uno_text_field_shift_cursor(int tag, i32 x_shift, i32 y_shift);

typedef struct {
    void (*press)(int buttonIndex, gpu_point location);
    void (*hover)(int buttonIndex, gpu_point location);
} button_info;

void uno_button(int tag, node_info info, button_info *b_info, string_slice label);

void uno_label(node_info info, doc_text_size size, string_slice content);

void set_document_view(void (*view_builder)(), gpu_rect canvas);
void uno_refresh();
void uno_refresh_layout();

void uno_draw(draw_ctx *ctx);

void uno_focus(int tag);
bool uno_dispatch_kbd(kbd_event ev);
bool uno_dispatch_mouse(mouse_data mouse);

#define VERTICAL(info, children) uno_begin_vertical((info)); children; uno_end_vertical();
#define HORIZONTAL(info, children) uno_begin_horizontal((info)); children; uno_end_horizontal();
#define DEPTH(info, children) uno_begin_depth((info)); children; uno_end_depth();

extern document_data default_doc_data;