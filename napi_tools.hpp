/*
 * napi_tools.hpp
 *
 * Licensed under the MIT License
 *
 * Copyright (c) 2020 - 2021 MarkusJx
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
#include <future>
#include <sstream>
#include <map>
#include <iostream>
#include <utility>

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

/**
 * The napi_tools namespace
 */
namespace napi_tools {
    /**
     * Napi types
     */
    enum napi_type {
        string = 0x1,
        number = 0x2,
        function = 0x4,
        object = 0x8,
        boolean = 0x10,
        array = 0x20,
        undefined = 0x40,
        null = 0x80,
        buffer = 0x100,
        promise = 0x200
    };

    /**
     * Utility namespace
     */
    namespace util {
        /**
         * Remove all namespace names from a function name
         *
         * @param str the function name
         * @return the function name without all namespace names
         */
        inline std::string removeNamespace(const std::string &str) {
            return str.substr(str.rfind(':') + 1);
        }

        /**
         * Split a string by a delimiter.
         *
         * Source: https://stackoverflow.com/a/14266139
         */
        inline std::vector<std::string> split_string(std::string str, const std::string &delimiter) {
            size_t pos = 0;
            std::string token;
            std::vector<std::string> res;
            while ((pos = str.find(delimiter)) != std::string::npos) {
                token = str.substr(0, pos);
                res.push_back(token);
                str.erase(0, pos + delimiter.length());
            }

            res.push_back(str);
            return res;
        }

        /**
         * Convert one or multiple napi type(s) to a string
         *
         * @param t the type bits
         * @return the type(s) as a string
         */
        inline std::string napi_type_to_string(uint16_t t) {
            std::stringstream ss;

            const auto print_to_stream = [&ss](const char *c) {
                if (ss.tellp() != std::streampos(0)) {
                    ss << " or ";
                }

                ss << c;
            };

            if (t & napi_type::string) ss << "string";
            if (t & napi_type::number) print_to_stream("number");
            if (t & napi_type::function) print_to_stream("function");
            if (t & napi_type::object) print_to_stream("object");
            if (t & napi_type::boolean) print_to_stream("boolean");
            if (t & napi_type::array) print_to_stream("array");
            if (t & napi_type::undefined) print_to_stream("undefined");
            if (t & napi_type::null) print_to_stream("null");
            if (t & napi_type::buffer) print_to_stream("buffer");
            if (t & napi_type::promise) print_to_stream("promise");

            return ss.str();
        }

        /**
         * Check the argument types of a function
         *
         * @param info the callback info
         * @param funcName the function name
         * @param types the expected argument types
         */
        inline void
        checkArgs(const Napi::CallbackInfo &info, const std::string &funcName, const std::vector<uint16_t> &types) {
            Napi::Env env = info.Env();
            if (info.Length() < types.size()) {
                throw Napi::TypeError::New(env, funcName + " requires " + std::to_string(types.size()) + " arguments");
            }

            // Check the argument type, returns true, if one requirement is satisfied
            const auto check_arg = [](const Napi::Value &val, uint16_t t) {
                if (t & napi_type::string && val.IsString()) return true;
                if (t & napi_type::number && val.IsNumber()) return true;
                if (t & napi_type::function && val.IsFunction()) return true;
                if (t & napi_type::object && val.IsObject()) return true;
                if (t & napi_type::boolean && val.IsBoolean()) return true;
                if (t & napi_type::array && val.IsArray()) return true;
                if (t & napi_type::undefined && val.IsUndefined()) return true;
                if (t & napi_type::null && val.IsNull()) return true;
                if (t & napi_type::buffer && val.IsBuffer()) return true;
                if (t & napi_type::promise && val.IsPromise()) return true;

                return false;
            };

            for (size_t i = 0; i < types.size(); i++) {
                if (!check_arg(info[i], types[i])) {
                    std::stringstream ss;
                    ss << "Argument type mismatch: " << funcName << " requires type " << napi_type_to_string(types[i]);
                    ss << " at position " << (i + 1);
                    throw Napi::TypeError::New(env, ss.str());
                }
            }
        }

        /**
         * A namespace for conversions
         */
        namespace conversions {
            /**
             * A namespace for checking if custom classes can be converted from and to Napi::Value.
             * Source: https://stackoverflow.com/a/16824239
             */
            namespace classes {
                template<typename, typename T>
                struct has_toNapiValue {
                    static_assert(std::integral_constant<T, false>::value,
                                  "Second template parameter needs to be of function type.");
                };

                // Struct to check if C has function toNapiValue(Args...)
                template<typename C, typename Ret, typename... Args>
                struct has_toNapiValue<C, Ret(Args...)> {
                private:
                    template<typename T>
                    static constexpr auto check(T *)
                    -> typename std::is_same<decltype(T::toNapiValue(std::declval<Args>()...)), Ret>::type;

                    template<typename>
                    static constexpr std::false_type check(...);

                    typedef decltype(check<C>(0)) type;

                public:
                    static constexpr bool value = type::value;
                };

                template<typename, typename T>
                struct has_fromNapiValue {
                    static_assert(std::integral_constant<T, false>::value,
                                  "Second template parameter needs to be of function type.");
                };

                // Struct to check if C has function fromNapiValue(Args...)
                template<typename C, typename Ret, typename... Args>
                struct has_fromNapiValue<C, Ret(Args...)> {
                private:
                    template<typename T>
                    static constexpr auto check(T *)
                    -> typename std::is_same<decltype(T::fromNapiValue(std::declval<Args>()...)), Ret>::type;

                    template<typename>
                    static constexpr std::false_type check(...);

                    typedef decltype(check<C>(0)) type;

                public:
                    static constexpr bool value = type::value;
                };
            } // namespace classes

            /**
             * Check if Args... is any of T
             *
             * @tparam T the type to check against
             * @tparam Args the types to check
             */
            template<class T, class...Args>
            static constexpr bool is_any_of = std::disjunction_v<std::is_same<T, Args>...>;

            /**
             * A type converter to convert to cpp values
             */
            template<class>
            struct toCpp;

            /**
             * Convert a Napi::Value to any type
             *
             * @tparam T the type to convert to
             */
            template<class T>
            struct toCpp {
                /**
                 * Convert the value
                 *
                 * @param val the value to convert
                 * @return the resulting value of type T
                 */
                static T convert(const Napi::Env &env, const Napi::Value &val) {
                    /*if (val.IsPromise()) {
                        std::cout << "value is promise" << std::endl;
                        return toCpp<std::future<T>>::convert(env, val).get();
                    }*/

                    if constexpr (classes::has_fromNapiValue<T, T(Napi::Env, Napi::Value)>::value) {
                        return T::fromNapiValue(env, val);
                    } else if constexpr (is_any_of<T, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>) {
                        if (!val.IsNumber()) throw std::runtime_error("The given type is not a number");
                        else return val.ToNumber();
                    } else if constexpr (is_any_of<T, std::string, const char *>) {
                        if (!val.IsString()) throw std::runtime_error("The given type is not a string");
                        else return val.ToString();
                    } else if constexpr (is_any_of<T, bool>) {
                        if (!val.IsBoolean()) throw std::runtime_error("The given type is not a boolean");
                        else return val.ToBoolean();
                    } else if constexpr (std::is_same_v<T, Napi::Value>) {
                        return val;
                    }
                }
            };

            /**
             * Convert a Napi::Array to a std::vector
             *
             * @tparam T the vector type
             */
            template<class T>
            struct toCpp<std::vector<T>> {
                /**
                 * Convert the value
                 *
                 * @param val the value to convert
                 * @return the resulting std::vector
                 */
                static std::vector<T> convert(const Napi::Env &, const Napi::Value &val) {
                    if (!val.IsArray()) throw std::runtime_error("The value supplied must be an array");

                    std::vector<T> vec;
                    auto arr = val.As<Napi::Array>();
                    for (int i = 0; i < arr.Length(); i++) {
                        vec.push_back(toCpp<T>::convert(arr.Get(i)));
                    }

                    return vec;
                }
            };

            /**
             * Convert a Napi::Object to a std::map
             *
             * @tparam T the map key type
             * @tparam U the map value type
             */
            template<class T, class U>
            struct toCpp<std::map<T, U>> {
                /**
                 * Convert the value
                 *
                 * @param val the value to convert
                 * @return the resulting std::map
                 */
                static std::map<T, U> convert(const Napi::Env &, const Napi::Value &val) {
                    if (!val.IsObject()) throw std::runtime_error("The value supplied must be an object");

                    std::map<T, U> map;
                    Napi::Object obj = val.ToObject();
                    std::vector<T> keys = toCpp<std::vector<T>>::convert(obj.GetPropertyNames());

                    for (const auto &key: keys) {
                        map.push(std::pair<T, U>(key, toCpp<U>::convert(obj.Get(key))));
                    }

                    return map;
                }
            };

            /**
             * Convert a Napi::Promise to a std::promise
             *
             * @tparam T the promise type
             * @see https://stackoverflow.com/a/70805475
             */
            template<class T>
            struct toCpp<std::shared_ptr<std::promise<T>>> {
                static std::shared_ptr<std::promise<T>> convert(const Napi::Env &env, const Napi::Value &val) {
                    if (!val.IsPromise()) throw std::runtime_error("The value supplied must be a promise");
                    auto promise = val.As<Napi::Promise>();

                    Napi::Value thenValue = promise.Get("then");
                    if (!thenValue.IsFunction())
                        throw std::runtime_error("Promise is not thenable");
                    auto then = thenValue.As<Napi::Function>();

                    std::shared_ptr<std::promise<T>> cppPromise = std::make_shared<std::promise<T>>();
                    Napi::Function callback = Napi::Function::New(env, [cppPromise](const Napi::CallbackInfo &info) {
                        std::cout << "Callback called" << std::endl;
                        if constexpr (std::is_same_v<T, void>) {
                            cppPromise->set_value();
                        } else {
                            cppPromise->set_value(toCpp<T>::convert(info.Env(), info[0]));
                        }
                    });

                    Napi::Function error = Napi::Function::New(env, [cppPromise](const Napi::CallbackInfo &info) {
                        if (info.Length() > 0) {
                            cppPromise->set_exception(std::make_exception_ptr(std::runtime_error(info[0].ToString())));
                        } else {
                            cppPromise->set_exception(std::make_exception_ptr(std::exception()));
                        }
                    });

                    std::cout << "Calling then" << std::endl;
                    then.Call(promise, {callback, error});
                    return cppPromise;
                }
            };

            /**
             * Convert a Napi::Value to any cpp type
             *
             * @tparam T the type to convert to
             * @param val the value to convert
             * @return the converted value
             */
            template<class T>
            static T convertToCpp(const Napi::Env &env, const Napi::Value &val) {
                return toCpp<T>::convert(env, val);
            }

            template<class T>
            using napi_can_convert = std::disjunction<
                    typename std::is_convertible<T, const char *>::type,
                    typename std::is_convertible<T, const char16_t *>::type,
                    typename std::is_convertible<T, std::string>::type,
                    typename std::is_convertible<T, std::u16string>::type,
                    typename std::is_integral<T>::type, typename std::is_floating_point<T>::type>;

            /**
             * Convert a c++ value to Napi::Value
             *
             * @tparam T the value type to convert
             * @param env the environment to run in
             * @param cppVal the c++ value to convert
             * @return the Napi::Value
             */
            template<class T>
            static Napi::Value cppValToValue(const Napi::Env &env, const T &cppVal) {
                if constexpr (classes::has_toNapiValue<T, Napi::Value(Napi::Env, T)>::value) {
                    return T::toNapiValue(env, cppVal);
                } else if constexpr(napi_can_convert<T>::value) {
                    return Napi::Value::From(env, cppVal);
                } else {
                    return env.Undefined();
                }
            }

            /**
             * Convert a std::vector to Napi::Value
             *
             * @tparam T the vector type
             * @param env the environment to run in
             * @param vec the vector to convert
             * @return the Napi::Value
             */
            template<class T>
            static Napi::Value cppValToValue(const Napi::Env &env, const std::vector<T> &vec) {
                auto v_s = (uint32_t) vec.size();
                Napi::Array arr = Napi::Array::New(env, v_s);
                for (uint32_t i = 0; i < v_s; i++) {
                    arr.Set(i, cppValToValue(env, vec[i]));
                }

                return arr;
            }

            /**
             * Convert a std::map to Napi::Value
             *
             * @tparam T the mep key type
             * @tparam U the map value type
             * @param env the environment to run in
             * @param map the map to convert
             * @return the Napi::Value
             */
            template<class T, class U>
            static Napi::Value cppValToValue(const Napi::Env &env, const std::map<T, U> &map) {
                Napi::Object obj = Napi::Object::New(env);
                for (const auto &p: map) {
                    obj.Set(cppValToValue(env, p.first), cppValToValue(env, p.second));
                }

                return obj;
            }
        } // namespace conversions
    } // namespace util

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#   define slash '\\'
#else
#   define slash '/'
#endif

    class exception : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;

        exception(const std::string &message, std::vector<std::string> stack) : std::runtime_error(message),
                                                                                _stack(std::move(stack)) {}

        void add_to_stack(const char *method, const std::string &file, int line) {
            _stack.insert(_stack.begin(),
                          std::string("\tat ") + method + " (" + file.substr(file.find_last_of(slash) + 1) + ':' +
                          std::to_string(line) + ')');
        }

        const std::vector<std::string> &stack() const {
            return _stack;
        }

        static exception from_napi_error(const Napi::Error &e) {
            if (e.Value().Has("stack") && e.Value().Get("stack").IsString()) {
                auto stack = util::split_string(e.Value().Get("stack").ToString().Utf8Value(), "\n");
                if (stack.size() > 1) {
                    // Remove the first element as it is 'Error: [TEXT]'
                    stack.erase(stack.begin());
                    exception ex(e.Message(), stack);
                    ex.add_to_stack(__FUNCTION__, __FILE__, __LINE__);
                    return ex;
                }
            }

            return exception{e.Message()};
        }

    private:
        std::vector<std::string> _stack;
    };

#undef slash

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
        public:
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
             * Get the promise
             *
             * @return a Napi::Promise
             */
            inline Napi::Promise GetPromise() const {
                return deferred.Promise();
            }

        protected:
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
#ifdef NAPI_TOOLS_ASYNC_WORKER_SLEEP
                    std::this_thread::sleep_for(std::chrono::milliseconds(NAPI_TOOLS_ASYNC_WORKER_SLEEP));
#endif //NAPI_TOOLS_ASYNC_WORKER_SLEEP
                } catch (std::exception &e) {
                    Napi::AsyncWorker::SetError(e.what());
                } catch (...) {
                    Napi::AsyncWorker::SetError("An unknown error occurred");
                }
            }

            /**
             * Default on ok
             */
            inline void OnOK() override {
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

            Napi::Promise::Deferred deferred;
        };

        /**
         * A class for creating Promises with return types
         *
         * @tparam T the return type of the operation
         */
        template<typename T>
        class promiseCreator : public AsyncWorker {
        public:
            /**
             * Construct a Promise
             *
             * @param env the environment to work in
             * @param _fn the function to call
             */
            inline promiseCreator(const Napi::Env &env, std::function<T()> _fn) : AsyncWorker(env),
                                                                                  fn(std::move(_fn)) {}

        protected:
            /**
             * A default destructor
             */
            inline ~promiseCreator() override = default;

            /**
             * The execution thread
             */
            inline void Run() override {
                val = fn();
            }

            /**
             * On ok
             */
            inline void OnOK() override {
                try {
                    deferred.Resolve(::napi_tools::util::conversions::cppValToValue(Env(), val));
                } catch (const std::exception &e) {
                    deferred.Reject(Napi::Error::New(Env(), e.what()).Value());
                } catch (...) {
                    deferred.Reject(Napi::Error::New(Env(), "An unknown error occurred").Value());
                }
            };

        private:
            std::function<T()> fn;
            T val;
        };

        /**
         * A class for creating Promises with no return type
         */
        template<>
        class promiseCreator<void> : public AsyncWorker {
        public:
            /**
             * Construct a Promise
             *
             * @param env the environment to work in
             * @param _fn the function to call
             */
            inline promiseCreator(const Napi::Env &env, std::function<void()> _fn) : AsyncWorker(env),
                                                                                     fn(std::move(_fn)) {}

        protected:
            /**
             * A default destructor
             */
            inline ~promiseCreator() override = default;

            /**
             * The run function
             */
            inline void Run() override {
                fn();
            }

        private:
            std::function<void()> fn;
        };

        /**
         * A class for creating promises
         *
         * @tparam T the return type of the promise
         */
        template<class T>
        class promise {
        public:
            /**
             * Create a promise
             *
             * @param env the environment to run in
             * @param fn the promise function to call
             */
            promise(const Napi::Env &env, const std::function<T()> &fn) {
                pr = new promiseCreator<T>(env, fn);
                pr->Queue();
            }

            /**
             * Get the Napi::Promise
             *
             * @return the Napi::Promise
             */
            [[nodiscard]] inline Napi::Promise getPromise() const {
                return pr->GetPromise();
            }

            /**
             * Get the Napi::Promise
             *
             * @return the Napi::Promise
             */
            [[nodiscard]] inline operator Napi::Promise() const {
                return this->getPromise();
            }

            /**
             * Get the Napi::Promise as a value
             *
             * @return the Napi::Value
             */
            [[nodiscard]] inline operator Napi::Value() const {
                return this->getPromise();
            }

            /**
             * A default destructor
             */
            inline ~promise() noexcept = default;

        private:
            promiseCreator<T> *pr;
        };

        /**
         * A class for creating void promises
         */
        template<>
        class promise<void> {
        public:
            /**
             * Create a promise
             *
             * @param env the environment to run in
             * @param fn the promise function to call
             */
            inline promise(const Napi::Env &env, const std::function<void()> &fn) {
                pr = new promiseCreator<void>(env, fn);
                pr->Queue();
            }

            /**
             * Get the Napi::Promise
             *
             * @return the Napi::Promise
             */
            [[nodiscard]] inline Napi::Promise getPromise() const {
                return pr->GetPromise();
            }

            /**
             * Get the Napi::Promise
             *
             * @return the Napi::Promise
             */
            [[nodiscard]] inline operator Napi::Promise() const {
                return this->getPromise();
            }

            /**
             * Get the Napi::Promise as a value
             *
             * @return the Napi::Value
             */
            [[nodiscard]] inline operator Napi::Value() const {
                return this->getPromise();
            }

            /**
             * A default destructor
             */
            inline ~promise() noexcept = default;

        private:
            promiseCreator<void> *pr;
        };
    } // namespace promises

    /**
     * A namespace for callbacks
     */
    namespace callbacks {
        using error_func = std::function<void(::napi_tools::exception)>;

        /**
         * Utility namespace
         */
        namespace util {
            template<class...Args>
            using converter_func = std::function<std::vector<napi_value>(const Napi::Env &, Args...)>;

            /**
             * The callback template
             *
             * @tparam T the javascriptCallback class type
             */
            template<class T, class...Args>
            class callback_template {
            public:
                /**
                 * Construct an empty callback function.
                 * Will throw an exception when trying to call.
                 */
                callback_template() noexcept: ptr(nullptr) {}

                /**
                 * Construct an empty callback function.
                 * Will throw an exception when trying to call.
                 */
                callback_template(std::nullptr_t) noexcept: ptr(nullptr) {}

                /**
                 * Construct a callback function
                 *
                 * @param info the CallbackInfo with typeof info[0] == 'function'
                 * @param converter an optional function to do the type conversions
                 */
                explicit callback_template(const Napi::CallbackInfo &info,
                                           const converter_func<Args...> &converter = nullptr)
                        : ptr(new wrapper(info, converter)), converter(converter) {}

                /**
                 * Construct a callback function
                 *
                 * @param env the environment to work in
                 * @param func the function to call
                 * @param converter an optional function to do the type conversions
                 */
                callback_template(const Napi::Env &env, const Napi::Function &func,
                                  const converter_func<Args...> &converter = nullptr)
                        : ptr(new wrapper(env, func, converter)), converter(converter) {}

                /**
                 * Get the underlying promise
                 *
                 * @return the promise
                 */
                [[nodiscard]] inline Napi::Promise getPromise() const {
                    if (ptr && !ptr->stopped) {
                        return ptr->fn->getPromise();
                    } else {
                        throw std::runtime_error("Callback was never initialized");
                    }
                }

                /**
                 * Get the underlying promise
                 *
                 * @return the promise
                 */
                [[nodiscard]] inline operator Napi::Promise() const {
                    return this->getPromise();
                }

                /**
                 * Get the underlying promise
                 *
                 * @return the promise
                 */
                [[nodiscard]] inline operator Napi::Value() const {
                    return this->operator Napi::Promise();
                }

                /**
                 * Get the setter function for this callback
                 *
                 * @param env the environment to run in
                 * @param setOnlyOnce whether to allow this callback to only get set once.
                 *                      Will throw an exception when tried to set a second time.
                 * @return the setter function
                 */
                inline Napi::Function getSetter(const Napi::Env &env, bool setOnlyOnce = false) {
                    return Napi::Function::New(env, [this, setOnlyOnce](const Napi::CallbackInfo &info) {
                        if (setOnlyOnce && this->operator bool()) {
                            throw Napi::Error::New(info.Env(),
                                                   "Tried to set a callback twice, which was not allowed to be set twice");
                        }
                        TRY
                            this->ptr.reset(new wrapper(info, converter));

                            return this->getPromise();
                        CATCH_EXCEPTIONS
                    });
                }

                /**
                 * Export the setter in the init function. Or wherever you like.
                 *
                 * @param env the environment to run in
                 * @param exports the exports object. Will set the setter function at index name.
                 * @param name the name of the setter function
                 * @param setOnlyOnce whether to allow this callback to only get set once.
                 *                      Will throw an exception when tried to set a second time.
                 */
                inline void exportSetter(const Napi::Env &env, Napi::Object &exports, const std::string &name,
                                         bool setOnlyOnce = false) {
                    exports.Set(name, this->getSetter(env, setOnlyOnce));
                }

                /**
                 * Check if the promise is initialized and not stopped
                 *
                 * @return true, if initialized and running
                 */
                [[nodiscard]] inline operator bool() const {
                    return ptr && !ptr->stopped;
                }

                /**
                 * Check if the promise is not initialized or stopped
                 *
                 * @return true, if not initialized or is stopped
                 */
                [[nodiscard]] inline bool stopped() const {
                    return !ptr || ptr->stopped;
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
                inline ~callback_template() = default;

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
                     * @param converter an optional function to do the type conversions
                     */
                    explicit wrapper(const Napi::CallbackInfo &info, const converter_func<Args...> &converter)
                            : fn(new T(info, converter)), stopped(false) {}

                    /**
                     * Create a wrapper instance
                     *
                     * @param env the environment to work in
                     * @param func the function to wrap the callback around
                     * @param converter an optional function to do the type conversions
                     */
                    wrapper(const Napi::Env &env, const Napi::Function &func, const converter_func<Args...> &converter)
                            : fn(new T(env, func, converter)), stopped(false) {}

                    /**
                     * Stop the callback
                     */
                    ~wrapper() {
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
                util::converter_func<Args...> converter;
            };
        } // namespace util

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
            /**
             * Create a javascript callback
             *
             * @param info the callback info
             * @param converter an optional function to do the type conversions
             */
            explicit inline javascriptCallback(const Napi::CallbackInfo &info,
                                               const util::converter_func<A...> &converter)
                    : deferred(Napi::Promise::Deferred::New(info.Env())), mtx(), converter(converter) {
                CHECK_ARGS(::napi_tools::napi_type::function);
                Napi::Env env = info.Env();

                run = true;

                // Create a new ThreadSafeFunction.
                this->ts_fn =
                        Napi::ThreadSafeFunction::New(env, info[0].As<Napi::Function>(), "javascriptCallback", 0, 1,
                                                      this, FinalizerCallback < R, A... >, (void *) nullptr);
                this->nativeThread = std::thread(threadEntry < R, A... >, this);
            }

            /**
             * Create a javascript callback
             *
             * @param env the environment to work in
             * @param func the function to wrap the callback around
             * @param converter an optional function to do the type conversions
             */
            javascriptCallback(const Napi::Env &env, const Napi::Function &func,
                               const util::converter_func<A...> &converter)
                    : deferred(Napi::Promise::Deferred::New(env)), mtx(), run(true), converter(converter) {
                // Create a new ThreadSafeFunction.
                this->ts_fn =
                        Napi::ThreadSafeFunction::New(env, func, "javascriptCallback", 0, 1,
                                                      this, FinalizerCallback < R, A... >, (void *) nullptr);
                this->nativeThread = std::thread(threadEntry < R, A... >, this);
            }

            /**
             * Async call the javascript function
             *
             * @param values the values to pass to the function
             * @param func the callback function
             */
            inline void asyncCall(A &&...values, const std::function<void(R)> &func, const error_func &on_error) {
                std::unique_lock<std::mutex> lock(mtx);
                queue.push_back(args(std::forward<A>(values)..., func, on_error, converter));
            }

            /**
             * Get the promise
             *
             * @return the Napi::Promise
             */
            [[nodiscard]] inline Napi::Promise getPromise() const {
                return deferred.Promise();
            }

            /**
             * Stop the function
             */
            inline void stop() {
                run = false;
                //mtx.unlock();
            }

        private:
            /**
             * A class for storing arguments
             */
            class args {
            public:
                /**
                 * Create a new args instance
                 *
                 * @param values the values to store
                 * @param func the callback function
                 * @param converter an optional function to do the type conversions
                 */
                inline explicit args(A &&...values, const std::function<void(R)> &func, error_func on_error,
                                     const util::converter_func<A...> &converter)
                        : args_t(std::forward<A>(values)...), fun(func), err(std::move(on_error)),
                          converter(converter) {}

                /**
                 * Convert the args to a napi_value vector.
                 * Source: https://stackoverflow.com/a/42495119
                 *
                 * @param env the environment to work in
                 * @return the value vector
                 */
                inline std::vector<napi_value> to_vector(const Napi::Env &env) {
                    if (converter) {
                        return std::apply([&env, this](auto &&... el) {
                            return converter(env, std::forward<decltype(el)>(el)...);
                        }, std::forward<std::tuple<A...>>(args_t));
                    } else {
                        return std::apply([&env](auto &&... el) {
                            return std::vector<napi_value>{
                                    ::napi_tools::util::conversions::cppValToValue(env,
                                                                                   std::forward<decltype(el)>(el))...};
                        }, std::forward<std::tuple<A...>>(args_t));
                    }
                }

                util::converter_func<A...> converter;
                std::function<void(R)> fun;
                error_func err;
            private:
                std::tuple<A...> args_t;
            };

            // The thread entry
            template<class U, class...Args>
            static void threadEntry(javascriptCallback<U(Args...)> *jsCallback) {
                // The callback function
                const auto callback = [](const Napi::Env &env, const Napi::Function &jsCallback, args *data) {
                    try {
                        Napi::Value val = jsCallback.Call(data->to_vector(env));
                        U ret = ::napi_tools::util::conversions::convertToCpp<U>(env, val);
                        data->fun(ret);
                    } catch (const Napi::Error &e) {
                        try {
                            auto ex = exception::from_napi_error(e);
                            ex.add_to_stack("napi_tools::callbacks::javascriptCallback::threadEntry::callback",
                                            __FILE__, __LINE__);
                            data->err(ex);
                        } catch (const std::exception &e) {
                            std::cerr << __FILE__ << ":" << __LINE__ << " Exception thrown: " << e.what() << std::endl;
                        } catch (...) {
                            std::cerr << __FILE__ << ":" << __LINE__ << " Unknown exception thrown" << std::endl;
                        }
                    } catch (const std::exception &e) {
                        try {
                            data->err(exception(e.what()));
                        } catch (const std::exception &e) {
                            std::cerr << __FILE__ << ":" << __LINE__ << " Exception thrown: " << e.what() << std::endl;
                        } catch (...) {
                            std::cerr << __FILE__ << ":" << __LINE__ << " Unknown exception thrown" << std::endl;
                        }
                    } catch (...) {
                        std::cerr << __FILE__ << ":" << __LINE__ << " Unknown exception thrown" << std::endl;
                    }
                    delete data;
                };

                while (jsCallback->run) {
                    // Lock the mutex
                    std::unique_lock<std::mutex> lock(jsCallback->mtx);
                    // Check if run is still true.
                    // Run may be false as the mutex is unlocked when stop() is called
                    if (jsCallback->run) {
                        for (const args &ar: jsCallback->queue) {
                            // Copy the arguments
                            auto *a = new args(ar);

                            // Call the callback
                            napi_status status = jsCallback->ts_fn.BlockingCall(a, callback);

                            if (status != napi_ok) {
                                Napi::Error::Fatal("ThreadEntry",
                                                   "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
                            }
                        }

                        // Clear the queue and unlock the mutex
                        jsCallback->queue.clear();
                        lock.unlock();

                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    } else {
                        lock.unlock();
                    }
                }

                jsCallback->ts_fn.Release();
            }

            // The finalizer callback
            template<class U, class...Args>
            static void FinalizerCallback(const Napi::Env &env, void *, javascriptCallback<U(Args...)> *jsCallback) {
                // Join the native thread and resolve the promise
                jsCallback->nativeThread.join();
                jsCallback->deferred.Resolve(env.Null());

                delete jsCallback;
            }

            /**
             * The destructor
             */
            ~javascriptCallback() noexcept = default;

            // Whether the callback thread should run
            bool run;
            std::mutex mtx;
            std::vector<args> queue;
            const Napi::Promise::Deferred deferred;
            std::thread nativeThread;
            Napi::ThreadSafeFunction ts_fn;
            util::converter_func<A...> converter;
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
             * @param converter an optional function to do the type conversions
             */
            explicit inline javascriptCallback(const Napi::CallbackInfo &info,
                                               const util::converter_func<A...> &converter)
                    : deferred(Napi::Promise::Deferred::New(info.Env())), queue(), mtx(), converter(converter) {
                CHECK_ARGS(::napi_tools::napi_type::function);
                Napi::Env env = info.Env();

                run = true;

                // Create a new ThreadSafeFunction.
                this->ts_fn = Napi::ThreadSafeFunction::New(env, info[0].As<Napi::Function>(), "javascriptCallback", 0,
                                                            1, this,
                                                            FinalizerCallback < A... >, (void *) nullptr);
                this->nativeThread = std::thread(threadEntry < A... >, this);
            }

            /**
             * Create a javascript callback
             *
             * @param env the environment to work in
             * @param func the function to wrap the callback around
             * @param converter an optional function to do the type conversions
             */
            javascriptCallback(const Napi::Env &env, const Napi::Function &func,
                               const util::converter_func<A...> &converter)
                    : deferred(Napi::Promise::Deferred::New(env)), queue(), mtx(), run(true), converter(converter) {
                // Create a new ThreadSafeFunction.
                this->ts_fn = Napi::ThreadSafeFunction::New(env, func, "javascriptCallback", 0,
                                                            1, this,
                                                            FinalizerCallback < A... >, (void *) nullptr);
                this->nativeThread = std::thread(threadEntry < A... >, this);
            }

            /**
             * Async call the function
             *
             * @param values the values to pass
             */
            inline void asyncCall(A &&...values, const std::function<void()> &callback, const error_func &on_error) {
                std::unique_lock<std::mutex> lock(mtx);
                queue.push_back(args(std::forward<A>(values)..., callback, on_error, converter));
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
             * Stop the function and deallocate all resources
             */
            inline void stop() {
                run = false;
                //mtx.unlock();
            }

        private:
            /**
             * A class for storing arguments
             */
            class args {
            public:
                /**
                 * Create the args class
                 *
                 * @param values the values to store
                 * @param converter an optional function to do the type conversions
                 */
                explicit args(A &&...values, std::function<void()> func, error_func on_error,
                              const util::converter_func<A...> &converter)
                        : args_t(std::forward<A>(values)...), fun(std::move(func)), err(std::move(on_error)),
                          converter(converter) {}

                /**
                 * Convert the args to a napi_value vector.
                 * Source: https://stackoverflow.com/a/42495119
                 *
                 * @param env the environment to work in
                 * @return the value vector
                 */
                inline std::vector<napi_value> to_vector(const Napi::Env &env) {
                    if (converter) {
                        return std::apply([&env, this](auto &&... el) {
                            return converter(env, std::forward<decltype(el)>(el)...);
                        }, std::forward<std::tuple<A...>>(args_t));
                    } else {
                        return std::apply([&env](auto &&... el) {
                            return std::vector<napi_value>{
                                    ::napi_tools::util::conversions::cppValToValue(env,
                                                                                   std::forward<decltype(el)>(el))...};
                        }, std::forward<std::tuple<A...>>(args_t));
                    }
                }

                std::function<void()> fun;
                error_func err;
            private:
                util::converter_func<A...> converter;
                std::tuple<A...> args_t;
            };

            // The thread entry
            template<class...Args>
            static void threadEntry(javascriptCallback<void(Args...)> *jsCallback) {
                // A callback function
                const auto callback = [](const Napi::Env &env, const Napi::Function &jsCallback, args *data) {
                    try {
                        jsCallback.Call(data->to_vector(env));
                        data->fun();
                    } catch (const Napi::Error &e) {
                        try {
                            auto ex = exception::from_napi_error(e);
                            ex.add_to_stack("napi_tools::callbacks::javascriptCallback::threadEntry::callback",
                                            __FILE__, __LINE__);
                            data->err(ex);
                        } catch (const std::exception &e) {
                            std::cerr << __FILE__ << ":" << __LINE__ << " Exception thrown: " << e.what() << std::endl;
                        } catch (...) {
                            std::cerr << __FILE__ << ":" << __LINE__ << " Unknown exception thrown" << std::endl;
                        }
                    } catch (const std::exception &e) {
                        try {
                            data->err(exception(e.what()));
                        } catch (const std::exception &e) {
                            std::cerr << __FILE__ << ":" << __LINE__ << " Exception thrown: " << e.what() << std::endl;
                        } catch (...) {
                            std::cerr << __FILE__ << ":" << __LINE__ << " Unknown exception thrown" << std::endl;
                        }
                    } catch (...) {
                        std::cerr << __FILE__ << ":" << __LINE__ << " Unknown exception thrown" << std::endl;
                    }
                    delete data;
                };

                while (jsCallback->run) {
                    std::unique_lock<std::mutex> lock(jsCallback->mtx);
                    // Check if run is still true,
                    // as the mutex is unlocked when stop() is called
                    if (jsCallback->run) {
                        // Go through all args in the queue
                        for (const args &val: jsCallback->queue) {
                            // Copy the args class
                            args *tmp = new args(val);

                            // Call the callback
                            napi_status status = jsCallback->ts_fn.BlockingCall(tmp, callback);

                            // Check the status
                            if (status != napi_ok) {
                                Napi::Error::Fatal("ThreadEntry",
                                                   "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
                            }
                        }

                        // Clear the queue
                        jsCallback->queue.clear();
                        lock.unlock();

                        // Sleep for some time
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    } else {
                        lock.unlock();
                    }
                }

                // Release the thread-safe function
                jsCallback->ts_fn.Release();
            }

            // The finalizer callback
            template<class...Args>
            static void FinalizerCallback(const Napi::Env &env, void *, javascriptCallback<void(Args...)> *jsCallback) {
                // Join the native thread and resolve the promise
                jsCallback->nativeThread.join();
                jsCallback->deferred.Resolve(env.Null());

                delete jsCallback;
            }

            // Default destructor
            ~javascriptCallback() noexcept = default;

            bool run;
            std::mutex mtx;
            std::vector<args> queue;
            const Napi::Promise::Deferred deferred;
            std::thread nativeThread;
            Napi::ThreadSafeFunction ts_fn;
            util::converter_func<A...> converter;
        };

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
        class callback<void(Args...)> : public util::callback_template<javascriptCallback<void(Args...)>, Args...> {
        public:
            using cb_template = util::callback_template<javascriptCallback<void(Args...)>, Args...>;
            using cb_template::cb_template;

            /**
             * Call the javascript function. Async call.
             *
             * @param args the function arguments
             */
            inline void call(Args...args, const std::function<void()> &callback,
                             const callbacks::error_func &on_error) {
                if (!callback || !on_error) {
                    throw std::runtime_error("The callback functions are not initialized");
                }

                if (this->ptr && !this->ptr->stopped) {
                    this->ptr->fn->asyncCall(std::forward<Args>(args)..., callback, on_error);
                } else {
                    throw std::runtime_error("Callback was never initialized");
                }
            }

            /**
             * Call the javascript function.
             *
             * @param args the function arguments
             * @return a promise to be resolved
             */
            std::future<void> call(Args...args) {
                std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();
                this->operator()(args..., [promise]() {
                    promise->set_value();
                }, [promise](const napi_tools::exception &e) {
                    promise->set_exception(std::make_exception_ptr(e));
                });

                return promise->get_future();
            }

            /**
             * Call the javascript function with a supplied promise.
             *
             * @param args the function arguments
             * @param promise the promise to be resolved
             */
            void call(Args...args, std::promise<void> &promise) {
                this->operator()(args..., [&promise]() {
                    promise.set_value();
                }, [&promise](const std::exception &e) {
                    promise.set_exception(std::make_exception_ptr(e));
                });
            }

            /**
             * Call the javascript function and wait for it to finish
             *
             * @param args the function arguments
             * @return the function return value
             */
            void callSync(Args...args) {
                std::promise<void> promise;
                std::future<void> future = promise.get_future();
                this->call(std::forward<Args>(args)..., promise);

                return future.get();
            }

            /**
             * Call the javascript function async.
             *
             * @param args the function arguments
             * @param callback the callback function to be called, as this is async
             */
            void operator()(Args...args, const std::function<void()> &callback,
                            const callbacks::error_func &on_error) {
                this->call(args..., callback, on_error);
            }

            /**
             * Call the javascript function.
             * Example usage:<br>
             *
             * <p><code>
             * std::promise&lt;int&gt; promise = callback();<br>
             * std::future&lt;int&gt; fut = promise.get_future();<br>
             * fut.wait();<br>
             * int res = fut.get();
             * </code></p>
             *
             * @param args the function arguments
             * @return a promise to be resolved
             */
            std::future<void> operator()(Args...args) {
                return this->call(args...);
            }

            /**
             * Call the javascript function with a supplied promise.
             * Example:<br>
             *
             * <p><code>
             * std::promise&lt;int&gt; promise;<br>
             * callback(promise);<br>
             * std::future&lt;int&gt; future = promise.get_future();<br>
             * int res = future.get();
             * </p></code>
             *
             * @param args the function arguments
             * @param promise the promise to be resolved
             */
            void operator()(Args...args, std::promise<void> &promise) {
                this->call(args..., promise);
            }
        };

        /**
         * A non-void javascript callback
         *
         * @tparam R the return type
         * @tparam Args the argument types
         */
        template<class R, class...Args>
        class callback<R(Args...)> : public util::callback_template<javascriptCallback<R(Args...)>, Args...> {
        public:
            using cb_template = util::callback_template<javascriptCallback<R(Args...)>, Args...>;
            using cb_template::cb_template;

            /**
             * Call the javascript function async.
             *
             * @param args the function arguments
             * @param callback the callback function to be called, as this is async
             */
            void call(Args...args, const std::function<void(R)> &callback, const callbacks::error_func &on_error) {
                if (!callback || !on_error) {
                    throw std::runtime_error("The callback functions are not initialized");
                }

                if (this->ptr && !this->ptr->stopped) {
                    this->ptr->fn->asyncCall(std::forward<Args>(args)..., callback, on_error);
                } else {
                    throw std::runtime_error("Callback was never initialized");
                }
            }

            /**
             * Call the javascript function.
             *
             * @param args the function arguments
             * @return a promise to be resolved
             */
            std::future<R> call(Args...args) {
                std::shared_ptr<std::promise<R>> promise = std::make_shared<std::promise<R>>();
                this->operator()(args..., [promise](const R &val) {
                    promise->set_value(val);
                }, [promise](const std::exception &e) {
                    promise->set_exception(std::make_exception_ptr(e));
                });

                return promise->get_future();
            }

            /**
             * Call the javascript function with a supplied promise.
             *
             * @param args the function arguments
             * @param promise the promise to be resolved
             */
            void call(Args...args, std::promise<R> &promise) {
                this->operator()(args..., [&promise](const R &val) {
                    promise.set_value(val);
                }, [&promise](const napi_tools::exception &e) {
                    promise.set_exception(std::make_exception_ptr(e));
                });
            }

            /**
             * Call the javascript function and wait for it to finish
             *
             * @param args the function arguments
             * @return the function return value
             */
            R callSync(Args...args) {
                std::promise<R> promise;
                std::future<R> future = promise.get_future();
                this->call(std::forward<Args>(args)..., promise);

                return future.get();
            }

            /**
             * Call the javascript function async.
             *
             * @param args the function arguments
             * @param callback the callback function to be called, as this is async
             */
            void operator()(Args...args, const std::function<void(R)> &callback,
                            const callbacks::error_func &on_error) {
                this->call(args..., callback, on_error);
            }

            /**
             * Call the javascript function.
             * Example usage:<br>
             *
             * <p><code>
             * std::promise&lt;int&gt; promise = callback();<br>
             * std::future&lt;int&gt; fut = promise.get_future();<br>
             * fut.wait();<br>
             * int res = fut.get();
             * </code></p>
             *
             * @param args the function arguments
             * @return a promise to be resolved
             */
            std::future<R> operator()(Args...args) {
                return this->call(args...);
            }

            /**
             * Call the javascript function with a supplied promise.
             * Example:<br>
             *
             * <p><code>
             * std::promise&lt;int&gt; promise;<br>
             * callback(promise);<br>
             * std::future&lt;int&gt; future = promise.get_future();<br>
             * int res = future.get();
             * </p></code>
             *
             * @param args the function arguments
             * @param promise the promise to be resolved
             */
            void operator()(Args...args, std::promise<R> &promise) {
                this->call(args..., promise);
            }
        };
    } // namespace callbacks
} // namespace napi_tools
#endif // NAPI_TOOLS_NAPI_TOOLS_HPP
