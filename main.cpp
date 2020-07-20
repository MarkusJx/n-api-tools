#include <iostream>
#include "napi_tools.hpp"

using namespace napi_tools;

Napi::String testString(const Napi::CallbackInfo &info) {
    CHECK_ARGS(STRING);

    var s = 5;
    std::cout << (int) s->getType() << std::endl;
    s = info[0];
    std::cout << (int) s->getType() << std::endl;
    s = 5;
    std::cout << (int) s->getType() << std::endl;
    s = "some string";
    std::cout << (int) s->getType() << std::endl;
    return s.toString()->toNapiString(info.Env());
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    exports.Set("testString", Napi::Function::New(env, testString));

    return exports;
}

NODE_API_MODULE(node_aot, InitAll)