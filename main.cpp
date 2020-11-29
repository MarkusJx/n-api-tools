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
static callbacks::callback<int(int)> int_callback = nullptr;

void setCallback(const Napi::CallbackInfo &info) {
    TRY
        callback = callbacks::callback<void()>(info);
    CATCH_EXCEPTIONS
}

void setIntCallback(const Napi::CallbackInfo &info) {
    TRY
        int_callback = callbacks::callback<int(int)>(info);
    CATCH_EXCEPTIONS
}

Napi::Promise callMeMaybe(const Napi::CallbackInfo &info) {
    TRY
        return promises::Promise<void>::create(info.Env(), [] {
            callback();
            int_callback(42, [] (int i) {
                std::cout << "Callback returned: " << i << std::endl;
            });
            std::promise<int> promise = int_callback(42);
            std::future<int> fut = promise.get_future();
            fut.wait();
            std::cout << "Callback returned: " << fut.get() << std::endl;
        });
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
    EXPORT_FUNCTION(exports, env, setIntCallback);
    EXPORT_FUNCTION(exports, env, callMeMaybe);
    EXPORT_FUNCTION(exports, env, stopCallback);

    return exports;
}

NODE_API_MODULE(napi_tools, InitAll)