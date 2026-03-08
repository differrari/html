#include "front.h"
#include "data/struct/chunk_array.h"
#include "syscalls/syscalls.h"

document_data default_doc_data;
document_node* current_node;

chunk_array_t *node_stack;

document_node* uno_make_view(node_info info){
    document_node *node = zalloc(sizeof(document_node));
    node->info = info;
    return node;
}

void uno_attach(document_node *parent, document_node *child){
    if (!parent || !child) return;
    if (!parent->children) parent->children = linked_list_create();
    linked_list_push(parent->children, child);
}

void uno_state_push(document_node *new_node){
    if (!node_stack){
        node_stack = chunk_array_create(sizeof(uptr), 64);
    }
    if (current_node){
        chunk_array_push(node_stack, &current_node);
    }
    print(">Save %llx. New %llx, %i in stack",current_node,new_node,chunk_array_count(node_stack));
    current_node = new_node;
}

void uno_state_pop(){
    size_t count = chunk_array_count(node_stack);
    if (count){
        current_node = (document_node*)*(uptr*)chunk_array_pop(node_stack);
        print("< Current node now %llx",current_node);
    } else print("No elements in array");
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

void uno_create_view(node_info info, string_slice content){
    document_node* node = uno_make_view(info);
    node->content = content;
    if (!default_doc_data.root){
        default_doc_data.root = node;
    }
    if (current_node)
        uno_attach(current_node, node);
}

void uno_create_empty_view(node_info info){
    document_node* node = uno_make_view(info);
    if (!default_doc_data.root)
        default_doc_data.root = node;
    if (current_node)
        uno_attach(current_node, node);
}