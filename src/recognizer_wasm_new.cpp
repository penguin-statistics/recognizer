#include "recognizer.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(recognizer)
{
    constant("version", version);
    constant("opencv_version", opencv_version);
    function("load_server", &load_server);
    function("load_stage_index", &load_stage_index);
    function("load_hash_index", &load_hash_index);
    function("load_templ", &load_templ);
    function("env_check", &env_check);
    class_<Recognizer>("Recognizer")
        .constructor<std::string>()
        .function("set_image", select_overload<void(std::string)>(&Recognizer::set_image))
        .function("recognize", &Recognizer::recognize)
        .function("get_report", &Recognizer::get_report);
}
#endif // __EMSCRIPTEN__