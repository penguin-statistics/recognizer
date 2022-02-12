#ifndef PENGUIN_RECOGNIZER_WASM_CPP_
#define PENGUIN_RECOGNIZER_WASM_CPP_

#include <string>

#include <emscripten.h>
#include <emscripten/bind.h>

#include "recognizer.hpp"

using namespace emscripten;

std::string getExceptionMessage(intptr_t exceptionPtr)
{
    return std::string(reinterpret_cast<std::exception*>(exceptionPtr)->what());
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(recognizer)
{
    function("getExceptionMessage", &getExceptionMessage);
    constant("version", version);
    constant("opencv_version", opencv_version);
    function("load_server", &load_server);
    function("load_stage_index", &wload_stage_index);
    function("load_hash_index", &wload_hash_index);
    function("load_templs", &wload_templs);
    function("env_check", &env_check);
    class_<Recognizer>("Recognizer")
        .constructor<std::string>()
        .function("set_img", &Recognizer::wset_img)
        .function("recognize", &Recognizer::wrecognize)
        .function("get_report", &Recognizer::wget_report)
        .function("get_debug_img", &Recognizer::wget_debug_img);
}
#endif // __EMSCRIPTEN__
#endif // PENGUIN_RECOGNIZER_WASM_CPP_