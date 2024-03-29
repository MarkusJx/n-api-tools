#include <iostream>
#include <napi.h>
#include "napi_tools.hpp"

using namespace napi_tools;

Napi::Promise promiseTest(const Napi::CallbackInfo &info) {
    TRY
        return promises::promise<std::string>(info.Env(), [] {
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

    static custom_t fromNapiValue(const Napi::Env &env, const Napi::Value &val) {
        auto o = val.ToObject();
        return custom_t{o.Get("a").ToString(), o.Get("b").ToString()};
    }
};

static callbacks::callback<void()> callback = nullptr;
static callbacks::callback<int(int)> int_callback = nullptr;
static callbacks::callback<int(std::vector<std::string>)> vec_callback = nullptr;
static callbacks::callback<custom_t(custom_t)> custom_callback = nullptr;
static callbacks::callback<void(std::string)> str_callback = nullptr;
static callbacks::callback<std::shared_ptr<std::promise<int>>()> promise_callback = nullptr;

void setCallback(const Napi::CallbackInfo &info) {
    TRY
        callback = callbacks::callback<void()>(info);
    CATCH_EXCEPTIONS
}

void setIntCallback(const Napi::CallbackInfo &info) {
    TRY
        int_callback = callbacks::callback<int(int)>(info, [](Napi::Env env, int i) -> std::vector<napi_value> {
            return {Napi::Number::New(env, i)};
        });
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
        return promises::promise<void>(info.Env(), [] {
            callback();
            str_callback("some string");
            std::promise<custom_t> pr;
            custom_callback({"def", "ghi"}, pr);
            auto ft = pr.get_future();
            ft.wait();
            std::cout << "Custom callback: " << ft.get().toString() << std::endl;
            int_callback(42, [](int i) {
                std::cout << "Callback returned: " << i << std::endl;
            }, [] (const std::exception &e) {
                std::cerr << "Exception thrown: " << e.what() << std::endl;
                exit(1);
            });
            std::future<int> fut = int_callback(42);
            fut.wait();
            std::cout << "Callback returned: " << fut.get() << std::endl;
            auto f = vec_callback({"a", "b", "c", "d", "e", "f"});
            f.wait();
            std::cout << "Vec Callback returned: " << f.get() << std::endl;
        });
    CATCH_EXCEPTIONS
}

void promiseCallback(const Napi::CallbackInfo &info) {
    std::thread([] {
        auto fut = promise_callback().get()->get_future();
        if (fut.wait_for(std::chrono::milliseconds(3000)) == std::future_status::timeout) {
            std::cerr << "The promise was not resolved in time" << std::endl;
            return;
        }

        std::cout << "Promise callback returned: " << fut.get() << std::endl;
    }).detach();
}

void stopCallback(const Napi::CallbackInfo &info) {
    TRY
        callback.stop();
    CATCH_EXCEPTIONS
}

void checkNullOrUndefined(const Napi::CallbackInfo &info) {
    CHECK_ARGS(undefined | string | null);
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    EXPORT_FUNCTION(exports, env, promiseTest);
    EXPORT_FUNCTION(exports, env, setCallback);
    EXPORT_FUNCTION(exports, env, setIntCallback);
    EXPORT_FUNCTION(exports, env, setVecCallback);
    EXPORT_FUNCTION(exports, env, setCustomCallback);
    EXPORT_FUNCTION(exports, env, callMeMaybe);
    EXPORT_FUNCTION(exports, env, stopCallback);
    EXPORT_FUNCTION(exports, env, checkNullOrUndefined);
    EXPORT_FUNCTION(exports, env, promiseCallback);
    str_callback.exportSetter(env, exports, "setStrCallback");
    promise_callback.exportSetter(env, exports, "setPromiseCallback");

    return exports;
}

NODE_API_MODULE(napi_tools, InitAll)