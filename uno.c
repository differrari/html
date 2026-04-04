#include "uno.h"
#include "data/struct/chunk_array.h"
#include "syscalls/syscalls.h"
#include "input_keycodes.h"
#include "memory/memory.h"
#include "math/math.h"

void (*view_build_func)();
document_data default_doc_data;
gpu_rect default_canvas;
document_node* current_node;

document_node* focused_node;
int focused_tag;

chunk_array_t *node_stack;

document_node* uno_make_view(node_info info){
    document_node *node = zalloc(sizeof(document_node));
    // print("Alloc %llx",node);
    node->info = info;
    return node;
}

void uno_attach(document_node *parent, document_node *child){
    if (!parent || !child) return;
    if (!parent->children) parent->children = linked_list_create();
    // print("Child now %llx",&parent->children);
    linked_list_push(parent->children, child);
}

void uno_state_push(document_node *new_node){
    if (!node_stack){
        node_stack = chunk_array_create(sizeof(uptr), 64);
    }
    if (current_node){
        chunk_array_push(node_stack, &current_node);
    }
    // print(">Save %llx. New %llx, %i in stack",current_node,new_node,chunk_array_count(node_stack));
    current_node = new_node;
}

void uno_state_pop(){
    size_t count = chunk_array_count(node_stack);
    if (count){
        current_node = (document_node*)*(uptr*)chunk_array_pop(node_stack);
        // print("< Current node now %llx",current_node);
    }
}

void uno_begin_vertical(node_info info){
    info.general_type = doc_gen_layout;
    info.type = doc_layout_vertical;
    document_node* vert = uno_make_view(info);
    if (!default_doc_data.root){
        default_doc_data.root = vert;
    }
    if (current_node)
        uno_attach(current_node, vert);
    uno_state_push(vert);
}

void uno_end_vertical(){
    uno_state_pop();
}

void uno_begin_horizontal(node_info info){
    info.general_type = doc_gen_layout;
    info.type = doc_layout_horizontal;
    document_node* horiz = uno_make_view(info);
    if (!default_doc_data.root){
        default_doc_data.root = horiz;
    }
    if (current_node)
        uno_attach(current_node, horiz);
    uno_state_push(horiz);
}

void uno_end_horizontal(){
    uno_state_pop();
}

void uno_begin_depth(node_info info){
    info.general_type = doc_gen_layout;
    info.type = doc_layout_depth;
    document_node* horiz = uno_make_view(info);
    if (!default_doc_data.root){
        default_doc_data.root = horiz;
    }
    if (current_node)
        uno_attach(current_node, horiz);
    uno_state_push(horiz);
}

void uno_end_depth(){
    uno_state_pop();
}

document_node* uno_create_view(node_info info, string_slice content){
    document_node* node = uno_make_view(info);
    node->content = content;
    if (!default_doc_data.root){
        default_doc_data.root = node;
    }
    if (current_node)
        uno_attach(current_node, node);
    return node;
}

void uno_create_empty_view(node_info info){
    document_node* node = uno_make_view(info);
    if (!default_doc_data.root)
        default_doc_data.root = node;
    if (current_node)
        uno_attach(current_node, node);
}

void uno_destroy_node(void *ptr){
    document_node* node = ptr;
    if (!node) return;
    if (node->children){
        linked_list_for_each(node->children, uno_destroy_node);
        // linked_list_destroy(node->children);
        node->children = 0;
    }
    // print("Release %llx",node);
    release(node);
}

void set_document_view(void (*view_builder)(), gpu_rect canvas){
    view_build_func = view_builder;
    default_canvas = canvas;
    uno_refresh();
}

void uno_refresh(){
    chunk_array_reset(node_stack);
    uno_destroy_node(default_doc_data.root);
    default_doc_data.root = 0;
    current_node = 0;
    if (view_build_func) view_build_func();
    uno_refresh_layout();
    if (focused_tag) uno_focus(focused_tag);
}

void uno_refresh_layout(){
    layout_document(default_canvas, default_doc_data);
}

void uno_draw(draw_ctx *ctx){
    render_document(ctx, default_doc_data);
}

document_node* uno_find_node(document_node *node, int tag){
    if (!tag) return 0;
    if (node->input.tag == tag){
        return node;
    }
    else if (node->children){
        for (linked_list_node_t *child = node->children->head; child; child = child->next){
            if (child && child->data){
                document_node* ret = uno_find_node(child->data, tag);
                if (ret) return ret;
            }
        }
    }
    return 0;
}

void uno_focus(int tag){
    focused_node = 0;
    if (!tag)
        return;
    document_node *node = default_doc_data.root;
    focused_node = uno_find_node(node, tag);
    if (focused_node) focused_tag = tag;
}

bool uno_text_field_input(document_node *node, kbd_event event){
    if (!node || !node->ctx) return false;
    text_field_info *info = node->ctx;
    buffer *content = info->content;
    if (!content || !content->buffer) return false;
    if (event.key == KEY_ENTER && !info->multiline) return false;
    if (event.modifier == KEY_MOD_LSHIFT || event.modifier == KEY_MOD_LCTRL)
        info->modifier = event.type == MOD_RELEASE ? 0 : event.modifier;
    if (event.type != KEY_PRESS) return false;
    if (event.key == KEY_BACKSPACE){
        buffer_delete(content, 1);
        uno_refresh();
        return true;
    }
    if (event.key == KEY_TAB){
        char *indent = "\t\t\t\t";
        buffer_write_to(content, indent, 4, content->cursor);
        uno_refresh();
        return true;
    }
    char c = hid_to_char(event.key, info->modifier);
    if (event.type == KEY_PRESS && c){
        buffer_write_to(content, &c, 1, content->cursor);
        uno_refresh();
        return true;
    }
    
    return false;
}

u32 lin_col_to_pos(i32 line, i32 col, string_slice content){
    i32 line_number = 0;
    i32 column = 0;
    u32 pos = 0;
    for (u32 i = 0; i < content.length; i++){
        pos = i;
        if (content.data[i] == '\n'){
            if (line_number == line) return i;
            column = 0;
            line_number++;
        } else {
            if (line_number == line && column == col) return i;
            column++;   
        }
    }
    return pos;
}

void pos_to_lin_col(u32 pos, string_slice content, i32 *lin, i32 *col){
    *lin = 0;
    *col = 0;
    for (u32 i = 0; i < min(pos,content.length); i++){
        if (content.data[i] == '\n'){
            *col = 0;
            *lin = (*lin) + 1;
        } else *col = (*col) + 1;
    }
}

bool uno_text_field_select(document_node *node, mouse_data data){
    if (data.position.x < node->info.rect.point.x || 
        data.position.x > node->info.rect.point.x + node->info.rect.size.width || 
        data.position.y < node->info.rect.point.y || 
        data.position.y > node->info.rect.point.y + node->info.rect.size.height) return false;
    text_field_info *info = node->ctx;
    if (!info) return false;
    buffer *content = info->content;
    if (!content || !content->buffer) return false;
    i32 x = data.position.x/fb_get_char_size(text_to_scale(node->info.type));
    i32 y = data.position.y/(fb_get_char_size(text_to_scale(node->info.type)) + 2);//TODO: line padding should be customizable
    float oy = (float)node->info.offset.y/((i32)fb_get_char_size(text_to_scale(node->info.type)) + 2.f);
    
    float ox = (float)node->info.offset.x/(i32)fb_get_char_size(text_to_scale(node->info.type));

    x -= round_to_int(ox);
    y -= round_to_int(oy);
    content->cursor = lin_col_to_pos(y, x, (string_slice){content->buffer,content->buffer_size});
    uno_refresh();
    return true;
}

#define line_height(text) (fb_get_char_size(text_to_scale(text)) + 2)
#define char_width(text) (fb_get_char_size(text_to_scale(text)))

void uno_text_field(int tag, node_info info, text_field_info *text_info){
    info.general_type = doc_gen_text;
    if (info.type == doc_gen_type_none) info.type = doc_text_footnote;
    if (!((info.fg_color >> 24) & 0xFF)) info.fg_color |= 0xFF << 24;
    
    info.offset = text_info->offset;
    uno_begin_depth(info);
    document_node *node = uno_create_view((node_info){.general_type = info.general_type, .type = info.type, .fg_color = info.fg_color, .bg_color = info.bg_color, .offset = info.offset}, text_info->content && text_info->content->buffer && text_info->content->buffer_size ? (string_slice){.data = text_info->content->buffer, .length = text_info->content->buffer_size } : text_info->placeholder);
    node->input.keyboard_input = uno_text_field_input;
    node->input.mouse_input = uno_text_field_select;
    node->input.tag = tag;
    
    node->ctx = text_info;
    
    i32 lin, col = 0;
    pos_to_lin_col(text_info->content->cursor, (string_slice){text_info->content->buffer,text_info->content->buffer_size}, &lin, &col);
    
    document_node *cursor = uno_create_view((node_info){.bg_color = text_info->cursor_color, .sizing_rule = size_absolute, .rect = (gpu_rect){node->info.offset.x + (col * char_width(info.type)),node->info.offset.y + (lin * line_height(info.type)),3,line_height(info.type)}}, (string_slice){});
    
    uno_end_depth();
}

void uno_text_field_scroll(int tag, i32 x_shift, i32 y_shift){
    document_node *node = uno_find_node(default_doc_data.root, tag);
    if (!node) return;
    if (node->info.general_type != doc_gen_text) return;
    text_field_info *info = node->ctx;
    if (!info) return;
    buffer *content = info->content;
    if (!content || !content->buffer) return;
    u8 cw = line_height(node->info.type);
    u8 lh = line_height(node->info.type);
    if (x_shift){
        if (info->offset.x + x_shift > 0) info->offset.x = 0;
        else info->offset.x += x_shift * cw;
    }
    if (y_shift){
        if (info->offset.y + y_shift > 0) info->offset.y = 0;
        else info->offset.y += y_shift * lh;   
    }
}

void uno_text_field_shift_cursor(int tag, i32 x_shift, i32 y_shift){
    document_node *node = uno_find_node(default_doc_data.root, tag);
    if (!node) return;
    if (node->info.general_type != doc_gen_text) return;
    text_field_info *info = node->ctx;
    if (!info) return;
    buffer *content = info->content;
    if (!content || !content->buffer) return;
    u8 cw = line_height(node->info.type);
    u8 lh = line_height(node->info.type);
    if (x_shift){
        if ((i64)content->cursor + x_shift < 0) content->cursor = 0;
        else content->cursor += x_shift;
        i32 lin, col = 0;
        string_slice slice = (string_slice){content->buffer,content->buffer_size};
        pos_to_lin_col(content->cursor, slice, &lin, &col);
        if (col - (info->offset.x/cw) >= node->info.rect.size.width/cw) uno_text_field_scroll(tag, x_shift, y_shift);
    }
    if (y_shift){
        i32 lin, col = 0;
        string_slice slice = (string_slice){content->buffer,content->buffer_size};
        pos_to_lin_col(content->cursor, slice, &lin, &col);
        if (lin + y_shift < 0) lin = 0;
        else lin += y_shift;
        content->cursor = lin_col_to_pos(lin, col, slice);
        if (lin - (info->offset.y/lh) > node->info.rect.size.height/lh) uno_text_field_scroll(tag, x_shift, y_shift);
    }
    content->cursor = clamp(content->cursor, 0, content->buffer_size);
}

void uno_label(node_info info, doc_text_size size, string_slice content){
    info.general_type = doc_gen_text;
    info.type = size;
    info.sizing_rule = size_fit;
    if (!((info.fg_color >> 24) & 0xFF)) info.fg_color |= 0xFF << 24;
    uno_create_view(info, content);
}

bool uno_dispatch_kbd(kbd_event ev){
    if (!focused_node) return false;//TODO: find input automatically?
    
    if (focused_node->input.keyboard_input)
        return focused_node->input.keyboard_input(focused_node,ev);
    
    return false;
}

bool uno_dispatch_mouse(mouse_data mouse){
    if (!mouse_button_down(&mouse, 0)) return false;
    // if (!focused_node){
    //     //TODO: focus
    // }
    
    if (focused_node->input.mouse_input)
        return focused_node->input.mouse_input(focused_node,mouse);
    
    return false;
}