#include "doc.h"

typedef struct {
    gpu_rect canvas;
    i32 x;
    i32 y;
} doc_layout;

int text_to_scale(doc_node_type type){
    switch (type) {
        case doc_simple_text:   return 3;
        case doc_title:         return 7;
        case doc_subtitle:      return 6;
        case doc_heading:       return 5;
        case doc_subheading:    return 4;
        case doc_h5:            return 3;
        case doc_h6:            return 2;
        case doc_type_none: return 0;
    }
}

void render_doc_node(draw_ctx *ctx, doc_layout *layout, document_data doc, document_node *node){
    if (node->contents){
        for (linked_list_node_t *n = node->contents->head; n; n = n->next)
            render_doc_node(ctx, layout, doc, n->data);
    }
    if (node->content.length){
        switch (node->info.general_type) {
            case doc_gen_text:
            {
                int text_size = text_to_scale(node->info.type);
                if (!text_size) return;
                gpu_size size = fb_draw_slice(ctx, node->content, layout->x, layout->y, text_size, 0xFFFFFFFF);
                layout->y += size.height;
                //TODO: canvas
                break;
            }
            default: break;
        }
    }
}

void render_document(draw_ctx *ctx, gpu_rect canvas, document_data doc){
    doc_layout layout = (doc_layout){.canvas = canvas};
    render_doc_node(ctx, &layout, doc, doc.root);
}