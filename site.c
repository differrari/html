#include "doc.h"
#include "syscalls/syscalls.h"
#include "front.h"
#include "input_keycodes.h"
#include "math/math.h"

void draw_view(){
    VERTICAL(((node_info){ doc_layout_vertical, doc_gen_layout, .sizing_rule = size_fill, .bg_color = 0 }), {
        HORIZONTAL(((node_info){.sizing_rule = size_relative, .percentage = 0.55f, .bg_color = 0}),{
            VERTICAL(((node_info){.sizing_rule = size_relative, .percentage = .4f, .bg_color = 0}),{
                uno_create_view((node_info){.general_type = doc_gen_text, .type = doc_text_title, .fg_color = 0xFFFFFFFF, .sizing_rule = size_relative, .percentage = 2/3.f, .vert_alignment = vertical_center}, slice_from_literal("Space n shit"));
                uno_create_view((node_info){.general_type = doc_gen_text, .type = doc_text_caption, .fg_color = 0xFFFFFFFF, .sizing_rule = size_relative, .percentage = 2/3.f, .vert_alignment = bottom }, slice_from_literal("Space n shit"));
            });
            VERTICAL(((node_info){.sizing_rule = size_fill, .bg_color = 0}),{
                uno_create_view((node_info){.general_type = doc_gen_text, .type = doc_text_body, .fg_color = 0xFFFFFFFF, .sizing_rule = size_relative, .percentage = .5f, .vert_alignment = vertical_center, .horiz_alignment = horizontal_center}, slice_from_literal("Image goes here"));
                uno_create_view((node_info){.general_type = doc_gen_text, .type = doc_text_title, .fg_color = 0xFFFFFFFF, .sizing_rule = size_relative, .percentage = .5f, .vert_alignment = vertical_center, .horiz_alignment = horizontal_center}, slice_from_literal("Some text here"));
            });
        });
        HORIZONTAL(((node_info){.sizing_rule = size_fill, .bg_color = 0}),{
            uno_create_empty_view((node_info){.sizing_rule = size_relative, .percentage = 1/3.f, .bg_color = 0});
            uno_create_empty_view((node_info){.sizing_rule = size_fill, .bg_color = 0});
        });
        // for (int y = 0; y < MAX_ROWS; y++){
        //     HORIZONTAL(((node_info){ .type = doc_layout_horizontal, .general_type = doc_gen_layout, .sizing_rule = size_fill}),{
        //         for (int x = 0; x < MAX_COLS; x++){
        //             DEPTH(((node_info){.bg_color = 0xFF123456 + (selected_x == x && selected_y == y ? 0x333333 : 0x111111), .sizing_rule = size_fill, .padding = 4}),{
        //                 if (selected_x == x && selected_y == y) uno_create_empty_view((node_info){.bg_color = 0xFF123456 + 0x111111, .sizing_rule = size_fill, .padding = 5});
        //                 uno_create_view((node_info){ .type = doc_text_caption, .general_type = doc_gen_text, .sizing_rule = size_fill, .fg_color = 0xFFFFFFFF, .padding = 5},
        //                     slice_from_literal("red"));
        //                 uno_create_view((node_info){ .type = doc_text_title, .general_type = doc_gen_text, .fg_color = 0xFFFFFFFF, .sizing_rule = size_fill,.horiz_alignment = horizontal_center,.vert_alignment = vertical_center},
        //                     slice_from_string(string_format("%i",(y *3)+x)));
        //             });
        //         }
        //     });
        // }
        
        // uno_create_empty_view((node_info){.sizing_rule = size_relative, .percentage = 0.35f, .bg_color = 0xFFb4dd13});
    });
}

int main(){
    
    draw_ctx ctx = {.width = 1920, .height = 1080};
    
    request_draw_ctx(&ctx);
    
    set_document_view(draw_view, (gpu_rect){ 0,0,ctx.width,ctx.height });
    
    debug_document(default_doc_data);
    
    while (!should_close_ctx()){
        fb_clear(&ctx, 0);
        uno_draw(&ctx);
        commit_draw_ctx(&ctx);
        kbd_event ev = {};
        if (read_event(&ev)){
            if (ev.key == KEY_ESC) return 0;  
        } 
    }
    
    destroy_draw_ctx(&ctx);
    
}