#include "uno.h"
#include "data/struct/chunk_array.h"
#include "syscalls/syscalls.h"
#include "input_keycodes.h"
#include "memory/memory.h"

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

void uno_focus_node(document_node *node, int tag){
    if (!tag) return;
    if (node->input.tag == tag){
        focused_node = node;
        focused_tag = tag;
    }
    else if (node->children){
        for (linked_list_node_t *child = node->children->head; child; child = child->next){
            if (child && child->data) uno_focus_node(child->data, tag);
        }
    }
}

void uno_focus(int tag){
    focused_node = 0;
    if (!tag)
        return;
    document_node *node = default_doc_data.root;
    uno_focus_node(node, tag);
}

bool uno_text_field_input(void *ctx, kbd_event event){
    if (!ctx) return false;
    string *content = ctx;
    if (event.key == KEY_ENTER) return false;
    if (event.type == KEY_PRESS && hid_keycode_to_char[event.key]){
        string new_string = string_from_char(hid_keycode_to_char[event.key]);
        if (!content->data){
            *content = new_string;
        } else {
            string_concat_inplace(content, new_string);
            string_free(new_string);
        }
        uno_refresh();
        return true;
    }
    if (event.type == KEY_PRESS && event.key == KEY_BACKSPACE){
        if (content->length) content->length--;
        if (content->data) content->data[content->length] = 0;
        if (content->data && !content->length){
            string_free(*content);
            memset(content, 0, sizeof(string));
        }
        uno_refresh();
        return true;
    }
    return false;
}

void uno_text_field(int tag, node_info info, string *content, string_slice placeholder){
    info.general_type = doc_gen_text;
    if (info.type == doc_gen_type_none) info.type = doc_text_footnote;
    if (!((info.fg_color >> 24) & 0xFF)) info.fg_color |= 0xFF << 24;
    
    document_node *node = uno_create_view(info, content && content->data && content->length ? slice_from_string(*content) : placeholder);
    node->input.keyboard_input = uno_text_field_input;
    node->input.tag = tag;
    node->ctx = content;
}

bool uno_dispatch_kbd(kbd_event ev){
    if (!focused_node) return false;//TODO: find input automatically?
    
    if (focused_node->input.keyboard_input)
        return focused_node->input.keyboard_input(focused_node->ctx,ev);
    
    return false;
}