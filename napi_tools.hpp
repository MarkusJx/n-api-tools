/*
 * napi_tools.hpp
 *
 * Licensed under the MIT License
 *
 * Copyright (c) 2020 MarkusJx
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef NAPI_TOOLS_NAPI_TOOLS_HPP
#define NAPI_TOOLS_NAPI_TOOLS_HPP

#include <napi.h>
#include <thread>
#include <memory>

#define TRY try {
#define CATCH_EXCEPTIONS                                                 \
    } catch (const std::exception& e) {                                  \
        throw Napi::Error::New(info.Env(), e.what());                    \
    } catch (...) {                                                      \
        throw Napi::Error::New(info.Env(), "An unknown error occurred"); \
    }

#define CHECK_ARGS(...)            \
    ::napi_tools::util::checkArgs( \
        info, ::napi_tools::util::removeNamespace(__FUNCTION__), { __VA_ARGS__ })
#define CHECK_LENGTH(len)       \
    if (info.Length() != len)   \
    throw Napi::TypeError::New( \
        info.Env(),             \
        ::napi_tools::util::removeNamespace(__FUNCTION__) + " requires " + std::to_string(len) + " arguments")

// Export a n-api function with the name of func, an environment and the exports variable
#define EXPORT_FUNCTION(exports, env, func) exports.Set(#func, ::Napi::Function::New(env, func))

namespace napi_tools {
    enum napi_type {
        STRING,
        NUMBER,
        FUNCTION,
        OBJECT,
        BOOLEAN,
        ARRAY
    };

    /**
     * Utility namespace
     */
    namespace util {
        inline std::string removeNamespace(const std::string &str) {
            return str.substr(str.rfind(':') + 1);
        }

        inline void
        checkArgs(const Napi::CallbackInfo &info, const std::string &funcName, const std::vector<napi_type> &types) {
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
    } // namespace util

    namespace promises {
        /**
         * A class for creating js promises. This class must exist since the original
         * n-api is so bad and cannot provide such a simple behaviour by default. Also,
         * the docs on Promises are worth shit, just as a side note. You will need to
         * look at the examples to find this, why not?
         *
         * Source:
         * https://github.com/nodejs/node-addon-examples/issues/85#issuecomment-583887294
         * Also exists here:
         * https://github.com/nodejs/node-addon-examples/blob/master/async_pi_estimate/node-addon-api/async.cc
         */
        class AsyncWorker : public Napi::AsyncWorker {
        protected:
            /**
             * Construct a Promise
             *
             * @param env the environment to work in
             * @param _fn the function to call
             */
            inline explicit AsyncWorker(const Napi::Env &env) : Napi::AsyncWorker(env),
                                                                deferred(Napi::Promise::Deferred::New(env)) {
            }

            /**
             * A default destructor
             */
            inline ~AsyncWorker() override = default;

            virtual void Run() = 0;

            /**
             * The execution thread
             */
            inline void Execute() override {
                try {
                    Run();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                } catch (std::exception &e) {
                    Napi::AsyncWorker::SetError(e.what());
                } catch (...) {
                    Napi::AsyncWorker::SetError("An unknown error occurred");
                }
            }

            /**
             * Default on ok
             */
            inline virtual void OnOK() override {
                deferred.Resolve(Env().Undefined());
            }

            /**
             * On error
             *
             * @param error the error to throw
             */
            inline void OnError(const Napi::Error &error) override {
                deferred.Reject(error.Value());
            }

            /**
             * Get the promise
             *
             * @return a Napi::Promise
             */
            inline Napi::Promise GetPromise() {
                return deferred.Promise();
            }

            Napi::Promise::Deferred deferred;
        };

        /**
         * A class for creating Promises with return types
         *
         * @tparam T the return type of the operation
         */
        template<typename T>
        class Promise : public AsyncWorker {
        public:
            /**
             * Create a javascript promise
             *
             * @param env the environment of the promise
             * @param fn the function to call. Must return T.
             * @return a Napi::Promise
             */
            static Napi::Promise create(const Napi::Env &env, const std::function<T()> &fn) {
                auto *promise = new Promise<T>(env, fn);
                promise->Queue();

                return promise->GetPromise();
            }

        protected:
            /**
             * Construct a Promise
             *
             * @param env the environment to work in
             * @param _fn the function to call
             */
            inline Promise(const Napi::Env &env, std::function<T()> _fn) : AsyncWorker(env), fn(std::move(_fn)) {
            }

            /**
             * A default destructor
             */
            inline ~Promise() override = default;

            /**
             * The execution thread
             */
            inline void Run() override { val = fn(); }

            /**
             * On ok
             */
            inline void OnOK() override {
                deferred.Resolve(Napi::Value::From(Env(), val));
            };

        private:
            std::function<T()> fn;
            T val;
        };

        /**
         * A class for creating Promises with no return type
         */
        template<>
        class Promise<void> : public AsyncWorker {
        public:
            /**
             * Create a javascript promise
             *
             * @param env the environment of the promise
             * @param fn the function to call. Must return T.
             * @return a Napi::Promise
             */
            static Napi::Promise create(const Napi::Env &env, const std::function<void()> &fn) {
                auto *promise = new Promise(env, fn);
                promise->Queue();

                return promise->GetPromise();
            }

        protected:
            /**
             * Construct a Promise
             *
             * @param env the environment to work in
             * @param _fn the function to call
             */
            inline Promise(const Napi::Env &env, std::function<void()> _fn) : AsyncWorker(env), fn(std::move(_fn)) {}

            /**
             * A default destructor
             */
            inline ~Promise() override = default;

            /**
             * The run function
             */
            inline void Run() override {
                fn();
            }

        private:
            std::function<void()> fn;
        };
    } // namespace promises

    /**
     * A namespace for callbacks
     */
    namespace callbacks {
        /**
         * A javascript callback
         */
        template<class>
        class javascriptCallback;

        /**
         * A javascript callback with return type
         *
         * @tparam R the return type
         * @tparam A the argument types
         */
        template<class R, class...A>
        class javascriptCallback<R(A...)> {
        public:
            explicit inline javascriptCallback(const Napi::CallbackInfo &info) : deferred(
                    Napi::Promise::Deferred::New(info.Env())), mtx() {
                CHECK_ARGS(::napi_tools::napi_type::FUNCTION);
                Napi::Env env = info.Env();

                run = true;

                // Create a new ThreadSafeFunction.
                this->ts_fn =
                        Napi::ThreadSafeFunction::New(env, info[0].As<Napi::Function>(), "javascriptCallback", 0, 1,
                                                      this, FinalizerCallback < R, A... >, (void *) nullptr);
                this->nativeThread = std::thread(threadEntry < R, A... >, this);
            }

            inline void asyncCall(A &&...values, const std::function<void(R)> &func) {
                mtx.lock();
                queue.push_back(new args(std::forward<A>(values)..., func));
                mtx.unlock();
            }

            [[nodiscard]] inline Napi::Promise getPromise() const {
                return deferred.Promise();
            }

            inline void stop() {
                run = false;
                mtx.unlock();
            }

        private:
            class args {
            public:
                inline args(A &&...values, const std::function<void(R)> &func) : args_t(std::forward<A>(values)...),
                                                                                 fun(func) {}

                /**
                 * Convert the args to a napi_value vector.
                 * Source: https://stackoverflow.com/a/42495119
                 *
                 * @param env the environment to work in
                 * @return the value vector
                 */
                inline std::vector<napi_value> to_vector(const Napi::Env &env) {
                    return std::apply([&env](auto &&... el) {
                        return std::vector<napi_value>{Napi::Value::From(env, std::forward<decltype(el)>(el))...};
                    }, std::forward<std::tuple<A...>>(args_t));
                }

                std::function<void(R)> fun;
            private:
                std::tuple<A...> args_t;
            };

            template<class T, class...Args>
            static constexpr bool is_any_of = std::disjunction_v<std::is_same<T, Args>...>;

            template<template<class> class T, class Elem>
            static T<Elem> arrayToVector(const Napi::Value &val) {
                static_assert(std::is_same_v<T, std::vector>);
                T<Elem> vec;
                auto arr = val.As<Napi::Array>();
                for (int i = 0; i < arr.Length(); i++) {
                    vec.push_back(valueToCppVal<Elem>(arr.Get(i)));
                }

                return vec;
            }

            template<class T>
            static T valueToCppVal(const Napi::Value &val) {
                if constexpr (is_any_of<T, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>) {
                    if (!val.IsNumber()) throw std::runtime_error("The given type is not a number");
                    else return val.ToNumber();
                } else if constexpr (is_any_of<T, std::string, const char *>) {
                    if (!val.IsString()) throw std::runtime_error("The given type is not a string");
                    else return val.ToString();
                } else if constexpr (is_any_of<T, bool>) {
                    if (!val.IsBoolean()) throw std::runtime_error("The given type is not a boolean");
                    else return val.ToBoolean();
                } else if constexpr (is_any_of<T, std::vector>) {
                    if (!val.IsArray()) throw std::runtime_error("The given type is not an array");
                    else return arrayToVector<T>(val);
                }
                // TODO: maps
            }

            template<class U, class...Args>
            static void threadEntry(javascriptCallback<U(Args...)> *jsCallback) {
                //U ret;
                const auto callback = [](Napi::Env env, Napi::Function jsCallback, args *data) {
                    //data->mtx.lock();
                    Napi::Value val = jsCallback.Call(data->to_vector(env));

                    try {
                        U ret = valueToCppVal<U>(val);
                        data->fun(ret);
                    } catch (std::exception &e) {
                        delete data;
                        throw Napi::Error::New(env, e.what());
                    }
                    delete data;
                };

                while (jsCallback->run) {
                    jsCallback->mtx.lock();
                    if (jsCallback->run) {
                        for (args *ar : jsCallback->queue) {
                            auto *a = new args(*ar);
                            delete ar;

                            napi_status status = jsCallback->ts_fn.BlockingCall(a, callback);

                            if (status != napi_ok) {
                                Napi::Error::Fatal("ThreadEntry",
                                                   "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
                            }
                        }
                        jsCallback->queue.clear();
                        jsCallback->mtx.unlock();

                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    } else {
                        jsCallback->mtx.unlock();
                    }
                }

                jsCallback->ts_fn.Release();
            }

            template<class U, class...Args>
            static void FinalizerCallback(Napi::Env env, void *, javascriptCallback<U(Args...)> *jsCallback) {
                jsCallback->nativeThread.join();

                jsCallback->deferred.Resolve(env.Null());
                delete jsCallback;
            }

            ~javascriptCallback() {
                mtx.lock();
                for (args *a : queue) {
                    delete a;
                }
            }

            bool run;
            std::mutex mtx;
            std::vector<args *> queue;
            const Napi::Promise::Deferred deferred;
            std::thread nativeThread;
            Napi::ThreadSafeFunction ts_fn;
        };

        /**
         * A void javascript callback
         *
         * @tparam A the argument types
         */
        template<class...A>
        class javascriptCallback<void(A...)> {
        public:
            /**
             * Create a javascript callback
             *
             * @param info the CallbackInfo. info[0] must be a napi function
             */
            explicit inline javascriptCallback(const Napi::CallbackInfo &info) : deferred(
                    Napi::Promise::Deferred::New(info.Env())), queue(), mtx() {
                CHECK_ARGS(::napi_tools::napi_type::FUNCTION);
                Napi::Env env = info.Env();

                run = true;

                // Create a new ThreadSafeFunction.
                this->ts_fn = Napi::ThreadSafeFunction::New(env, info[0].As<Napi::Function>(), "javascriptCallback", 0,
                                                            1, this,
                                                            FinalizerCallback<A...>, (void *) nullptr);
                this->nativeThread = std::thread(threadEntry<A...>, this);
            }

            /**
             * Async call the function
             *
             * @param values the values to pass
             */
            inline void asyncCall(A &&...values) {
                mtx.lock();
                queue.push_back(args(std::forward<A>(values)...));
                mtx.unlock();
            }

            /**
             * Get the napi promise
             *
             * @return the promise
             */
            [[nodiscard]] inline Napi::Promise getPromise() const {
                return deferred.Promise();
            }

            /**
             * Stop the function and deallocate all ressources
             */
            inline void stop() {
                run = false;
                mtx.unlock();
            }

        private:
            class args {
            public:
                args(A &&...values) : args_t(std::forward<A>(values)...) {}

                /**
                 * Convert the args to a napi_value vector.
                 * Source: https://stackoverflow.com/a/42495119
                 *
                 * @param env the environment to work in
                 * @return the value vector
                 */
                inline std::vector<napi_value> to_vector(const Napi::Env &env) {
                    return std::apply([&env](auto &&... el) {
                        return std::vector<napi_value>{Napi::Value::From(env, std::forward<decltype(el)>(el))...};
                    }, std::forward<std::tuple<A...>>(args_t));
                }

            private:
                std::tuple<A...> args_t;
            };

            template<class...Args>
            static void threadEntry(javascriptCallback<void(Args...)> *jsCallback) {
                auto callback = [](Napi::Env env, Napi::Function jsCallback, args *data) {
                    jsCallback.Call(data->to_vector(env));
                    delete data;
                };

                while (jsCallback->run) {
                    jsCallback->mtx.lock();
                    if (jsCallback->run) {
                        for (const args &val : jsCallback->queue) {
                            args *tmp = new args(val);
                            napi_status status = jsCallback->ts_fn.BlockingCall(tmp, callback);

                            if (status != napi_ok) {
                                Napi::Error::Fatal("ThreadEntry",
                                                   "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
                            }
                        }
                        jsCallback->queue.clear();
                        jsCallback->mtx.unlock();
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    } else {
                        jsCallback->mtx.unlock();
                    }
                }

                jsCallback->ts_fn.Release();
            }

            template<class...Args>
            static void FinalizerCallback(Napi::Env env, void *, javascriptCallback<void(Args...)> *jsCallback) {
                jsCallback->nativeThread.join();

                jsCallback->deferred.Resolve(env.Null());
                delete jsCallback;
            }

            ~javascriptCallback() noexcept = default;

            bool run;
            std::mutex mtx;
            std::vector<args> queue;
            const Napi::Promise::Deferred deferred;
            std::thread nativeThread;
            Napi::ThreadSafeFunction ts_fn;
        };

        /**
         * Utility namespace
         */
        namespace util {
            /**
             * The callback template
             *
             * @tparam T the javascriptCallback class type
             */
            template<class T>
            class callback_template {
            public:
                /**
                 * Construct an empty callback function.
                 * Will throw an exception when trying to call.
                 */
                inline callback_template() noexcept: ptr(nullptr) {}

                /**
                 * Construct an empty callback function.
                 * Will throw an exception when trying to call.
                 */
                inline callback_template(std::nullptr_t) noexcept: ptr(nullptr) {}

                /**
                 * Construct a callback function
                 *
                 * @param info the CallbackInfo with typeof info[0] == 'function'
                 */
                inline explicit callback_template(const Napi::CallbackInfo &info) : ptr(new wrapper(info)) {}

                /**
                 * Get the underlying promise
                 *
                 * @return the promise
                 */
                inline Napi::Promise getPromise() {
                    if (ptr && !ptr->stopped) {
                        ptr->fn->getPromise();
                    } else {
                        throw std::runtime_error("Callback was never initialized");
                    }
                }

                /**
                 * Get the underlying promise
                 *
                 * @return the promise
                 */
                inline operator Napi::Promise() {
                    return this->getPromise();
                }

                /**
                 * Get the underlying promise
                 *
                 * @return the promise
                 */
                inline operator Napi::Value() {
                    return this->operator Napi::Promise();
                }

                /**
                 * Stop the callback function and deallocate all resources
                 */
                inline void stop() {
                    if (ptr && !ptr->stopped) {
                        ptr->fn->stop();
                        ptr->stopped = true;
                    }
                }

                /**
                 * Default destructor
                 */
                inline ~callback_template() noexcept = default;

            protected:
                /**
                 * A class for wrapping around the javascriptCallback class
                 */
                class wrapper {
                public:
                    /**
                     * Create a wrapper instance
                     *
                     * @param info the callbackInfo to construct the callback
                     */
                    inline wrapper(const Napi::CallbackInfo &info) : fn(new T(info)),
                                                                     stopped(false) {}

                    /**
                     * Stop the callback
                     */
                    inline ~wrapper() {
                        if (!stopped) fn->stop();
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }

                    T *fn;
                    bool stopped;
                };

                /**
                 * The ptr holding the wrapper
                 */
                std::shared_ptr<wrapper> ptr;
            };
        } // namespace util

        /**
         * A class for creating javascript callbacks
         */
        template<class>
        class callback;

        /**
         * A void javascript callback
         *
         * @tparam Args the argument types
         */
        template<class...Args>
        class callback<void(Args...)> : public util::callback_template<javascriptCallback<void(Args...)>> {
        public:
            using cb_template = util::callback_template<javascriptCallback<void(Args...)>>;
            using cb_template::cb_template;

            /**
             * Call the javascript function. Async call
             *
             * @param args the function arguments
             */
            inline void operator()(Args...args) {
                if (this->ptr && !this->ptr->stopped) {
                    this->ptr->fn->asyncCall(std::forward<Args>(args)...);
                } else {
                    throw std::runtime_error("Callback was never initialized");
                }
            }
        };

        /**
         * A non-void javascript callback
         *
         * @tparam R the return type
         * @tparam Args the argument types
         */
        template<class R, class...Args>
        class callback<R(Args...)> : public util::callback_template<javascriptCallback<R(Args...)>> {
        public:
            using cb_template = util::callback_template<javascriptCallback<R(Args...)>>;
            using cb_template::cb_template;

            /**
             * Call the javascript function async.
             *
             * @param args the function arguments
             * @param callback the callback function to be called, as this is async
             */
            inline void operator()(Args...args, const std::function<void(R)> &callback) {
                if (this->ptr && !this->ptr->stopped) {
                    this->ptr->fn->asyncCall(std::forward<Args>(args)..., callback);
                } else {
                    throw std::runtime_error("Callback was never initialized");
                }
            }
        };
    } // namespace callbacks
} // namespace napi_tools
#endif // NAPI_TOOLS_NAPI_TOOLS_HPP
