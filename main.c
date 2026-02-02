#include "syscalls/syscalls.h"
#include "files/helpers.h"
#include "data/format/scanner/scanner.h"
#include "input_keycodes.h"
#include "data/struct/linked_list.h"

typedef enum { doc_type_none, simple_text, title, subtitle, heading, subheading, h5, h6 } doc_node_type;
typedef enum { doc_gen_type_none, doc_text } doc_gen_type;

typedef struct {
    doc_node_type type;
    doc_gen_type general_type;
} node_info;

typedef struct {
    node_info info;
    clinkedlist_t *contents;
    string_slice content;
} document_node;

uintptr_t x_pos, y_pos;

node_info interpret_tag(string_slice tag){
    if (slice_lit_match(tag, "p", true))
        return (node_info){simple_text,doc_text};
    if (slice_lit_match(tag, "h1", true))
        return (node_info){title,doc_text};
    if (slice_lit_match(tag, "h2", true))
        return (node_info){subtitle,doc_text};
    if (slice_lit_match(tag, "h3", true))
        return (node_info){heading,doc_text};
    if (slice_lit_match(tag, "h4", true))
        return (node_info){subheading,doc_text};
    if (slice_lit_match(tag, "h5", true))
        return (node_info){h5,doc_text};
    if (slice_lit_match(tag, "h6", true))
        return (node_info){h6,doc_text};
    if (slice_lit_match(tag, "script", true)){
        in_case_of_js_break_glass();
    }
    return (node_info){};
}

int text_to_scale(doc_node_type type){
    switch (type) {
        case simple_text:   return 3;
        case title:         return 7;
        case subtitle:      return 6;
        case heading:       return 5;
        case subheading:    return 4;
        case h5:            return 3;
        case h6:            return 2;
        case doc_type_none: return 0;
    }
}

void render_node(draw_ctx *ctx, document_node *node){
    if (node->contents){
        for (clinkedlist_node_t *n = node->contents->head; n; n = n->next)
            render_node(ctx, n->data);
    }
    if (node->content.length){
        switch (node->info.general_type) {
            case doc_text:
            {
                int text_size = text_to_scale(node->info.type);
                if (!text_size) return;
                gpu_size size = fb_draw_slice(ctx, node->content, x_pos, y_pos, text_size, 0xFFFFFFFF);
                y_pos += size.height;
                break;
            }
            default: break;
        }
    }
}

document_node* emit_content(string_slice slice, node_info info){
    document_node* node = zalloc(sizeof(document_node));
    node->content = slice;
    node->info = info;
    return node;
}

document_node* parse_tag(Scanner *s){
    
    document_node* node = zalloc(sizeof(document_node));
    node->contents = clinkedlist_create();
    
    scan_to(s, '<');
    string_slice open = scan_to(s, '>');
    uint32_t in_pos = s->pos;
    open.length--;
    
    node->info = interpret_tag(open);
    if (node->info.type == doc_type_none){
        print("Unknown tag %v",open);
        return node;
    }
    
    uint32_t pos = s->pos;
    
    scan_to(s, '<');
    clinkedlist_push(node->contents, emit_content((string_slice){(char*)s->buf + in_pos, s->pos - in_pos - 1},node->info));
    while (scan_peek(s) != '/'){
        s->pos--;
        print("POS3 %i",s->pos);
        clinkedlist_push(node->contents, parse_tag(s));
        in_pos = s->pos;
        print("POS4 %i",s->pos);
        scan_to(s, '<');
        clinkedlist_push(node->contents, emit_content((string_slice){(char*)s->buf + in_pos, s->pos - in_pos - 1},node->info));
    }
    string_slice close = scan_to(s, '>');
    if (*(char*)close.data != '/'){
        s->pos = pos;
        parse_tag(s);
    }
    close.data++;
    close.length -= 2;
    
    if (!slices_equal(open, close, true)){
        print("Wrong tag buddy");
        return node;
    }
    
    return node;
}

int main(int argc, char* argv[]){
    print("Ola mundo");
    size_t file_size = 0;
    char *file = read_full_file("/resources/index.html", &file_size);
    
    draw_ctx ctx = {};
    ctx.width = 1920;
    ctx.height = 1080;
    request_draw_ctx(&ctx);
    Scanner s = scanner_make(file, file_size);
    
    document_node *root = zalloc(sizeof(document_node));
    root->contents = clinkedlist_create();
    
    while (!scan_eof(&s)){
        clinkedlist_push(root->contents, parse_tag(&s));
    }    
    
    while (!should_close_ctx()){
        begin_drawing(&ctx);
        
        x_pos = 0;
        y_pos = 0;
        
        fb_clear(&ctx, 0);
        
        render_node(&ctx, root);
        
        commit_draw_ctx(&ctx);
        
        kbd_event ev = {};
        if (read_event(&ev) && ev.key == KEY_ESC) return 0;
    }
    
    destroy_draw_ctx(&ctx);
    
    return 0;
    
}