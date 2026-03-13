#pragma once

#include "string/string.h"
#include "string/slice.h"
#include "keyboard_input.h"

void uno_text_field(string *content, string_slice placeholder);

void uno_dispatch_kbd(kbd_event ev);