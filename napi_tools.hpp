#ifndef NAPI_TOOLS_NAPI_TOOLS_HPP
#define NAPI_TOOLS_NAPI_TOOLS_HPP

#include "var_type.hpp"

#define call(name, object, ...) Get(name).As<Napi::Function>().Call(object, {__VA_ARGS__})
#define create(...) As<Napi::Function>().New({__VA_ARGS__})

#define TRY try {
#define CATCH_EXCEPTIONS } catch (const std::exception &e) {throw Napi::Error::New(info.Env(), e.what());}\
        catch (...) {throw Napi::Error::New(info.Env(), "An unknown error occurred");}

#define CHECK_ARGS(...) ::napi_tools::util::checkArgs(info, ::napi_tools::util::removeNamespace(__FUNCTION__), {__VA_ARGS__})
#define CHECK_LENGTH(len) if (info.Length() != len) throw Napi::TypeError::New(info.Env(), \
        ::napi_tools::util::removeNamespace(__FUNCTION__) + " requires " + std::to_string(len) + " arguments")

namespace napi_tools {
    enum type {
        STRING,
        NUMBER,
        FUNCTION,
        OBJECT,
        BOOLEAN,
        ARRAY
    };

    /**
     * Require a node.js object
     *
     * @param env the environment to work with
     * @param toRequire the package to require
     * @return the acquired package object
     */
    inline Napi::Object require(const Napi::Env &env, const ::var_type::raw::string &toRequire) {
        return env.Global().call("require", env.Global(), toRequire.toNapiString(env)).As<Napi::Object>();
    }

    class ThreadSafeFunction {
    public:
        inline ThreadSafeFunction(const Napi::ThreadSafeFunction &fn) : ts_fn(fn) {}

        // This API may be called from any thread.
        inline void blockingCall() const {
            napi_status status = ts_fn.BlockingCall();

            if (status != napi_ok) {
                Napi::Error::Fatal("ThreadEntry", "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
            }
        }

        // This API may be called from any thread.
        template<typename Callback>
        inline void blockingCall(Callback callback) const {
            napi_status status = ts_fn.BlockingCall(callback);

            if (status != napi_ok) {
                Napi::Error::Fatal("ThreadEntry", "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
            }
        }

        // This API may be called from any thread.
        template<typename DataType, typename Callback>
        inline void blockingCall(DataType *data, Callback callback) const {
            napi_status status = ts_fn.BlockingCall(data, callback);

            if (status != napi_ok) {
                Napi::Error::Fatal("ThreadEntry", "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
            }
        }

    private:
        const Napi::ThreadSafeFunction &ts_fn;
    };

    using thread_entry = std::function<::var_type::js_object(const ThreadSafeFunction &)>;

    // TODO: Fix this
    class Promise {
    public:
        inline static Napi::Promise New(const Napi::Env &env, const thread_entry &function) {
            Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
            new Promise(env, deferred, function);

            return deferred.Promise();
        }

    private:
        inline Promise(const Napi::Env &env, const Napi::Promise::Deferred &deferred, const thread_entry &function)
                : deferred(
                deferred),
                  entry(function) {
            ts_fn = Napi::ThreadSafeFunction::New(env, Napi::Function::New(env, [](const Napi::CallbackInfo &info) {}),
                                                  "Promise", 0, 1, this,
                                                  [](Napi::Env env, void *finalizeData, Promise *context) {
                                                      context->FinalizerCallback(env, finalizeData);
                                                  }, (void *) nullptr);

            nativeThread = std::thread([](Promise *context) {
                context->threadEntry();
            }, this);
        }

        inline void threadEntry() {
            ThreadSafeFunction threadSafeFunction(ts_fn);
            result = entry(threadSafeFunction);

            // Release the thread-safe function. This decrements the internal thread
            // count, and will perform finalization since the count will reach 0.
            ts_fn.Release();
        }

        inline void FinalizerCallback(Napi::Env env, void *) {
            // Join the thread
            nativeThread.join();

            // Resolve the Promise previously returned to JS via the CreateTSFN method.
            deferred.Resolve(result->getValue(env));

            //delete this;
        }

        inline ~Promise() = default;

        thread_entry entry;
        ::var_type::js_object result;
        Napi::Promise::Deferred deferred;
        std::thread nativeThread;
        Napi::ThreadSafeFunction ts_fn;
    };

    /**
     * Utility namespace
     */
    namespace util {
        inline std::string removeNamespace(const std::string &str) {
            return str.substr(str.rfind(':') + 1);
        }

        inline void
        checkArgs(const Napi::CallbackInfo &info, const std::string &funcName, const std::vector<type> &types) {
            Napi::Env env = info.Env();
            if (info.Length() < types.size()) {
                throw Napi::TypeError::New(env, funcName + " requires " + std::to_string(types.size()) + " arguments");
            }

            for (size_t i = 0; i < types.size(); i++) {
                if (types[i] == STRING) {
                    if (!info[i].IsString()) {
                        throw Napi::TypeError::New(env, "Argument type mismatch: " + funcName +
                                                        " requires type string at position " + std::to_string(i + 1));
                    }
                } else if (types[i] == NUMBER) {
                    if (!info[i].IsNumber()) {
                        throw Napi::TypeError::New(env, "Argument type mismatch: " + funcName +
                                                        " requires type number at position " + std::to_string(i + 1));
                    }
                } else if (types[i] == FUNCTION) {
                    if (!info[i].IsFunction()) {
                        throw Napi::TypeError::New(env, "Argument type mismatch: " + funcName +
                                                        " requires type function at position " + std::to_string(i + 1));
                    }
                } else if (types[i] == OBJECT) {
                    if (!info[i].IsObject()) {
                        throw Napi::TypeError::New(env, "Argument type mismatch: " + funcName +
                                                        " requires type object at position " + std::to_string(i + 1));
                    }
                } else if (types[i] == BOOLEAN) {
                    if (!info[i].IsBoolean()) {
                        throw Napi::TypeError::New(env, "Argument type mismatch: " + funcName +
                                                        " requires type boolean at position " + std::to_string(i + 1));
                    }
                } else if (types[i] == ARRAY) {
                    if (!info[i].IsArray()) {
                        throw Napi::TypeError::New(env, "Argument type mismatch: " + funcName +
                                                        " requires type array at position " + std::to_string(i + 1));
                    }
                }
            }
        }
    }

    /**
     * js console
     */
    namespace console {
        /**
         * Log a message
         *
         * @param env the environment to work in
         * @param args the arguments
         */
        [[maybe_unused]] inline void log(const Napi::Env &env, const std::vector<napi_value> &args) {
            auto console = env.Global().Get("console").As<Napi::Object>();
            auto log = console.Get("log").As<Napi::Function>();

            log.Call(console, args);
        }

        /**
         * Log a warning message
         *
         * @param env the environment to work in
         * @param args the arguments
         */
        [[maybe_unused]] inline void warn(const Napi::Env &env, const std::vector<napi_value> &args) {
            auto console = env.Global().Get("console").As<Napi::Object>();
            auto warn = console.Get("warn").As<Napi::Function>();

            warn.Call(console, args);
        }

        /**
         * Log a error message
         *
         * @param env the environment to work in
         * @param args the arguments
         */
        [[maybe_unused]] inline void error(const Napi::Env &env, const std::vector<napi_value> &args) {
            auto console = env.Global().Get("console").As<Napi::Object>();
            auto error = console.Get("error").As<Napi::Function>();

            error.Call(console, args);
        }
    }

    /**
     * js JSON
     */
    namespace JSON {
        /**
         * Stringify a n-api object
         *
         * @param env the environment to work in
         * @param object the object to stringify
         * @return the resulting string
         */
        [[maybe_unused]] inline Napi::String stringify(const Napi::Env &env, const ::var_type::object &object) {
            auto json = env.Global().Get("JSON").As<Napi::Object>();
            auto stringify = json.Get("stringify").As<Napi::Function>();
            return stringify.Call(json, {object->getValue(env)}).As<Napi::String>();
        }

        /**
         * Parse a JSON string
         *
         * @param env the environment to work in
         * @param string the string to parse
         * @return the resulting n-api object
         */
        [[maybe_unused]] inline Napi::Object parse(const Napi::Env &env, const ::var_type::string &string) {
            auto json = env.Global().Get("JSON").As<Napi::Object>();
            auto parse = json.Get("parse").As<Napi::Function>();
            return parse.Call(json, {string->getValue(env)}).As<Napi::Object>();
        }
    }
}
#endif //NAPI_TOOLS_NAPI_TOOLS_HPP
