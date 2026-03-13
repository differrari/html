#include "uno_helpers.h"
#include "uno.h"

void uno_text_field(string *content, string_slice placeholder){
    uno_create_view((node_info){.general_type = doc_gen_text, .type = doc_text_footnote, .percentage = 0.1f, .fg_color = 0xFFFFFFFF, .padding = 5}, content->data && content->length ? slice_from_string(*content) : placeholder);
}

void uno_dispatch_kbd(kbd_event ev){
    //STUB: Will need to pass the keyboard input to the correct instance of a type (only text_field rn, but also buttons, forms, etc)
    //and its type should handle the input (text field uses its *content to expand/reduce and to set itself to use placeholder)
    
}