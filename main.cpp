#include <iostream>
#include <napi.h>
#include "napi_tools.hpp"

using namespace napi_tools;

Napi::Promise promiseTest(const Napi::CallbackInfo &info) {
    TRY
        return promises::Promise<std::string>::create(info.Env(), [] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            return "abc";
        });
    CATCH_EXCEPTIONS
}

static callbacks::callback<void()> callback = nullptr;

//callbacks::javascriptCallback<int> *callback;

void setCallback(const Napi::CallbackInfo &info) {
    TRY
        callback = callbacks::callback<void()>(info);
        //return callback->getPromise();
    CATCH_EXCEPTIONS
}

void callMeMaybe(const Napi::CallbackInfo &info) {
    TRY
        callback();
        callback();
    CATCH_EXCEPTIONS
}

void stopCallback(const Napi::CallbackInfo &info) {
    TRY
        callback.stop();
    CATCH_EXCEPTIONS
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    EXPORT_FUNCTION(exports, env, promiseTest);
    EXPORT_FUNCTION(exports, env, setCallback);
    EXPORT_FUNCTION(exports, env, callMeMaybe);
    EXPORT_FUNCTION(exports, env, stopCallback);

    return exports;
}

NODE_API_MODULE(napi_tools, InitAll)