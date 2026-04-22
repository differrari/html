#include "doc.h"
#include "syscalls/syscalls.h"
#include "math/math.h"
#include "draw/textdraw.h"

typedef struct {
    gpu_rect canvas;
    doc_layout_types direction;
} doc_layout;

typedef struct {
    bool force_newline;
} doc_layout_result;

int text_to_scale(doc_text_size type){
    switch (type) {
        case doc_text_body:          return 3;
        case doc_text_title:         return 7;
        case doc_text_subtitle:      return 6;
        case doc_text_heading:       return 5;
        case doc_text_subheading:    return 4;
        case doc_text_footnote:      return 3;
        case doc_text_caption:       return 2;
        case doc_text_none:          return 0;
    }
}

data_signature supported_data(doc_gen_type type){
    switch (type) {
        case doc_gen_text: return DATA_SIG_TEXT;
        default: return DATA_SIG_UNKNOWN;
    }
}

int text_force_newline(doc_text_size type){
    return false;
}

static gpu_size calculate_label_size(string_slice slice, u32 font_size){
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

gpu_rect calculate_label(string_slice slice, u32 font_size, gpu_rect rect, horizontal_alignment horiz_align, vertical_alignment vert_align){
    //TODO: have the new textdraw more accurately do this for us
    gpu_size s = calculate_label_size(slice,font_size);
    gpu_point point = rect.point;
    switch (horiz_align)
    {
    case trailing:
        point.x = (s.width >= rect.size.width) ? rect.point.x : (int32_t)(rect.point.x + (rect.size.width - s.width));
        break;
    case horizontal_center:
        point.x = (s.width >= rect.size.width) ? rect.point.x : (int32_t)(rect.point.x + ((rect.size.width - s.width)/2));
        break;
    default:
        break;
    }

    switch (vert_align)
    {
    case bottom:
        point.y = (s.height >= rect.size.height) ? rect.point.y : (int32_t)(rect.point.y + (rect.size.height - s.height));
        break;
    case vertical_center:
        point.y = (s.height >= rect.size.height) ? rect.point.y : (int32_t)(rect.point.y + ((rect.size.height - s.height)/2));
        break;
    default:
        break;
    }

    return (gpu_rect){.size = s, .point = point};
}

doc_layout_result layout_doc_node(doc_layout layout, document_data doc, document_node *node){
    doc_layout_result result = {};
    if (!node) return result;
    if (node->info.sizing_rule != size_absolute) node->info.rect.point = (gpu_point){layout.canvas.point.x,layout.canvas.point.y};
    if (node->info.general_type == doc_gen_layout){
        if (node->info.type != doc_layout_none) layout.direction = node->info.type;
    }
    if (node->info.sizing_rule == size_fill || node->info.sizing_rule == size_relative){
        node->info.rect.size = layout.canvas.size;
    }
    
    if (node->info.sizing_rule != size_absolute){
        layout.canvas.point.x += node->info.padding;
        layout.canvas.point.y += node->info.padding;
    }
    layout.canvas.size.width -= node->info.padding * 2;
    layout.canvas.size.height -= node->info.padding * 2;
    if (node->children){
        size_t num_children = linked_list_count(node->children);
        int index = 0;
        float remaining_percentage = 1.f;
        bool force_equal = false;
        int remaining_children = 0;
        for (linked_list_node_t *n = node->children->head; n; n = n->next){
            if (!n->data) break;
            document_node *child = n->data;
            if (child->info.sizing_rule == size_relative){
                remaining_percentage -= child->info.percentage;
            } else remaining_children++;
        }
        if (remaining_percentage <= 0 || remaining_percentage >= 1){
            remaining_percentage = 1;
            force_equal = true;
        }
        for (linked_list_node_t *n = node->children->head; n; n = n->next){
            if (!n->data) break;
            document_node *child = n->data;
            doc_layout new_layout = layout;
            float view_percentage = child->info.sizing_rule == size_relative ? clampf(child->info.percentage,0.f,1.f) : remaining_percentage/remaining_children;
            if (layout.direction == doc_layout_horizontal){
                new_layout.canvas.size.width = force_equal ? floor((float)new_layout.canvas.size.width / num_children) : floor(new_layout.canvas.size.width*view_percentage);
            } else if (layout.direction == doc_layout_vertical){
                new_layout.canvas.size.height = force_equal ? floor((float)new_layout.canvas.size.height / num_children) : floor(new_layout.canvas.size.height*view_percentage);
            }
            doc_layout_result layout_result = layout_doc_node(new_layout, doc, child);
            if (layout.direction == doc_layout_horizontal && !layout_result.force_newline){
                layout.canvas.point.x += child->info.rect.size.width;
                if (node->info.sizing_rule == size_fit){
                    node->info.rect.size.width += child->info.rect.size.width;
                    node->info.rect.size.height = max(node->info.rect.size.height,child->info.rect.size.height);
                }
            } else if (layout.direction != doc_layout_depth){
                layout.canvas.point.y += child->info.rect.size.height;
                if (node->info.sizing_rule == size_fit){
                    node->info.rect.size.height += child->info.rect.size.height;
                    node->info.rect.size.width = max(node->info.rect.size.width,child->info.rect.size.width);
                }
            } else if (node->info.sizing_rule == size_fit){
                node->info.rect.size.height = max(node->info.rect.size.height,child->info.rect.size.height);
                node->info.rect.size.width = max(node->info.rect.size.width,child->info.rect.size.width);
            }
        }
    }
    if (node->content.length){
        int text_size = text_to_scale(node->info.type);
        if (!text_size) return (doc_layout_result){};
        gpu_rect label_rect = calculate_label(node->content, text_size, layout.canvas, node->info.horiz_alignment, node->info.vert_alignment);
        label_rect.size.width += node->info.padding * 2;
        label_rect.size.height += node->info.padding * 2;
        if (node->info.sizing_rule == size_fit) node->info.rect.size = label_rect.size;
        node->info.rect.point = label_rect.point;
        result.force_newline = text_force_newline(node->info.type);
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
        fb_fill_rect(ctx, node->info.rect.point.x + node->info.padding, node->info.rect.point.y + node->info.padding, node->info.rect.size.width - (node->info.padding*2), node->info.rect.size.height - (node->info.padding*2), node->info.bg_color);
    }
    if (node->children)
        for (linked_list_node_t *n = node->children->head; n; n = n->next){
            if (!n->data) break;
            render_doc_node(ctx,n->data);
        }
    if (node->content.length){
        int text_size = text_to_scale(node->info.type);
        gpu_rect rect = (gpu_rect){ 
            { 
                node->info.rect.point.x + node->info.offset.x + node->info.padding, 
                node->info.rect.point.y + node->info.offset.y + node->info.padding
            }, { 
                node->info.rect.size.width - node->info.padding*2,
                node->info.rect.size.height - node->info.padding*2
            }
        };
        if (node->info.text_formatting.array_type != fmt_array_none){
            fb_draw_text(ctx, node->content, rect, (text_format){.scale = text_size, .color = node->info.fg_color, .wrap = node->info.text_wrap_policy }, node->info.text_formatting);
        } else 
            fb_draw_single_text(ctx, node->content, rect, (text_format){.scale = text_size, .color = node->info.fg_color, .wrap = node->info.text_wrap_policy });
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
    print("%sNode %ix%i - %ix%i - %i",indent_by(depth),node->info.rect.point.x,node->info.rect.point.y,node->info.rect.size.width,node->info.rect.size.height,node->info.padding);
    if (node->children)
        for (linked_list_node_t *n = node->children->head; n; n = n->next){
            if (!n->data) break;
            debug_node(n->data, depth+1);
        }
}

void debug_document(document_data doc){
    debug_node(doc.root, 0);
}