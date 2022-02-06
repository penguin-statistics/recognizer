#include "recognizer.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(recognizer)
{
    emscripten::function("load_server", &load_server);
    emscripten::function("load_stage_index", &load_stage_index);
    emscripten::function("load_hash_index", &load_hash_index);
    emscripten::function("load_templ", &load_templ);
    emscripten::function("env_check", &env_check);
    emscripten::class_<Recognizer>("Recognizer")
        .constructor<std::string>()
        .function("set_iamge", selete_overload<void(uint8_t*, size_t)>(&Recognizer::set_image))
        .function("recognize", &Recognizer::recognize);
}
#endif // __EMSCRIPTEN__