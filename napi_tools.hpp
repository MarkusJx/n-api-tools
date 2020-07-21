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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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

#include <regex>
#include <utility>
#include <map>
#include <type_traits>
#include <functional>
#include <thread>

#ifdef NAPI_VERSION
#   define call(name, object, ...) Get(name).As<Napi::Function>().Call(object, {__VA_ARGS__})
#   define create(...) As<Napi::Function>().New({__VA_ARGS__})

#   define TRY try {
#   define CATCH_EXCEPTIONS } catch (const std::exception &e) {throw Napi::Error::New(info.Env(), e.what());} catch (...) {throw Napi::Error::New(info.Env(), "An unknown error occurred");}

#   define CHECK_ARGS(...) ::util::checkArgs(info, ::util::removeNamespace(__FUNCTION__), {__VA_ARGS__})
#   define CHECK_LENGTH(len) if (info.Length() != len) throw Napi::TypeError::New(info.Env(), ::util::removeNamespace(__FUNCTION__) + " requires " + std::to_string(len) + " arguments")
#endif //NAPI_VERSION

/**
 * N-api tools namespace
 */
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
     * Exception override mainly for non-windows environments
     */
    class exception : public std::exception {
    public:
#if !(defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__))
        explicit exception(const char *msg) : std::exception(), msg(msg) {}

            [[nodiscard]] const char *what() const noexcept override {
                return msg;
            }

        private:
            const char *msg;
#else
        using std::exception::exception;
#endif
    };

    /**
     * An exception thrown if argument types do not match
     */
    class argumentMismatchException : public exception {
    public:
        /**
         * The same as std::exception, just repackaged and sold for a higher price
         */
        using exception::exception;
    };

    /**
     * A js object pointer
     *
     * @tparam T the object type
     */
    template<class T>
    class js_object_ptr;

    /**
     * Raw classes
     */
    namespace raw {
        /**
         * Javascript type enum
         */
        enum class js_type {
            string,
            number,
            boolean,
            function,
            array,
            object,
            null,
            undefined,
            none
        };

        /**
         * Convert a raw::js_type enum to string
         *
         * @param t the type
         * @return t as a string
         */
        inline std::string js_type_toString(js_type t) {
            switch (t) {
                case js_type::string:
                    return "string";
                case js_type::number:
                    return "number";
                case js_type::boolean:
                    return "boolean";
                case js_type::function:
                    return "function";
                case js_type::array:
                    return "array";
                case js_type::object:
                    return "object";
                case js_type::null:
                    return "null";
                case js_type::undefined:
                    return "undefined";
                default:
                    return "none";
            }
        }

        /*
         * The js object base class
         */
        class js_object {
        public:
#ifdef NAPI_VERSION

            /**
             * Get the value as a n-api value
             *
             * @param env the target environment
             * @return the n-api value
             */
            [[nodiscard]] virtual inline Napi::Value getValue(const Napi::Env &env) const {
                return env.Undefined();
            }

#endif //NAPI_VERSION

            /**
             * Check if two objects are equal
             *
             * @return true, if both objects are the same type and value
             */
            [[nodiscard]] virtual inline bool operator==(const js_object_ptr<js_object> &) const {
                return false;
            }

            /**
             * Get the type of this object
             *
             * @return the type of this object
             */
            [[nodiscard]] virtual inline js_type getType() const {
                return js_type::none;
            }

            /**
             * Convert this type to a string. Virtual. Different member functions will return different strings.
             * A string member function will return itself, a number member function will return its number as a string,
             * a boolean member function will return either 'true' or 'false' as literal string. All other members will
             * return its type as a string, e.g. object will return 'object'.
             *
             * @return this type or this type's value as a string
             */
            [[nodiscard]] virtual inline std::string toString() const {
                return "none";
            }

            /**
             * Check if this is a string
             *
             * @return true, if this is of type string
             */
            [[nodiscard]] inline bool isString() const {
                return getType() == js_type::string;
            }

            /**
             * Check if this is a number
             *
             * @return true, if this is of type number
             */
            [[nodiscard]] inline bool isNumber() const {
                return getType() == js_type::number;
            }

            /**
             * Check if this is a boolean
             *
             * @return true, if this is of type boolean
             */
            [[nodiscard]] inline bool isBoolean() const {
                return getType() == js_type::boolean;
            }

            /**
             * Check if this is a array
             *
             * @return true, if this is of type array
             */
            [[nodiscard]] inline bool isArray() const {
                return getType() == js_type::array;
            }

            /**
             * Check if this is a object
             *
             * @return true, if this is of type object
             */
            [[nodiscard]] inline bool isObject() const {
                return getType() == js_type::object;
            }

            /**
             * Check if this is a function
             *
             * @return true, if this is of type function
             */
            [[nodiscard]] inline bool isFunction() const {
                return getType() == js_type::function;
            }

            /**
             * Check if this is a undefined
             *
             * @return true, if this is of type undefined
             */
            [[nodiscard]] inline bool isUndefined() const {
                return getType() == js_type::undefined;
            }

            /**
             * Check if this is a null
             *
             * @return true, if this is of type null
             */
            [[nodiscard]] inline bool isNull() const {
                return getType() == js_type::null;
            }

            /**
             * The default js_object destructor
             */
            [[maybe_unused]] virtual inline ~js_object() = default;
        };

        /**
         * Convert varargs to vector with given type
         *
         * @tparam T the type of the resulting vector
         * @tparam Args the argument types. Should be the same as T
         * @param args the arguments
         * @return the resulting vector
         */
        template<class T, class ...Args>
        inline std::vector<T> convertArgsToVector(Args...args) {
            std::vector<T> argv;
            // Use volatile to disable optimization
            [[maybe_unused]] volatile auto x = {(argv.push_back(args), 0)...};

            return argv;
        }

        class undefined;

        class null;

        class number;

        class boolean;

        class string;

        class array;

        class object;

#ifdef NAPI_VERSION

        class function;

#endif //NAPI_VERSION
    }

    inline raw::js_object *getObjectFromExisting(const raw::js_object *current);

    /**
     * A pointer to a js object
     *
     * @tparam T the object type
     */
    template<class T>
    class js_object_ptr {
    public:
        // Throw a compile-time error if T is not derived from raw::js_object or if T is not equal to raw::js_object
        static_assert(std::is_base_of_v<raw::js_object, T> || std::is_same_v<raw::js_object, T>,
                      "js_object_ptr can only store pointers derived from raw::js_object or pointers that are the same as raw::js_object");

        /**
         * The default constructor
         */
        inline js_object_ptr() {
            // Create new instance of empty T
            ptr = new T();
        }

        /**
         * Create a js_object_ptr from an existing poitner
         *
         * @param obj_ptr the already existing pointer
         */
        explicit inline js_object_ptr(T *obj_ptr) {
            ptr = obj_ptr;
        }

        /**
         * Create an object from a value.
         * Will be disabled if T = raw::js_object. For this case, specialized constructors are available.
         *
         * @tparam U the value type
         * @param obj the object
         */
        template<class U>
        inline js_object_ptr(U obj) {
            static_assert(!std::is_same_v<raw::js_object, T>,
                          "raw::js_object cannot be constructed using given type U.");
            ptr = new T(obj);
        }

#ifdef NAPI_VERSION

        /**
         * Construct from a n-api value
         *
         * @param value the value to construct from
         */
        inline js_object_ptr(const Napi::Value &value) {
            ptr = new T(value);
        }

#endif //NAPI_VERSION

        // Specialized constructors. Only available when T = raw::js_object

        /**
         * Create a js_object_ptr<raw::js_object> from a char array.
         * Will create a js_object_ptr<raw::string> and cast it to raw::js_object
         *
         * @param c the char array containing data
         */
        inline js_object_ptr(const char *c) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::string, T>,
                          "Specialized constructors are not allowed when the type is not raw::js_object");
            ptr = (T *) new raw::string(c);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a string.
         * Will create a js_object_ptr<raw::string> and cast it to raw::js_object
         *
         * @param s the string containing the data
         */
        inline js_object_ptr(const std::string &s) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::string, T>,
                          "Specialized constructors are not allowed when the type is not raw::js_object");
            ptr = (T *) new raw::string(s);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a bool value.
         * Will create a js_object_ptr<raw::boolean> and cast it to raw::js_object
         *
         * @param b the bool value
         */
        inline js_object_ptr(bool b) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::boolean, T>,
                          "Specialized constructors are not allowed when the type is not raw::js_object");
            ptr = (T *) new raw::boolean(b);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a integer value.
         * Will create a js_object_ptr<raw::number> and cast it to raw::js_object
         *
         * @param i the integer value
         */
        inline js_object_ptr(int i) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                          "Specialized constructors are not allowed when the type is not raw::js_object");
            ptr = (T *) new raw::number(i);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a double value.
         * Will create a js_object_ptr<raw::number> and cast it to raw::js_object
         *
         * @param d the double value
         */
        inline js_object_ptr(double d) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                          "Specialized constructors are not allowed when the type is not raw::js_object");
            ptr = (T *) new raw::number(d);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a vector<js_object_ptr<raw::js_object>> value.
         * Will create a js_object_ptr<raw::array> and cast it to raw::js_object
         *
         * @param data the data array
         */
        inline js_object_ptr(const std::vector<js_object_ptr<raw::js_object>> data) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::array, T>,
                          "Specialized constructors are not allowed when the type is not raw::js_object");
            ptr = (T *) new raw::array(data);
        }

        /**
         * Create an object from a js_object. Copy constructor.
         * The pointer will be converted first to prevent data loss
         *
         * @param obj the pointer to create the object from
         */
        inline js_object_ptr(const js_object_ptr<raw::js_object> &obj) {
            ptr = new T(*obj.as<T>());
        }

        /**
         * Copy constructor
         *
         * @tparam U the object type to copy from
         * @param obj the object to copy from
         */
        template<class U>
        inline js_object_ptr(const js_object_ptr<U> &obj) {
            ptr = (T *) new U(*obj.get());
        }

        /**
         * Copy constructor
         *
         * @param obj the object to copy from
         */
        inline js_object_ptr(const T &obj) {
            ptr = new T(obj);
        }

        /**
         * Operator =
         *
         * @param other the pointer to copy from
         * @return this object
         */
        inline js_object_ptr<T> &operator=(const js_object_ptr<T> &other) {
            if (this != &other) {
                delete ptr;
                if (other.get() == nullptr) {
                    ptr = nullptr;
                } else {
                    if constexpr (std::is_same_v<raw::js_object, T>) {
                        ptr = getObjectFromExisting(other.get());
                    } else {
                        ptr = new T(*other.as<T>());
                    }
                }
            }

            return *this;
        }

        /**
         * operator =
         *
         * @tparam U the type of the other pointer
         * @param other the other pointer
         * @return this
         */
        template<class U>
        inline js_object_ptr<T> &operator=(const js_object_ptr<U> &other) {
            static_assert(std::is_same_v<raw::js_object, T>,
                          "Operator= with type = U is only allowed when T = raw::js_object");
            delete ptr;
            if (other.get() == nullptr) {
                ptr = nullptr;
            } else {
                ptr = (T *) new U(*other.get());
            }

            return *this;
        }

        /**
         * Set a value
         *
         * @tparam U the type of the value
         * @param value the value
         * @return this
         */
        template<class U>
        inline js_object_ptr<T> &operator=(U value) {
            ptr->operator=(value);

            return *this;
        }

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::string
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(const char *value) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::string, T>,
                          "this operator overload is only available when T = raw::js_object or T = raw::string");
            ptr->operator=(value);

            return *this;
        }

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::string
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(std::string value) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::string, T>,
                          "this operator overload is only available when T = raw::js_object or T = raw::string");
            ptr->operator=(value);

            return *this;
        }

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::number
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(int value) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                          "this operator overload is only available when T = raw::js_object or T = raw::number");
            ptr->operator=(value);

            return *this;
        }

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::number
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(double value) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                          "this operator overload is only available when T = raw::js_object or T = raw::number");
            ptr->operator=(value);

            return *this;
        }

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::boolean
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(bool value) {
            static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::boolean, T>,
                          "this operator overload is only available when T = raw::js_object or T = raw::boolean");
            ptr->operator=(value);

            return *this;
        }

#ifdef NAPI_VERSION

        /**
         * Set a n-api value
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(const Napi::Value &value) {
            delete ptr;
            ptr = new T(value);
        }

#endif //NAPI_VERSION

        /**
         * operator []
         *
         * @tparam U the type of the search value
         * @param searchValue the search value
         * @return the result of the operation
         */
        template<class U>
        inline auto operator[](U searchValue) {
            return ptr->operator[](searchValue);
        }

        /**
         * operator +
         *
         * @param other the object to add
         * @return the result of the operation
         */
        template<class U>
        inline js_object_ptr<T> operator+(const js_object_ptr<U> &other) {
            if constexpr (std::is_same_v<raw::string, U> || std::is_same_v<raw::string, T>) {
                std::string s(ptr->toString());
                s.append(other->toString());

                return s;
            } else {
                return *ptr + *other.ptr;
            }
        }

        /**
         * operator +
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return the result of the operation
         */
        template<class U>
        inline js_object_ptr<T> operator+(U val) {
            if (ptr->getType() == raw::js_type::number) {
                // If U = std::string or U = const char *, create a string
                if constexpr (std::is_same_v<std::string, U> || std::is_same_v<const char *, U>) {
                    std::string s(ptr->toString());
                    s.append(val);

                    return s;
                } else {
                    return ((raw::number *) ptr)->operator+(val);
                }
            } else if (ptr->getType() == raw::js_type::string) { // If T is a string, add to the string
                std::string s(ptr->toString());
                if constexpr (std::is_same_v<std::string, U> || std::is_same_v<const char *, U>) {
                    // U = std::string or U = const char* can append
                    s.append(val);
                } else {
                    // U != some sort of string, must convert it
                    if constexpr (std::is_same_v<bool, U>) {
                        // U = bool, make it more human-readable
                        if (val) {
                            s.append("true");
                        } else {
                            s.append("false");
                        }
                    } else {
                        // Append val as std::string
                        s.append(std::to_string(val));
                    }
                }

                return js_object_ptr<T>(s);
            } else {
                // If U = std::string or U = const char *, create string
                if constexpr (std::is_same_v<std::string, U> || std::is_same_v<const char *, U>) {
                    std::string s(ptr->toString());
                    s.append(val);

                    return s;
                } else {
                    // If not, crash violently
                    throw argumentMismatchException("Can only concatenate strings or add numbers");
                }
            }
        }

        /**
         * operator +
         *
         * @param other the object to subtract
         * @return the result of the operation
         */
        inline auto operator-(const js_object_ptr<T> &other) {
            return *ptr - *other.ptr;
        }

        /**
         * operator -
         *
         * @tparam U the type of the value to subtract
         * @param val the value to subtract
         * @return the result of the operation
         */
        template<class U>
        inline auto operator-(U val) {
            return *ptr - val;
        }

        /**
         * operator *
         *
         * @param other the object to multiply with
         * @return the result of the operation
         */
        inline auto operator*(const js_object_ptr<T> &other) {
            return (*ptr) * (*other.ptr);
        }

        /**
         * operator *
         *
         * @tparam U the type of the value to multiply with
         * @param val the value to multiply with
         * @return the result of the operation
         */
        template<class U>
        inline auto operator*(U val) {
            return (*ptr) * val;
        }

        /**
         * operator /
         *
         * @param other the object to add
         * @return the result of the operation
         */
        inline auto operator/(const js_object_ptr<T> &other) {
            return (*ptr) / (*other.ptr);
        }

        /**
         * operator /
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return the result of the operation
         */
        template<class U>
        inline auto operator/(U val) {
            return (*ptr) / val;
        }

        /**
         * Operator +=
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return this
         */
        template<class U>
        inline js_object_ptr<T> &operator+=(U val) {
            return this->operator=(this->operator+(val));
        }

        /**
         * Get this pointer as another object
         *
         * @tparam U the type to cast to
         * @return the resulting pointer
         */
        template<class U>
        [[nodiscard]] inline U *as() const {
            return (U *) ptr;
        }

        /**
         * Convert to another js_object_ptr
         *
         * @tparam U the target type
         * @return the resulting pointer
         */
        template<class U>
        [[nodiscard]] inline js_object_ptr<U> to() const {
            return js_object_ptr<U>(new U(*((U *) ptr)));
        }

        /**
         * Convert this to js_object_ptr<raw::js_object>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::js_object> asRawObject() const {
            return to<raw::js_object>();
        }

        // Convert to various types. Only available when T = raw::js_object

        /**
         * Convert this to js_object_ptr<raw::string>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::string> asString() const {
            // Throw compile-time error if asString is not called on a raw::js_object
            static_assert(std::is_same_v<raw::js_object, T>, "asString can only be called on a raw object");
            if (ptr->getType() != raw::js_type::string)
                throw argumentMismatchException("asString can only be called on a raw object of type string");
            return to<raw::string>();
        }

        /**
         * Convert this to js_object_ptr<raw::number>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::number> asNumber() const {
            static_assert(std::is_same_v<raw::js_object, T>, "asNumber can only be called on a raw object");
            if (ptr->getType() != raw::js_type::number)
                throw argumentMismatchException("asNumber can only be called on a raw object of type number");
            return to<raw::number>();
        }

        /**
         * Convert this to js_object_ptr<raw::boolean>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::boolean> asBoolean() const {
            static_assert(std::is_same_v<raw::js_object, T>, "asBoolean can only be called on a raw object");
            if (ptr->getType() != raw::js_type::boolean)
                throw argumentMismatchException("asBoolean can only be called on a raw object of type boolean");
            return to<raw::boolean>();
        }

        /**
         * Convert this to js_object_ptr<raw::array>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::array> asArray() const {
            static_assert(std::is_same_v<raw::js_object, T>, "asArray can only be called on a raw object");
            if (ptr->getType() != raw::js_type::string)
                throw argumentMismatchException("asArray can only be called on a raw object of type array");
            return to<raw::array>();
        }

        /**
         * Convert this to js_object_ptr<raw::object>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::object> asObject() const {
            static_assert(std::is_same_v<raw::js_object, T>, "asObject can only be called on a raw object");
            if (ptr->getType() != raw::js_type::object)
                throw argumentMismatchException("asObject can only be called on a raw object of type object");
            return to<raw::object>();
        }

        /**
         * Convert this to js_object_ptr<raw::function>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::function> asFunction() const {
            static_assert(std::is_same_v<raw::js_object, T>, "asFunction can only be called on a raw object");
            if (ptr->getType() != raw::js_type::string)
                throw argumentMismatchException("asFunction can only be called on a raw object of type function");
            return to<raw::function>();
        }

        // Overloaded operators for std::string, bool and double
        // Only available for the specific classes storing these

        /**
         * operator std::string
         *
         * @return the string value
         */
        [[nodiscard]] inline operator std::string() const {
            static_assert(std::is_same_v<raw::string, T> || std::is_same_v<raw::object, T>,
                          "operator std::string can only be used with a string type");
            if constexpr (std::is_same_v<raw::string, T>) {
                return ptr->operator std::string();
            } else { // T = raw::object
                if (ptr->isString()) {
                    return ((raw::string *) ptr)->operator std::string();
                } else {
                    throw argumentMismatchException("Can not use operator std::string on non-string type");
                }
            }
        }

        /**
         * operator bool
         *
         * @return the bool value
         */
        [[nodiscard]] inline operator bool() const {
            static_assert(std::is_same_v<raw::boolean, T> || std::is_same_v<raw::object, T>,
                          "operator bool can only be used with a boolean type");
            if constexpr (std::is_same_v<raw::boolean, T>) {
                return ptr->operator bool();
            } else {
                if (ptr->isBoolean()) {
                    return ((raw::boolean *) ptr)->operator bool();
                } else {
                    throw argumentMismatchException("Can not use operator bool on non-bool type");
                }
            }
        }

        /**
         * operator double
         *
         * @return the double value
         */
        [[nodiscard]] inline operator double() const {
            static_assert(std::is_same_v<raw::number, T> || std::is_same_v<raw::object, T>,
                          "operator double can only be used with a number type");
            if constexpr (std::is_same_v<raw::number, T>) {
                return ptr->operator double();
            } else {
                if (ptr->isNumber()) {
                    return ((raw::number *) ptr)->operator double();
                } else {
                    throw argumentMismatchException("Can not use operator double on non-number type");
                }
            }
        }

        /**
         * Operator ->
         *
         * @return the pointer
         */
        [[nodiscard]] inline T *operator->() const {
            return ptr;
        }

        /**
         * The operator equals
         *
         * @tparam U the type to compare with
         * @param other the object to compare with
         * @return true, if both objects are equal
         */
        template<class U>
        [[nodiscard]] inline bool operator==(const js_object_ptr<U> &other) const {
            return ptr->operator==(*other.ptr);
        }

        /**
         * The operator not equals
         *
         * @tparam U the type to compare with
         * @param other the object to compare with
         * @return true, if both objects are not equal
         */
        template<class U>
        [[nodiscard]] inline bool operator!=(const js_object_ptr<U> &other) const {
            return !this->operator==(other);
        }

        /**
         * Get the underlying pointer
         *
         * @return the underlying pointer
         */
        [[nodiscard]] inline T *get() const {
            return ptr;
        }

        /**
         * Reset the pointer
         */
        inline void reset() {
            delete ptr;
            ptr = nullptr;
        }

        /**
         * The destructor
         */
        inline ~js_object_ptr() {
            delete ptr;
        }

        /**
         * Create a new instance of this class with a given type
         *
         * @tparam Types the argument types
         * @param args the arguments
         * @return the resulting pointer
         */
        template<class...Types>
        inline static js_object_ptr<T> make(Types &&...args) {
            return js_object_ptr<T>(new T(std::forward<Types>(args)...));
        }

    private:
        T *ptr;
    };

    using js_object = js_object_ptr<raw::js_object>;
    using string = js_object_ptr<raw::string>;
    using number = js_object_ptr<raw::number>;
    using boolean = js_object_ptr<raw::boolean>;
    using object = js_object_ptr<raw::object>;
    using array = js_object_ptr<raw::array>;
    using null = js_object_ptr<raw::null>;
    using undefined = js_object_ptr<raw::undefined>;

    /**
     * A variable data type
     */
    using var = js_object;

#ifdef NAPI_VERSION

    inline js_object getObject(const Napi::Value &val);

#endif //NAPI_VERSION

    namespace raw {
        /**
         * Undefined property
         */
        class undefined : public js_object {
        public:
            /**
             * Undefined constructor
             */
            inline undefined() = default;

#ifdef NAPI_VERSION

            /**
             * Get the undefined value as javascript value
             *
             * @param env the environment to work in
             * @return the value
             */
            [[maybe_unused]] [[nodiscard]] inline Napi::Value getValue(const Napi::Env &env) const override {
                return env.Undefined();
            }

#endif //NAPI_VERSION

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if both are equal
             */
            [[nodiscard]] inline bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType();
            }

            /**
             * Get the type of this object
             *
             * @return the type of this object
             */
            [[nodiscard]] inline js_type getType() const override {
                return js_type::undefined;
            }

            /**
             * Convert to string
             *
             * @return this value as a string
             */
            [[nodiscard]] inline std::string toString() const override {
                return "undefined";
            }

            /**
             * The destructor
             */
            inline ~undefined() override = default;
        };

        /**
         * null property
         */
        class null : public js_object {
        public:
            /**
             * The constructor
             */
            inline null() = default;

#ifdef NAPI_VERSION

            /**
             * Get the null value as javascript value
             *
             * @param env the environment to work in
             * @return the value
             */
            [[maybe_unused]] [[nodiscard]] inline Napi::Value getValue(const Napi::Env &env) const override {
                return env.Null();
            }

#endif //NAPI_VERSION

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the other object is also null
             */
            [[nodiscard]] inline bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType();
            }

            /**
             * Get the type of this object
             *
             * @return the type
             */
            [[nodiscard]] inline js_type getType() const override {
                return js_type::null;
            }

            /**
             * Convert to string
             *
             * @return this value as a string
             */
            [[nodiscard]] inline std::string toString() const override {
                return "null";
            }

            /**
             * The destructor
             */
            inline ~null() override = default;
        };

        /**
         * boolean type
         */
        class boolean : public js_object {
        public:
#ifdef NAPI_VERSION

            /**
             * Construct boolean from a n-api value
             *
             * @param v the value to get the boolean from
             */
            [[maybe_unused]] inline boolean(const Napi::Value &v) {
                if (!v.IsBoolean())
                    throw argumentMismatchException("class boolean requires a n-api value of type boolean");
                value = v.ToBoolean().Value();
            }

            /**
             * Construct a boolean from a n-api boolean
             *
             * @param b the boolean value to construct from
             */
            [[maybe_unused]] inline boolean(const Napi::Boolean &b) : value(b.Value()) {}

#endif //NAPI_VERSION

            /**
             * Construct a boolean from a bool value
             *
             * @param val the bool value
             */
            [[maybe_unused]] inline boolean(bool val) : value(val) {}

#ifdef NAPI_VERSION

            /**
             * Get the n-api boolean value
             *
             * @param env the environment to worn in
             * @return the n-api boolean value
             */
            [[maybe_unused]] [[nodiscard]] inline Napi::Value getValue(const Napi::Env &env) const override {
                return Napi::Boolean::New(env, value);
            }

#endif //NAPI_VERSION

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the objects are equal
             */
            [[nodiscard]] inline bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType() && value == obj.as<boolean>()->operator bool();
            }

            /**
             * Get the type of the object
             *
             * @return the type
             */
            [[nodiscard]] inline js_type getType() const override {
                return js_type::boolean;
            }

            /**
             * Convert to string
             *
             * @return this value as a string
             */
            [[nodiscard]] inline std::string toString() const override {
                if (value) {
                    return "true";
                } else {
                    return "false";
                }
            }

            /**
             * Set the value
             *
             * @param val the new value
             * @return this
             */
            inline boolean &operator=(bool val) {
                value = val;
            }

            /**
             * Operator bool
             *
             * @return the bool value
             */
            inline operator bool() const {
                return value;
            }

            /**
             * Get the value
             *
             * @return the value
             */
            [[nodiscard]] inline bool getValue() const {
                return value;
            }

        private:
            bool value;
        };

        /**
         * number type
         */
        class number : public js_object {
        public:
#ifdef NAPI_VERSION

            /**
             * Create a number from a n-api number
             *
             * @param number the number
             */
            [[maybe_unused]] inline number(const Napi::Number &number) {
                value = number.DoubleValue();
            }

            /**
             * Create a number from a n-api value
             *
             * @param val the value
             */
            [[maybe_unused]] inline number(const Napi::Value &val) {
                if (!val.IsNumber())
                    throw argumentMismatchException("class number requires a n-api value of type number");
                value = val.ToNumber().DoubleValue();
            }

#endif //NAPI_VERSION

            /**
             * Create a number from a double value
             *
             * @param val the value
             */
            [[maybe_unused]] inline number(double val) {
                value = val;
            }

            /**
             * Create a number from a integer value
             *
             * @param val the value
             */
            [[maybe_unused]] inline number(int val) {
                value = val;
            }

#ifdef NAPI_VERSION

            /**
             * Get the n-api value of this number
             *
             * @param env the environment to work in
             * @return the n-api value
             */
            [[maybe_unused]] [[nodiscard]] inline Napi::Value getValue(const Napi::Env &env) const override {
                return Napi::Number::New(env, value);
            }

#endif //NAPI_VERSION

            /**
             * Operator equal
             *
             * @param obj the object to compare with
             * @return true if the object is equal
             */
            [[nodiscard]] inline bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType() && value == obj.as<number>()->operator double();
            }

            /**
             * Get the type of this object
             *
             * @return the type
             */
            [[nodiscard]] inline js_type getType() const override {
                return js_type::number;
            }

            /**
             * Convert to string
             *
             * @return this value as a string
             */
            [[nodiscard]] inline std::string toString() const override {
                std::string str = std::to_string(value);
                str.erase(str.find_last_not_of('0') + 1, std::string::npos);
                if (str.ends_with('.')) str.pop_back();
                return str;
            }

            /**
             * Double operator
             *
             * @return the double value
             */
            inline operator double() const {
                return value;
            }

            /**
             * Get the double value
             *
             * @return the value
             */
            [[nodiscard]] [[maybe_unused]] inline double getValue() const {
                return value;
            }

            /**
             * Get the integer value
             *
             * @return the value
             */
            [[nodiscard]] [[maybe_unused]] inline int intValue() const {
                return (int) value;
            }

            /**
             * Set the value
             *
             * @param val the new value
             * @return this
             */
            inline number &operator=(double val) {
                value = val;

                return *this;
            }

            /**
             * Set the value
             *
             * @param val the new value
             * @return this
             */
            inline number &operator=(int val) {
                value = val;

                return *this;
            }

            template<class T>
            inline auto operator+(T val) {
                return value + val;
            }

        private:
            double value;
        };

        /**
         * string type
         */
        class string : public std::string, public js_object {
        public:
            // Use string constructors
            using std::string::string;

            /**
             * The default constructor
             */
            inline string() = default;

#ifdef NAPI_VERSION

            /**
             * Create a string from a n-api string
             *
             * @param str the n-api string value
             */
            [[maybe_unused]] inline string(const Napi::String &str) : std::string(str.Utf8Value()) {}

            /**
             * Create a string from a n-api value
             *
             * @param value the value
             */
            [[maybe_unused]] inline string(const Napi::Value &value) : std::string() {
                if (!value.IsString())
                    throw argumentMismatchException("class string requires a n-api value of type string");
                this->assign(value.As<Napi::String>().Utf8Value());
            }

#endif //NAPI_VERSION

            /**
             * Replace a value with another value
             *
             * @param toReplace the value to replace
             * @param replacement the replacement value
             * @return this
             */
            [[maybe_unused]] inline string replace(const string &toReplace, const string &replacement) {
                const std::regex re(toReplace);
                this->assign(std::regex_replace(*this, re, replacement));
                return *this;
            }

            /**
             * Concatenate strings
             *
             * @tparam Args the argument types
             * @param args the strings to concatenate
             * @return this
             */
            template<class...Args>
            [[maybe_unused]] inline string concat(Args...args) {
                std::vector<std::string> strings = convertArgsToVector<std::string>(args...);
                for (const std::string &s : strings) {
                    this->append(s);
                }

                return *this;
            }

            /**
             * Check if this ends with a value
             *
             * @param s the value to check
             * @return true if this ends with the given value
             */
            [[maybe_unused]] [[nodiscard]] inline bool endsWith(const string &s) const {
                return this->ends_with(s);
            }

            /**
             * Check if this includes a substring
             *
             * @param s the substring to check for
             * @return true if the substring exists
             */
            [[maybe_unused]] [[nodiscard]] inline bool includes(const string &s) const {
                return this->find(s) != std::string::npos;
            }

            /**
             * Get the index of a substring
             *
             * @param s the string to search for
             * @return the index of the substring or -1 if not found
             */
            [[maybe_unused]] [[nodiscard]] inline int indexOf(const string &s) const {
                return (int) this->find_first_of(s);
            }

            /**
             * Get the index of a substring
             *
             * @param s the string to search for
             * @param fromIndex the starting index
             * @return the index of the substring or -1 if not found
             */
            [[maybe_unused]] [[nodiscard]] inline int indexOf(const string &s, int fromIndex) const {
                return (int) this->find_first_of(s, fromIndex + s.length());
            }

            /**
             * Get the last index of a substring
             *
             * @param s the string to search for
             * @param fromIndex the last index to search for
             * @return the index of the substring or -1 if not found
             */
            [[maybe_unused]] [[nodiscard]] inline int
            lastIndexOf(const string &s, size_t fromIndex = std::string::npos) const {
                return (int) this->find_last_of(s, fromIndex);
            }

            /**
             * Get the char at a position
             *
             * @param pos the position of the char to get
             * @return the char as a string
             */
            [[nodiscard]] inline string charAt(int pos) const {
                string str;
                str += this->at(pos);
                return str;
            }

            /**
             * Get the char at a position
             *
             * @param pos the position of the char to get
             * @return the char as a string
             */
            [[nodiscard]] inline string operator[](int pos) const {
                string str;
                str += this->at(pos);
                return str;
            }

            /**
             * Check if a regex matches
             *
             * @param regex the regex to check
             * @return all matches in this string
             */
            [[maybe_unused]] inline std::vector<string> match(const string &regex) {
                const std::regex re(regex);
                std::smatch match;

                std::vector<string> result;

                if (regex_search(*this, match, re)) {
                    for (int i = 1; i < match.size(); i++) {
                        auto s = match[i].str();
                        result.emplace_back(s.begin(), s.end());
                    }
                }

                return result;
            }

#ifdef NAPI_VERSION

            /**
             * Get this as a n-api string
             *
             * @param env the environment to work in
             * @return the n-api string
             */
            [[maybe_unused]] [[nodiscard]] inline Napi::String toNapiString(const Napi::Env &env) const {
                return Napi::String::New(env, *this);
            }

            /**
             * Get this as a n-api value
             *
             * @param env the environment to work in
             * @return the n-api value
             */
            [[maybe_unused]] [[nodiscard]] inline Napi::Value getValue(const Napi::Env &env) const override {
                return Napi::String::New(env, *this);
            }

#endif //NAPI_VERSION

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the two objects are equal
             */
            [[nodiscard]] inline bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType() && strcmp(this->c_str(), obj.as<string>()->c_str()) == 0;
            }

            /**
             * Get the type of this object
             *
             * @return the type
             */
            [[nodiscard]] inline js_type getType() const override {
                return js_type::string;
            }

            /**
             * Convert to string
             *
             * @return this value as a string
             */
            [[nodiscard]] inline std::string toString() const override {
                return std::string(this->begin(), this->end());
            }

            /**
             * Create a string from characters
             *
             * @tparam Args the argument types
             * @param args the characters
             * @return the string
             */
            template<class...Args>
            inline static string fromCharCode(Args...args) {
                std::vector<char> data = convertArgsToVector<char>();
                return string(data.begin(), data.end());
            }
        };

        /**
         * object l-value
         */
        class objLValue {
        public:
            /**
             * The constructor
             *
             * @param setter the setter function
             * @param ptr the pointer
             */
            explicit inline objLValue(std::function<void(::napi_tools::js_object)> setter,
                                      const ::napi_tools::js_object &ptr)
                    : setter(std::move(setter)), ptr(ptr) {}

            /**
             * The object setter
             *
             * @param val the value
             * @return this
             */
            inline objLValue &operator=(const ::napi_tools::js_object &val) {
                setter(val);
                ptr = val;
            }

            /**
             * operator ->
             *
             * @return the object
             */
            [[nodiscard]] inline ::napi_tools::js_object operator->() const {
                return ptr;
            }

            /**
             * Get the pointer
             *
             * @return the pointer
             */
            [[nodiscard]] inline ::napi_tools::js_object getValue() const {
                return ptr;
            }

            /**
             * Get the pointer
             *
             * @return the pointer
             */
            [[nodiscard]] inline operator ::napi_tools::js_object() const {
                return ptr;
            }

        private:
            std::function<void(::napi_tools::js_object)> setter;
            ::napi_tools::js_object ptr;
        };

        /**
         * array type
         */
        class array : public js_object {
        public:
#ifdef NAPI_VERSION

            /**
             * Create an array from an n-api array
             *
             * @param array the n-api array
             */
            inline array(const Napi::Array &array) : values() {
                for (uint32_t i = 0; i < array.Length(); i++) {
                    values.push_back(getObject(array[i]));
                }
            }

            /**
             * Create an array from a n-api value
             *
             * @param value the value
             */
            inline array(const Napi::Value &value) : values() {
                if (!value.IsArray())
                    throw argumentMismatchException("class array requires a n-api value of type array");

                auto array = value.As<Napi::Array>();
                for (uint32_t i = 0; i < array.Length(); i++) {
                    values.push_back(getObject(array[i]));
                }
            }

#endif //NAPI_VERSION

            /**
             * Create an array from a vector of js_objects
             *
             * @param val the vector of js_objects
             */
            inline array(std::vector<::napi_tools::js_object> val) : values(std::move(val)) {}

            /**
             * Get the value at an index
             *
             * @param index the index
             * @return the obj l-value
             */
            inline objLValue operator[](int index) {
                return objLValue([this, index](const ::napi_tools::js_object &newPtr) {
                    values.insert(values.begin() + index, newPtr);
                }, values.at(index));
            }

            /**
             * Get the length of the array
             *
             * @return the length
             */
            [[nodiscard]] inline size_t length() const {
                return values.size();
            }

            /**
             * Get the underlying vector
             *
             * @return the vector
             */
            [[nodiscard]] inline std::vector<::napi_tools::js_object> getValues() const {
                return values;
            }

#ifdef NAPI_VERSION

            /**
             * Get this object as a n-api value
             *
             * @param env the environment to work in
             * @return the value
             */
            [[maybe_unused]] [[nodiscard]] inline Napi::Value getValue(const Napi::Env &env) const override {
                Napi::Array array = Napi::Array::New(env, values.size());
                for (size_t i = 0; i < values.size(); i++) {
                    array.Set((uint32_t) i, values[i]->getValue(env));
                }
                return array;
            }

#endif //NAPI_VERSION

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the two objects are equal
             */
            [[nodiscard]] inline bool operator==(const ::napi_tools::js_object &obj) const override {
                if (obj->getType() == this->getType() && this->length() == obj.as<array>()->length()) {
                    for (int i = 0; i < this->length(); ++i) {
                        if (this->getValues()[i] != obj.as<array>()->operator[](i).getValue()) {
                            return false;
                        }
                    }

                    return true;
                } else {
                    return false;
                }
            }

            /**
             * Get the type of this object
             *
             * @return the type
             */
            [[nodiscard]] inline js_type getType() const override {
                return js_type::array;
            }

            /**
             * Convert to string
             *
             * @return this value as a string
             */
            [[nodiscard]] inline std::string toString() const override {
                return "array";
            }

            /**
             * The destructor
             */
            inline ~array() override = default;

        private:
            std::vector<::napi_tools::js_object> values;
        };

        /**
         * object type
         */
        class object : public js_object {
        public:
#ifdef NAPI_VERSION

            /**
             * Create an object from an n-api object
             *
             * @param object the object
             */
            inline object(const Napi::Object &object) : contents() {
                Napi::Array names = object.GetPropertyNames();
                for (uint32_t i = 0; i < names.Length(); i++) {
                    ::napi_tools::js_object obj = getObject(object[i]);
                    contents.insert(std::make_pair(names[i].operator Napi::Value().As<Napi::String>(), obj));
                    values.push_back(obj);
                }
            }

            /**
             * Create an object from a n-api value
             *
             * @param value the value
             */
            inline object(const Napi::Value &value) : contents() {
                if (!value.IsObject())
                    throw argumentMismatchException("class object requires a n-api value of type object");

                auto object = value.As<Napi::Object>();
                Napi::Array names = object.GetPropertyNames();
                for (uint32_t i = 0; i < names.Length(); i++) {
                    ::napi_tools::js_object obj = getObject(object[i]);
                    contents.insert(std::make_pair(names[i].operator Napi::Value().As<Napi::String>(), obj));
                    values.push_back(obj);
                }
            }

#endif //NAPI_VERSION

            /**
             * Get the length of the object
             *
             * @return the length
             */
            [[nodiscard]] inline size_t length() const {
                return values.size();
            }

            /**
             * Get a value by a key
             *
             * @param key the key
             * @return the object l-value
             */
            inline objLValue operator[](const std::string &key) {
                ::napi_tools::js_object obj = contents.at(key);
                auto iter = std::find(values.begin(), values.end(), obj);

                return objLValue([this, key, iter](::napi_tools::js_object newPtr) {
                    contents.insert_or_assign(key, newPtr);
                    values.insert(iter, newPtr);
                }, obj);
            }

            /**
             * Get a value by an index
             *
             * @param index the index of the value
             * @return the object l-value
             */
            inline objLValue operator[](int index) {
                ::napi_tools::js_object obj = values.at(index);
                std::string key;
                for (const auto &p : contents) {
                    if (p.second == obj) {
                        key = p.first;
                        break;
                    }
                }

                return objLValue([this, key, index](::napi_tools::js_object newPtr) {
                    contents.insert_or_assign(key, newPtr);
                    values.insert(values.begin() + index, newPtr);
                }, obj);
            }

#ifdef NAPI_VERSION

            /**
             * Get this object as an n-api object
             *
             * @param env the environment to work with
             * @return the value
             */
            [[maybe_unused]] [[nodiscard]] inline Napi::Value getValue(const Napi::Env &env) const override {
                Napi::Object object = Napi::Object::New(env);
                for (const auto &p : contents) {
                    object.Set(p.first, p.second->getValue(env));
                }
                return object;
            }

#endif //NAPI_VERSION

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the two objects are equal
             */
            [[nodiscard]] inline bool operator==(const ::napi_tools::js_object &obj) const override {
                if (obj->getType() == this->getType() && this->length() == obj.as<object>()->length()) {
                    for (int i = 0; i < this->length(); ++i) {
                        if (this->values[i] != obj.as<object>()->operator[](i).getValue()) {
                            return false;
                        }
                    }

                    return true;
                } else {
                    return false;
                }
            }

            /**
             * Get the type of this object
             *
             * @return the type
             */
            [[nodiscard]] inline js_type getType() const override {
                return js_type::object;
            }

            /**
             * Convert to string
             *
             * @return this value as a string
             */
            [[nodiscard]] inline std::string toString() const override {
                return "object";
            }

            /**
             * The destructor
             */
            inline ~object() override = default;

        private:
            std::map<std::string, ::napi_tools::js_object> contents;
            std::vector<::napi_tools::js_object> values;
        };

#ifdef NAPI_VERSION // Disable function if n-api is not used

        /**
         * function type
         */
        class function : public js_object {
        public:
            /**
             * Get a function from an n-api object
             *
             * @param name the name of the function
             * @param object the object containing the function
             * @return the function object
             */
            inline static function getFromObject(const std::string &name, const Napi::Object &object) {
                return function(object.Get(name));
            }

            /**
             * Create a function from a n-api function
             *
             * @param func the function
             */
            inline function(const Napi::Function &func) : fn(func) {}

            /**
             * Create a function from a n-api value
             *
             * @param value the value
             */
            inline function(const Napi::Value &value) : fn(value.As<Napi::Function>()) {
                if (!value.IsFunction())
                    throw argumentMismatchException("object function requires n-api value of type function");
            }

            template<class callable>
            inline
            function(const Napi::Env &env, callable func) : fn(Napi::Function::New(env, func)) {}

            /**
             * Call the function
             *
             * @param env the environment to work with
             * @param val the object to call the function on
             * @param args the arguments to pass to the function
             * @return the result of the function call
             */
            inline ::napi_tools::js_object
            operator()(const Napi::Env &env, const napi_value &val, const std::vector<::napi_tools::js_object> &args) {
                std::vector<napi_value> values;
                for (const ::napi_tools::js_object &arg : args) {
                    values.push_back(arg->getValue(env));
                }

                return getObject(fn.Call(val, values));
            }

            /**
             * Call the function
             *
             * @tparam Args the argument types
             * @param env the environment to work with
             * @param val the object to call the function on
             * @param args the arguments to pass to the function
             * @return the result of the function call
             */
            template<class...Args>
            inline ::napi_tools::js_object operator()(const Napi::Env &env, const napi_value &val, Args...args) {
                return this->operator()(env, val, convertArgsToVector<::js_object>(args...));
            }

            /**
             * Get the function as a n-api value
             *
             * @return the value
             */
            [[maybe_unused]] [[nodiscard]] inline Napi::Value getValue(const Napi::Env &) const override {
                return fn;
            }

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the two objects are equal
             */
            [[nodiscard]] inline bool operator==(const ::napi_tools::js_object &obj) const override {
                return this->getType() == obj->getType() && this->fn == obj.as<function>()->fn;
            }

            /**
             * Get the type of the object
             *
             * @return the type
             */
            [[nodiscard]] inline js_type getType() const override {
                return js_type::function;
            }

            /**
             * Convert to string
             *
             * @return this value as a string
             */
            [[nodiscard]] inline std::string toString() const override {
                return "function";
            }

            /**
             * Get the n-api function of this class
             *
             * @return the stored n-api function
             */
            [[nodiscard]] inline operator Napi::Function() const {
                return fn;
            }

            /**
             * The destructor
             */
            inline ~function() = default;

        private:
            const Napi::Function fn;
        };

#endif //NAPI_VERSION
    }

#ifdef NAPI_VERSION

    /**
     * Specialized constructor for T = raw::js_object. Constructs a corresponding element from any n-api value.
     * Can be used in functions to get the arguments.
     *
     * @param value the n-api value to use
     */
    template<>
    inline js_object_ptr<raw::js_object>::js_object_ptr(const Napi::Value &value) {
        if (value.IsObject()) {
            ptr = (raw::js_object *) new raw::object(value);
        } else if (value.IsArray()) {
            ptr = (raw::js_object *) new raw::array(value);
        } else if (value.IsString()) {
            ptr = (raw::js_object *) new raw::string(value);
        } else if (value.IsBoolean()) {
            ptr = (raw::js_object *) new raw::boolean(value);
        } else if (value.IsNumber()) {
            ptr = (raw::js_object *) new raw::number(value);
        } else if (value.IsNull()) {
            ptr = (raw::js_object *) new raw::null();
        } else {
            ptr = (raw::js_object *) new raw::undefined();
        }
    }

#endif //NAPI_VERSION

    /**
     * Set a value. Only available when T = raw::js_object or T = raw::string
     *
     * @param value the value to set
     * @return this
     */
    template<>
    inline js_object_ptr<raw::js_object> &js_object_ptr<raw::js_object>::operator=(const char *value) {
        if (get()->getType() == raw::js_type::string) {
            ((raw::string *) ptr)->operator=(value);
        } else {
            delete ptr;
            ptr = new raw::string(value);
        }

        return *this;
    }

    /**
     * Set a value. Only available when T = raw::js_object or T = raw::string
     *
     * @param value the value to set
     * @return this
     */
    template<>
    inline js_object_ptr<raw::js_object> &js_object_ptr<raw::js_object>::operator=(std::string value) {
        if (get()->getType() == raw::js_type::string) {
            ((raw::string *) ptr)->assign(value);
        } else {
            delete ptr;
            ptr = new raw::string(value.begin(), value.end());
        }

        return *this;
    }

    /**
     * Set a value. Only available when T = raw::js_object or T = raw::number
     *
     * @param value the value to set
     * @return this
     */
    template<>
    inline js_object_ptr<raw::js_object> &js_object_ptr<raw::js_object>::operator=(int value) {
        if (get()->getType() == raw::js_type::number) {
            ((raw::number *) ptr)->operator=(value);
        } else {
            delete ptr;
            ptr = new raw::number(value);
        }

        return *this;
    }

    /**
     * Set a value. Only available when T = raw::js_object or T = raw::number
     *
     * @param value the value to set
     * @return this
     */
    template<>
    inline js_object_ptr<raw::js_object> &js_object_ptr<raw::js_object>::operator=(double value) {
        if (get()->getType() == raw::js_type::number) {
            ((raw::number *) ptr)->operator=(value);
        } else {
            delete ptr;
            ptr = new raw::number(value);
        }

        return *this;
    }

    /**
     * Set a value. Only available when T = raw::js_object or T = raw::boolean
     *
     * @param value the value to set
     * @return this
     */
    template<>
    inline js_object_ptr<raw::js_object> &js_object_ptr<raw::js_object>::operator=(bool value) {
        if (get()->getType() == raw::js_type::boolean) {
            ((raw::boolean *) ptr)->operator=(value);
        } else {
            delete ptr;
            ptr = new raw::boolean(value);
        }

        return *this;
    }

#ifdef NAPI_VERSION

    /**
     * Overloaded operator= for raw::js_object to set a n-api value
     *
     * @param value the value to set
     * @return this
     */
    template<>
    inline js_object_ptr<raw::js_object> &js_object_ptr<raw::js_object>::operator=(const Napi::Value &value) {
        delete ptr;

        if (value.IsObject()) {
            ptr = (raw::js_object *) new raw::object(value);
        } else if (value.IsArray()) {
            ptr = (raw::js_object *) new raw::array(value);
        } else if (value.IsString()) {
            ptr = (raw::js_object *) new raw::string(value);
        } else if (value.IsBoolean()) {
            ptr = (raw::js_object *) new raw::boolean(value);
        } else if (value.IsNumber()) {
            ptr = (raw::js_object *) new raw::number(value);
        } else if (value.IsNull()) {
            ptr = (raw::js_object *) new raw::null();
        } else {
            ptr = (raw::js_object *) new raw::undefined();
        }

        return *this;
    }

    /**
     * Get an object from a n-api value
     *
     * @param val the value
     * @return the object
     */
    inline js_object getObject(const Napi::Value &val) {
        if (val.IsObject()) {
            return js_object_ptr<raw::object>::make(val);
        } else if (val.IsArray()) {
            return js_object_ptr<raw::array>::make(val);
        } else if (val.IsString()) {
            return js_object_ptr<raw::string>::make(val);
        } else if (val.IsBoolean()) {
            return js_object_ptr<raw::boolean>::make(val);
        } else if (val.IsNumber()) {
            return js_object_ptr<raw::number>::make(val);
        } else if (val.IsNull()) {
            return js_object_ptr<raw::null>::make();
        } else {
            return js_object_ptr<raw::undefined>::make();
        }
    }

#endif //NAPI_VERSION

    /**
     * Copy a js_object
     *
     * @param current the object to copy
     * @return the result
     */
    inline raw::js_object *getObjectFromExisting(const raw::js_object *current) {
        if (current->getType() == raw::js_type::object) {
            return new raw::object(*((raw::object *) current));
        } else if (current->getType() == raw::js_type::array) {
            return new raw::array(*((raw::array *) current));
        } else if (current->getType() == raw::js_type::string) {
            return new raw::string(*((raw::string *) current));
        } else if (current->getType() == raw::js_type::boolean) {
            return new raw::boolean(*((raw::boolean *) current));
        } else if (current->getType() == raw::js_type::number) {
            return new raw::number(*((raw::number *) current));
        } else if (current->getType() == raw::js_type::null) {
            return new raw::null();
        } else {
            return new raw::undefined();
        }
    }

#ifdef NAPI_VERSION // Everything down here is n-api only

    /**
     * Require a node.js object
     *
     * @param env the environment to work with
     * @param toRequire the package to require
     * @return the acquired package object
     */
    inline Napi::Object require(const Napi::Env &env, const raw::string &toRequire) {
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

    using thread_entry = std::function<js_object(const ThreadSafeFunction &)>;

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
        js_object result;
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
        [[maybe_unused]] inline Napi::String stringify(const Napi::Env &env, const object &object) {
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
        [[maybe_unused]] inline Napi::Object parse(const Napi::Env &env, const string &string) {
            auto json = env.Global().Get("JSON").As<Napi::Object>();
            auto parse = json.Get("parse").As<Napi::Function>();
            return parse.Call(json, {string->getValue(env)}).As<Napi::Object>();
        }
    }
#endif //NAPI_VERSION
}

#endif //NAPI_TOOLS_NAPI_TOOLS_HPP
