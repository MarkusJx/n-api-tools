#include <iostream>

// Include napi.h before napi_tools.hpp to enable n-api support
#include <napi.h>
#include "napi_tools.hpp"

using namespace napi_tools;

Napi::String testString(const Napi::CallbackInfo &info) {
    CHECK_ARGS(STRING);

    TRY
        var s;
        s = 5;
        s = info[0];
        std::cout << s->toString() << std::endl;
        s = 5;
        s += 5;

        s += "some string";
        std::cout << s->toString() << std::endl;

        // Now possible: string creation and concatenation. You may want to ask: "y tho". And honestly, I don't know either
        // why anyone would want such a feature in c++. But if you like a lot of weird errors, you're welcome.
        // At this point, i'm really questioning my existence. Weird.
        s += 54;
        std::cout << s->toString() << std::endl;

        s = s + "a " + true;
        std::cout << s->toString() << std::endl;

        return s.asString()->toNapiString(info.Env());
    CATCH_EXCEPTIONS
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    exports.Set("testString", Napi::Function::New(env, testString));

    return exports;
}

NODE_API_MODULE(node_aot, InitAll)