#ifndef _JS_ESCAPE_H_
#define _JS_ESCAPE_H_
#include <string>
/// @brief Escape text (usually JSON) for use inside a JavaScript string literal in a page,
/// whether the page quotes it with '...', "..." or `...`. Escapes quotes, backslashes and
/// line breaks so the value cannot end the string, '$' so it cannot start a `${...}`
/// substitution, and '<' so it cannot close the <script> element.
static inline std::string escape_for_js_string(const std::string& text){
    std::string out;
    out.reserve(text.size() + 16);
    for(char c : text){
        switch(c){
            case '\\': out += "\\\\"; break;
            case '\'': out += "\\'"; break;
            case '"':  out += "\\\""; break;
            case '`':  out += "\\`"; break;
            case '$':  out += "\\x24"; break;
            case '<':  out += "\\x3c"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            default:   out += c;
        }
    }
    return out;
}
#endif
