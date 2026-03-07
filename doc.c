#include "doc.h"
#include "syscalls/syscalls.h"
#include "math/math.h"

typedef struct {
    gpu_rect canvas;
    i32 x;
    i32 y;
    doc_layout_types direction;
} doc_layout;

typedef struct {
    bool force_newline;
} doc_layout_result;

int text_to_scale(doc_node_type type){
    switch (type) {
        case doc_simple_text:   return 3;
        case doc_title:         return 7;
        case doc_subtitle:      return 6;
        case doc_heading:       return 5;
        case doc_subheading:    return 4;
        case doc_h5:            return 3;
        case doc_h6:            return 2;
        case doc_type_none:     return 0;
    }
}

int text_force_newline(doc_node_type type){
    return true;
}

gpu_size calculate_label_size(string_slice slice, u32 font_size){
    if (!slice.length || font_size==0) return (gpu_size){0,0};
    int num_lines = 1;
    int num_chars = 0;
    int local_num_chars = 0;
    for (size_t i = 0; i < slice.length; i++){
        if (slice.data[i] == '\n'){
            if (local_num_chars > num_chars)
                num_chars = local_num_chars;
            num_lines++;
            local_num_chars = 0;
        }
        else 
            local_num_chars++;
    }
    if (local_num_chars > num_chars)
        num_chars = local_num_chars;
    unsigned int size = fb_get_char_size(font_size);
    return (gpu_size){size * num_chars, (size + 2) * num_lines };
}

doc_layout_result layout_doc_node(doc_layout layout, document_data doc, document_node *node){
    doc_layout_result result = {};
    node->info.rect.point = (gpu_point){layout.x,layout.y};
    if (!node) return result;
    if (node->info.general_type == doc_gen_layout){
        if (node->info.type != doc_layout_none) layout.direction = node->info.type;
    }
    if (node->info.sizing_rule == size_fill){
        node->info.rect.size = layout.canvas.size;
    }
    if (node->children){
        size_t num_children = linked_list_count(node->children);
        for (linked_list_node_t *n = node->children->head; n; n = n->next){
            if (!n->data) break;
            document_node *child = n->data;
            doc_layout new_layout = layout;
            new_layout.canvas = node->info.rect;
            if (layout.direction == doc_layout_horizontal)
                new_layout.canvas.size.width /= num_children;
            else
                new_layout.canvas.size.height /= num_children;
            doc_layout_result layout_result = layout_doc_node(new_layout, doc, child);
            if (layout.direction == doc_layout_horizontal && !layout_result.force_newline){
                layout.x += child->info.rect.size.width;
                if (node->info.sizing_rule == size_fit){
                    node->info.rect.size.width += child->info.rect.size.width;
                    node->info.rect.size.height = max(child->info.rect.size.height,child->info.rect.size.height);
                }
            } else {
                layout.x = 0;
                layout.y += child->info.rect.size.height;
                if (node->info.sizing_rule == size_fit){
                    node->info.rect.size.height += child->info.rect.size.height;
                    node->info.rect.size.width = max(child->info.rect.size.width,child->info.rect.size.width);
                }
            }
        }
    }
    if (node->content.length){
        switch (node->info.general_type) {
            case doc_gen_text:
            {
                doc_node_type text_type = node->info.type;
                int text_size = text_to_scale(text_type);
                if (!text_size) return (doc_layout_result){};
                node->info.rect.size = calculate_label_size(node->content, text_size);
                return (doc_layout_result){.force_newline = text_force_newline(node->info.type)};
            }
            default: break;
        }
    }
    
    return result;
}

void layout_document(gpu_rect canvas, document_data doc){
    doc_layout layout = (doc_layout){.canvas = canvas};
    layout_doc_node(layout, doc, doc.root);
}

void render_doc_node(draw_ctx *ctx, document_node *node){
    if (!node) return;
    if (node->info.bg_color){
        fb_fill_rect(ctx, node->info.rect.point.x, node->info.rect.point.y, node->info.rect.size.width, node->info.rect.size.height, node->info.bg_color);
    }
    if (node->children)
        for (linked_list_node_t *n = node->children->head; n; n = n->next){
            if (!n->data) break;
            render_doc_node(ctx,n->data);
        }
    if (node->content.length){
        switch (node->info.general_type) {
            case doc_gen_text:
            {
                doc_node_type text_type = node->info.type;
                int text_size = text_to_scale(text_type);
                fb_draw_slice(ctx, node->content, node->info.rect.point.x, node->info.rect.point.y, text_size, node->info.fg_color);
            }
            default: break;
        }
    }
}

void render_document(draw_ctx *ctx, document_data doc){
    render_doc_node(ctx, doc.root);
}

char *indent = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
#define MAX_DEPTH 64
#define indent_by(depth) (indent + (MAX_DEPTH-depth))

void debug_node(document_node *node, int depth){
    if (!node) return;
    print("%sNode %ix%i - %ix%i",indent_by(depth),node->info.rect.point.x,node->info.rect.point.y,node->info.rect.size.width,node->info.rect.size.height);
    if (node->children)
        for (linked_list_node_t *n = node->children->head; n; n = n->next){
            if (!n->data) break;
            debug_node(n->data, depth+1);
        }
}

void debug_document(document_data doc){
    debug_node(doc.root, 0);
}