#pragma once

#include "data/struct/linked_list.h"
#include "string/slice.h"
#include "draw/draw.h"

typedef enum { doc_type_none, doc_simple_text, doc_title, doc_subtitle, doc_heading, doc_subheading, doc_h5, doc_h6 } doc_node_type;
typedef enum { doc_gen_type_none, doc_gen_text } doc_gen_type;

typedef struct {
    doc_node_type type;
    doc_gen_type general_type;
} node_info;

typedef struct {
    node_info info;
    linked_list_t *contents;
    string_slice content;
} document_node;

typedef struct {
    document_node *root;
} document_data;

void render_document(draw_ctx *ctx, gpu_rect canvas, document_data doc);