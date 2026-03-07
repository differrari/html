#pragma once

#include "data/struct/linked_list.h"
#include "string/slice.h"
#include "draw/draw.h"

typedef enum { doc_type_none, doc_simple_text, doc_title, doc_subtitle, doc_heading, doc_subheading, doc_h5, doc_h6 } doc_node_type;
typedef enum { doc_layout_none, doc_layout_vertical, doc_layout_horizontal } doc_layout_types;
typedef enum { doc_gen_type_none, doc_gen_text, doc_gen_layout } doc_gen_type;

typedef enum { size_none, size_fit, size_fill } size_rule;

typedef struct {
    int type;
    doc_gen_type general_type;
    
    color bg_color;
    color fg_color;
    
    size_rule sizing_rule;
    gpu_rect rect;
    
} node_info;

typedef struct {
    node_info info;
    linked_list_t *children;
    string_slice content;
} document_node;

typedef struct {
    document_node *root;
} document_data;

void layout_document(gpu_rect canvas, document_data doc);
void render_document(draw_ctx *ctx, document_data doc);
void debug_document(document_data doc);