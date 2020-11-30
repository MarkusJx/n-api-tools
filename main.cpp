#include <iostream>
#include <napi.h>
#include <sstream>
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

class custom_t {
public:
    std::string a, b;

    [[nodiscard]] std::string toString() const {
        std::stringstream ss;
        ss << "a: " << a << ", b: " << b;

        return ss.str();
    }

    static Napi::Value toNapiValue(const Napi::Env &env, const custom_t &c) {
        auto o = Napi::Object::New(env);
        o.Set("a", Napi::Value::From(env, c.a));
        o.Set("b", Napi::Value::From(env, c.b));
        return o;
    }

    static custom_t fromNapiValue(const Napi::Value &val) {
        auto o = val.ToObject();
        return custom_t{o.Get("a").ToString(), o.Get("b").ToString()};
    }
};

static callbacks::callback<void()> callback = nullptr;
static callbacks::callback<int(int)> int_callback = nullptr;
static callbacks::callback<int(std::vector<std::string>)> vec_callback = nullptr;
static callbacks::callback<custom_t(custom_t)> custom_callback = nullptr;

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

void setVecCallback(const Napi::CallbackInfo &info) {
    TRY
        vec_callback = callbacks::callback<int(std::vector<std::string>)>(info);
    CATCH_EXCEPTIONS
}

void setCustomCallback(const Napi::CallbackInfo &info) {
    TRY
        custom_callback = callbacks::callback<custom_t(custom_t)>(info);
    CATCH_EXCEPTIONS
}

Napi::Promise callMeMaybe(const Napi::CallbackInfo &info) {
    TRY
        return promises::Promise<void>::create(info.Env(), [] {
            callback();
            auto pr = custom_callback({"def", "ghi"});
            auto ft = pr.get_future();
            ft.wait();
            std::cout << "Custom callback: " << ft.get().toString() << std::endl;
            int_callback(42, [](int i) {
                std::cout << "Callback returned: " << i << std::endl;
            });
            std::promise<int> promise = int_callback(42);
            std::future<int> fut = promise.get_future();
            fut.wait();
            std::cout << "Callback returned: " << fut.get() << std::endl;
            auto p = vec_callback({"a", "b", "c", "d", "e", "f"});
            auto f = p.get_future();
            f.wait();
            std::cout << "Vec Callback returned: " << f.get() << std::endl;
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
    EXPORT_FUNCTION(exports, env, setVecCallback);
    EXPORT_FUNCTION(exports, env, setCustomCallback);
    EXPORT_FUNCTION(exports, env, callMeMaybe);
    EXPORT_FUNCTION(exports, env, stopCallback);

    return exports;
}

NODE_API_MODULE(napi_tools, InitAll)