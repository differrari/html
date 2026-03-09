#include "doc.h"
#include "syscalls/syscalls.h"
#include "front.h"
#include "input_keycodes.h"

int main(){
    
    draw_ctx ctx = {.width = 1920, .height = 1080};
    
    request_draw_ctx(&ctx);
    
    uno_begin_vertical((node_info){ doc_layout_vertical, doc_gen_layout, .sizing_rule = size_fill, .bg_color = 0xFF123456 + 0x050505 });
        for (int y = 0; y < 3; y++){
            uno_begin_horizontal((node_info){ .type = doc_layout_horizontal, .general_type = doc_gen_layout, .sizing_rule = size_fill});
                for (int x = 0; x < 3; x++){
                    uno_begin_depth((node_info){.bg_color = 0xFF123456 + 0x333333, .sizing_rule = size_fill, .padding = 4});
                        uno_create_empty_view((node_info){.bg_color = 0xFF123456 + 0x111111, .sizing_rule = size_fill, .padding = 5});
                        uno_create_view((node_info){ .type = doc_text_caption, .general_type = doc_gen_text, .sizing_rule = size_fill, .fg_color = 0xFFFFFFFF, .padding = 5},
                            slice_from_literal("red"));
                        uno_create_view((node_info){ .type = doc_text_title, .general_type = doc_gen_text, .fg_color = 0xFFFFFFFF, .sizing_rule = size_fill,.horiz_alignment = horizontal_center,.vert_alignment = vertical_center},
                            slice_from_string(string_format("%i",(y *3)+x)));
                    uno_end_depth();
                }
            uno_end_horizontal();
        }
    uno_end_vertical();
    
    layout_document((gpu_rect){ 0,0,ctx.width,ctx.height }, default_doc_data);
    
    debug_document(default_doc_data);
    
    while (!should_close_ctx()){
        fb_clear(&ctx, 0);
        render_document(&ctx, default_doc_data);
        commit_draw_ctx(&ctx);
        kbd_event ev = {};
        if (read_event(&ev) && ev.key == KEY_ESC) return 0;
    }
    
    destroy_draw_ctx(&ctx);
    
}