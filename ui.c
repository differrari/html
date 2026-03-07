#include "doc.h"
#include "syscalls/syscalls.h"

int main(){
    
    draw_ctx ctx = {};
    
    request_draw_ctx(&ctx);
    
    document_data data = {};
    
    data.root = zalloc(sizeof(document_node));
    data.root->info = (node_info){ doc_layout_vertical, doc_gen_layout, .sizing_rule = size_fill, .bg_color = 0xFFFFFFFF };
    data.root->children = linked_list_create();
    
    for (int y = 0; y < 3; y++){
        document_node *row = zalloc(sizeof(document_node));
        row->info = (node_info){ .type = doc_layout_horizontal, .general_type = doc_gen_layout, .sizing_rule = size_fill, .rect = {.size = {30,30}}};
        linked_list_push(data.root->children,row);
        row->children = linked_list_create();
        for (int x = 0; x < 3; x++){
            document_node *column = zalloc(sizeof(document_node));
            column->info = (node_info){ .type = doc_layout_none, .general_type = doc_gen_layout, .bg_color = 0xFFb4dd13 + ((y*0x10) << 8) + (x*0x50), .sizing_rule = size_fill, .rect = {.size = {30,30}}};
            linked_list_push(row->children,column);
            column->content = slice_from_string(string_format("%i",(y *3)+x));
        }
    }
    
    layout_document((gpu_rect){ 0,0,ctx.width,ctx.height }, data);
    
    debug_document(data);
    
    while (!should_close_ctx()){
        fb_clear(&ctx, 0);
        render_document(&ctx, data);
        commit_draw_ctx(&ctx);
    }
    
    destroy_draw_ctx(&ctx);
    
}