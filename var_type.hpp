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
#ifndef NAPI_TOOLS_VAR_TYPE_HPP
#define NAPI_TOOLS_VAR_TYPE_HPP

#include <regex>
#include <utility>
#include <map>
#include <type_traits>
#include <functional>
#include <thread>

/**
 * N-api tools namespace
 */
namespace var_type {
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
        inline js_object_ptr();

        /**
         * Create a js_object_ptr from an existing poitner
         *
         * @param obj_ptr the already existing pointer
         */
        explicit inline js_object_ptr(T *obj_ptr);

        /**
         * Create an object from a value.
         * Will be disabled if T = raw::js_object. For this case, specialized constructors are available.
         *
         * @tparam U the value type
         * @param obj the object
         */
        template<class U>
        inline js_object_ptr(U obj);

#ifdef NAPI_VERSION

        /**
         * Construct from a n-api value
         *
         * @param value the value to construct from
         */
        inline js_object_ptr(const Napi::Value &value);

#endif //NAPI_VERSION

        // Specialized constructors. Only available when T = raw::js_object

        /**
         * Create a js_object_ptr<raw::js_object> from a char array.
         * Will create a js_object_ptr<raw::string> and cast it to raw::js_object
         *
         * @param c the char array containing data
         */
        inline js_object_ptr(const char *c);

        /**
         * Create a js_object_ptr<raw::js_object> from a string.
         * Will create a js_object_ptr<raw::string> and cast it to raw::js_object
         *
         * @param s the string containing the data
         */
        inline js_object_ptr(const std::string &s);

        /**
         * Create a js_object_ptr<raw::js_object> from a bool value.
         * Will create a js_object_ptr<raw::boolean> and cast it to raw::js_object
         *
         * @param b the bool value
         */
        inline js_object_ptr(bool b);

        /**
         * Create a js_object_ptr<raw::js_object> from a integer value.
         * Will create a js_object_ptr<raw::number> and cast it to raw::js_object
         *
         * @param i the integer value
         */
        inline js_object_ptr(int i);

        /**
         * Create a js_object_ptr<raw::js_object> from a double value.
         * Will create a js_object_ptr<raw::number> and cast it to raw::js_object
         *
         * @param d the double value
         */
        inline js_object_ptr(double d);

        /**
         * Create a js_object_ptr<raw::js_object> from a vector<js_object_ptr<raw::js_object>> value.
         * Will create a js_object_ptr<raw::array> and cast it to raw::js_object
         *
         * @param data the data array
         */
        inline js_object_ptr(const std::vector<js_object_ptr<raw::js_object>> data);

        /**
         * Copy constructor
         *
         * @tparam U the object type to copy from
         * @param obj the object to copy from
         */
        template<class U>
        inline js_object_ptr(const js_object_ptr<U> &obj);

        /**
         * Copy constructor
         *
         * @param other the object to copy from
         */
        inline js_object_ptr(const js_object_ptr &other);

        /**
         * Copy constructor
         *
         * @param obj the object to copy from
         */
        inline js_object_ptr(const T &obj);

        /**
         * Operator =
         *
         * @param other the pointer to copy from
         * @return this object
         */
        inline js_object_ptr<T> &operator=(const js_object_ptr<T> &other);

        /**
         * operator =
         *
         * @tparam U the type of the other pointer
         * @param other the other pointer
         * @return this
         */
        template<class U>
        inline js_object_ptr<T> &operator=(const js_object_ptr<U> &other);

        /**
         * Set a value
         *
         * @tparam U the type of the value
         * @param value the value
         * @return this
         */
        template<class U>
        inline js_object_ptr<T> &operator=(U value);

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::string
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(const char *value);

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::string
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(std::string value);

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::number
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(int value);

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::number
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(double value);

        /**
         * Set a value. Only available when T = raw::js_object or T = raw::boolean
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(bool value);

#ifdef NAPI_VERSION

        /**
         * Set a n-api value
         *
         * @param value the value to set
         * @return this
         */
        inline js_object_ptr<T> &operator=(const Napi::Value &value);

#endif //NAPI_VERSION

        /**
         * operator []
         *
         * @tparam U the type of the search value
         * @param searchValue the search value
         * @return the result of the operation
         */
        template<class U>
        inline auto operator[](const U &searchValue);

        /**
         * operator +
         *
         * @param other the object to add
         * @return the result of the operation
         */
        template<class U>
        inline js_object_ptr<T> operator+(const js_object_ptr<U> &other);

        /**
         * operator +
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return the result of the operation
         */
        template<class U>
        inline js_object_ptr<T> operator+(U val);

        /**
         * operator +
         *
         * @param other the object to subtract
         * @return the result of the operation
         */
        [[nodiscard]] inline js_object_ptr<T> operator-(const js_object_ptr<T> &other) const;

        /**
         * operator -
         *
         * @tparam U the type of the value to subtract
         * @param val the value to subtract
         * @return the result of the operation
         */
        template<class U>
        [[nodiscard]] inline js_object_ptr<T> operator-(const U &val) const;

        /**
         * operator *
         *
         * @param other the object to multiply with
         * @return the result of the operation
         */
        [[nodiscard]] inline js_object_ptr<T> operator*(const js_object_ptr<T> &other) const;

        /**
         * operator *
         *
         * @tparam U the type of the value to multiply with
         * @param val the value to multiply with
         * @return the result of the operation
         */
        template<class U>
        [[nodiscard]] inline js_object_ptr<T> operator*(const U &val) const;

        /**
         * operator /
         *
         * @param other the object to add
         * @return the result of the operation
         */
        [[nodiscard]] inline js_object_ptr<T> operator/(const js_object_ptr<T> &other) const;

        /**
         * operator /
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return the result of the operation
         */
        template<class U>
        [[nodiscard]] inline js_object_ptr<T> operator/(const U &val) const;

        /**
         * Operator +=
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return this
         */
        template<class U>
        inline js_object_ptr<T> &operator+=(U val);

        /**
         * Operator -=
         *
         * @tparam U the type of the value to subtract
         * @param val the value to subtract
         * @return this
         */
        template<class U>
        inline js_object_ptr<T> &operator-=(U val);

        /**
         * Operator *=
         *
         * @tparam U the type of the value to multiply
         * @param val the value to multiply
         * @return this
         */
        template<class U>
        inline js_object_ptr<T> &operator*=(U val);

        /**
         * Operator /=
         *
         * @tparam U the type of the value to divide
         * @param val the value to divide
         * @return this
         */
        template<class U>
        inline js_object_ptr<T> &operator/=(U val);

        /**
         * Operator ++
         *
         * @return this
         */
        inline const js_object_ptr<T> operator++(int n);

        /**
         * Operator --
         *
         * @return this
         */
        inline const js_object_ptr<T> operator--(int n);

        /**
         * Operator <
         *
         * @tparam T the type of the value to compare with
         * @param val the value to compare with
         * @return the operation result
         */
        template<class U>
        [[nodiscard]] inline bool operator<(const U &val) const;

        /**
         * Operator >
         *
         * @tparam T the type of the value to compare with
         * @param val the value to compare with
         * @return the operation result
         */
        template<class U>
        [[nodiscard]] inline bool operator>(const U &val) const;

        /**
         * Operator <=
         *
         * @tparam T the type of the value to compare with
         * @param val the value to compare with
         * @return the operation result
         */
        template<class U>
        [[nodiscard]] inline bool operator<=(const U &val) const;

        /**
         * Operator >=
         *
         * @tparam T the type of the value to compare with
         * @param val the value to compare with
         * @return the operation result
         */
        template<class U>
        [[nodiscard]] inline bool operator>=(const U &val) const;

        /**
         * Operator <
         *
         * @tparam T the type of the value to compare with
         * @param val the value to compare with
         * @return the operation result
         */
        template<class U>
        [[nodiscard]] inline bool operator<(const js_object_ptr<U> &val) const;

        /**
         * Operator >
         *
         * @tparam T the type of the value to compare with
         * @param val the value to compare with
         * @return the operation result
         */
        template<class U>
        [[nodiscard]] inline bool operator>(const js_object_ptr<U> &val) const;

        /**
         * Operator <=
         *
         * @tparam T the type of the value to compare with
         * @param val the value to compare with
         * @return the operation result
         */
        template<class U>
        [[nodiscard]] inline bool operator<=(const js_object_ptr<U> &val) const;

        /**
         * Operator >=
         *
         * @tparam T the type of the value to compare with
         * @param val the value to compare with
         * @return the operation result
         */
        template<class U>
        [[nodiscard]] inline bool operator>=(const js_object_ptr<U> &val) const;

        /**
         * Get this pointer as another object
         *
         * @tparam U the type to cast to
         * @return the resulting pointer
         */
        template<class U>
        [[nodiscard]] inline U *as() const;

        /**
         * Convert to another js_object_ptr
         *
         * @tparam U the target type
         * @return the resulting pointer
         */
        template<class U>
        [[nodiscard]] inline js_object_ptr<U> to() const;

        /**
         * Convert this to js_object_ptr<raw::js_object>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::js_object> asRawObject() const;

        // Convert to various types. Only available when T = raw::js_object

        /**
         * Convert this to js_object_ptr<raw::string>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::string> asString() const;

        /**
         * Convert this to js_object_ptr<raw::number>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::number> asNumber() const;

        /**
         * Convert this to js_object_ptr<raw::boolean>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::boolean> asBoolean() const;

        /**
         * Convert this to js_object_ptr<raw::array>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::array> asArray() const;

        /**
         * Convert this to js_object_ptr<raw::object>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::object> asObject() const;

#ifdef NAPI_VERSION
        /**
         * Convert this to js_object_ptr<raw::function>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::function> asFunction() const;
#endif //NAPI_VERSION

        // Overloaded operators for std::string, bool and double
        // Only available for the specific classes storing these

        /**
         * operator std::string
         *
         * @return the string value
         */
        [[nodiscard]] inline operator std::string() const;

        /**
         * operator bool
         *
         * @return the bool value
         */
        [[nodiscard]] inline operator bool() const;

        /**
         * operator double
         *
         * @return the double value
         */
        [[nodiscard]] inline operator double() const;

        /**
         * Operator ->
         *
         * @return the pointer
         */
        [[nodiscard]] inline T *operator->() const;

        /**
         * The operator equals
         *
         * @tparam U the type to compare with
         * @param other the object to compare with
         * @return true, if both objects are equal
         */
        template<class U>
        [[nodiscard]] inline bool operator==(const js_object_ptr<U> &other) const;

        /**
         * The operator not equals
         *
         * @tparam U the type to compare with
         * @param other the object to compare with
         * @return true, if both objects are not equal
         */
        template<class U>
        [[nodiscard]] inline bool operator!=(const js_object_ptr<U> &other) const;

        /**
         * Operator << overload for output streams
         *
         * @param os the output stream to append this to
         * @param data the data to append
         * @return the output stream
         */
        friend inline std::ostream &operator<<(std::ostream &os, const js_object_ptr<T> &data) {
            os << data->toString();
            return os;
        }

        /**
         * Get the underlying pointer
         *
         * @return the underlying pointer
         */
        [[nodiscard]] inline T *get() const;

        /**
         * Reset the pointer
         */
        inline void reset();

        /**
         * The destructor
         */
        inline ~js_object_ptr();

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
            [[nodiscard]] inline bool operator==(const ::var_type::js_object &obj) const override {
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
            [[nodiscard]] inline bool operator==(const ::var_type::js_object &obj) const override {
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
            [[nodiscard]] inline bool operator==(const ::var_type::js_object &obj) const override {
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

                return *this;
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
            [[nodiscard]] inline bool operator==(const ::var_type::js_object &obj) const override {
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

            /**
             * Operator +
             *
             * @tparam T the type of the value to add
             * @param val the value to add
             * @return the result of the operation
             */
            template<class T>
            inline auto operator+(T val) {
                return (value + val);
            }

            /**
             * Operator -
             *
             * @tparam T the type of the value to subtract
             * @param val the value to subtract
             * @return the result of the operation
             */
            template<class T>
            inline auto operator-(T val) {
                return value + val;
            }

            /**
             * Operator *
             *
             * @tparam T the type of the value to multiply
             * @param val the value to multiply
             * @return the result of the operation
             */
            template<class T>
            inline auto operator*(T val) {
                return value * val;
            }

            /**
             * Operator /
             *
             * @tparam T the type of the value to divide
             * @param val the value to divide
             * @return the result of the operation
             */
            template<class T>
            inline auto operator/(T val) {
                return value / val;
            }

            /**
             * Operator +=
             *
             * @tparam T the type of the value to add
             * @param val the value to add
             * @return this
             */
            template<class T>
            inline number &operator+=(T val) {
                value += val;
                return *this;
            }

            /**
             * Operator -=
             *
             * @tparam T the type of the value to subtract
             * @param val the value to subtract
             * @return this
             */
            template<class T>
            inline number &operator-=(T val) {
                value -= val;
                return *this;
            }

            /**
             * Operator *=
             *
             * @tparam T the type of the value to multiply
             * @param val the value to multiply
             * @return this
             */
            template<class T>
            inline number &operator*=(T val) {
                value *= val;
                return *this;
            }

            /**
             * Operator /=
             *
             * @tparam T the type of the value to divide
             * @param val the value to divide
             * @return this
             */
            template<class T>
            inline number &operator/=(T val) {
                value /= val;
                return *this;
            }

            /**
             * Operator ++
             *
             * @return this
             */
            inline number &operator++(int) {
                value = value + 1;
                return *this;
            }

            /**
             * Operator --
             *
             * @return this
             */
            inline number &operator--(int) {
                value = value - 1;
                return *this;
            }

            /**
             * Operator <
             *
             * @tparam T the type of the value to compare with
             * @param val the value to compare with
             * @return the operation result
             */
            template<class T>
            inline bool operator<(T val) {
                return value < val;
            }

            /**
             * Operator >
             *
             * @tparam T the type of the value to compare with
             * @param val the value to compare with
             * @return the operation result
             */
            template<class T>
            inline bool operator>(T val) {
                return value > val;
            }

            /**
             * Operator <=
             *
             * @tparam T the type of the value to compare with
             * @param val the value to compare with
             * @return the operation result
             */
            template<class T>
            inline bool operator<=(T val) {
                return value <= val;
            }

            /**
             * Operator >=
             *
             * @tparam T the type of the value to compare with
             * @param val the value to compare with
             * @return the operation result
             */
            template<class T>
            inline bool operator>=(T val) {
                return value >= val;
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
            [[nodiscard]] inline bool operator==(const ::var_type::js_object &obj) const override {
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
            explicit inline objLValue(std::function<void(::var_type::js_object)> setter,
                                      const ::var_type::js_object &ptr)
                    : setter(std::move(setter)), ptr(ptr) {}

            /**
             * The object setter
             *
             * @param val the value
             * @return this
             */
            inline objLValue &operator=(const ::var_type::js_object &val) {
                setter(val);
                ptr = val;

                return *this;
            }

            /**
             * operator ->
             *
             * @return the object
             */
            [[nodiscard]] inline ::var_type::js_object operator->() const {
                return ptr;
            }

            /**
             * Get the pointer
             *
             * @return the pointer
             */
            [[nodiscard]] inline ::var_type::js_object getValue() const {
                return ptr;
            }

            /**
             * Get the pointer
             *
             * @return the pointer
             */
            [[nodiscard]] inline operator ::var_type::js_object() const {
                return ptr;
            }

        private:
            std::function<void(::var_type::js_object)> setter;
            ::var_type::js_object ptr;
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
            inline array(std::vector<::var_type::js_object> val) : values(std::move(val)) {}

            /**
             * Get the value at an index
             *
             * @param index the index
             * @return the obj l-value
             */
            inline objLValue operator[](int index) {
                return objLValue([this, index](const ::var_type::js_object &newPtr) {
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
            [[nodiscard]] inline std::vector<::var_type::js_object> getValues() const {
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

            /**
             * Append an n-api array
             *
             * @param array
             */
            inline array &append(const Napi::Array &array) {
                for (uint32_t i = 0; i < array.Length(); i++) {
                    values.push_back(getObject(array[i]));
                }

                return *this;
            }

            /**
             * Append an n-api array
             *
             * @param value the value
             */
            inline array &append(const Napi::Value &value) {
                if (!value.IsArray())
                    throw argumentMismatchException("class array requires a n-api value of type array");

                auto array = value.As<Napi::Array>();
                for (uint32_t i = 0; i < array.Length(); i++) {
                    values.push_back(getObject(array[i]));
                }

                return *this;
            }

#endif //NAPI_VERSION

            /**
             * Append a vector of js_objects
             *
             * @param data the vector to append
             */
            inline array &append(const std::vector<::var_type::js_object> &data) {
                values.insert(values.end(), data.begin(), data.end());
                return *this;
            }

            /**
             * Append data. May be a vector of js_objects, or an n-api array
             *
             * @tparam T the type of the data to append
             * @param data the data to append
             */
            template<class T>
            inline void operator+=(const T &data) {
                this->append(data);
            }

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the two objects are equal
             */
            [[nodiscard]] inline bool operator==(const ::var_type::js_object &obj) const override {
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
            std::vector<::var_type::js_object> values;
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
                    ::var_type::js_object obj = getObject(object[i]);
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
                    ::var_type::js_object obj = getObject(object[i]);
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
                ::var_type::js_object obj = contents.at(key);
                auto iter = std::find(values.begin(), values.end(), obj);

                return objLValue([this, key, iter](::var_type::js_object newPtr) {
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
                ::var_type::js_object obj = values.at(index);
                std::string key;
                for (const auto &p : contents) {
                    if (p.second == obj) {
                        key = p.first;
                        break;
                    }
                }

                return objLValue([this, key, index](::var_type::js_object newPtr) {
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
            [[nodiscard]] inline bool operator==(const ::var_type::js_object &obj) const override {
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
            std::map<std::string, ::var_type::js_object> contents;
            std::vector<::var_type::js_object> values;
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
            inline ::var_type::js_object
            operator()(const Napi::Env &env, const napi_value &val, const std::vector<::var_type::js_object> &args) {
                std::vector<napi_value> values;
                for (const ::var_type::js_object &arg : args) {
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
            inline ::var_type::js_object operator()(const Napi::Env &env, const napi_value &val, Args...args) {
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
            [[nodiscard]] inline bool operator==(const ::var_type::js_object &obj) const override {
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

    // js_object_ptr function definitions =====================================================================

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

    template<class T>
    inline js_object_ptr<T>::js_object_ptr() {
        // Create new instance of empty T
        ptr = new T();
    }

    template<class T>
    inline js_object_ptr<T>::js_object_ptr(T *obj_ptr) {
        ptr = obj_ptr;
    }

    template<class T>
    template<class U>
    inline js_object_ptr<T>::js_object_ptr(U obj) {
        static_assert(!std::is_same_v<raw::js_object, T>,
                      "raw::js_object cannot be constructed using given type U.");
        ptr = new T(obj);
    }

#ifdef NAPI_VERSION

    template<class T>
    inline js_object_ptr<T>::js_object_ptr(const Napi::Value &value) {
        ptr = new T(value);
    }

#endif //NAPI_VERSION

    // Specialized constructors. Only available when T = raw::js_object
    template<class T>
    inline js_object_ptr<T>::js_object_ptr(const char *c) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::string, T>,
                      "Specialized constructors are not allowed when the type is not raw::js_object");
        ptr = (T *) new raw::string(c);
    }

    template<class T>
    inline js_object_ptr<T>::js_object_ptr(const std::string &s) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::string, T>,
                      "Specialized constructors are not allowed when the type is not raw::js_object");
        ptr = (T *) new raw::string(s.begin(), s.end());
    }

    template<class T>
    inline js_object_ptr<T>::js_object_ptr(bool b) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::boolean, T>,
                      "Specialized constructors are not allowed when the type is not raw::js_object");
        ptr = (T *) new raw::boolean(b);
    }

    template<class T>
    inline js_object_ptr<T>::js_object_ptr(int i) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "Specialized constructors are not allowed when the type is not raw::js_object");
        ptr = (T *) new raw::number(i);
    }

    template<class T>
    inline js_object_ptr<T>::js_object_ptr(double d) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "Specialized constructors are not allowed when the type is not raw::js_object");
        ptr = (T *) new raw::number(d);
    }

    template<class T>
    inline js_object_ptr<T>::js_object_ptr(const std::vector<js_object_ptr<raw::js_object>> data) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::array, T>,
                      "Specialized constructors are not allowed when the type is not raw::js_object");
        ptr = (T *) new raw::array(data);
    }

    template<class T>
    template<class U>
    inline js_object_ptr<T>::js_object_ptr(const js_object_ptr<U> &obj) {
        if constexpr (std::is_same_v<raw::js_object, U>) {
            // If U = raw::js_object, cast the data to T and then create new T from it
            ptr = new T(*obj.template as<T>());
        } else {
            ptr = (T *) new U(*obj.get());
        }
    }

    template<class T>
    inline js_object_ptr<T>::js_object_ptr(const js_object_ptr &other) {
        ptr = new T(*other.get());
    }

    template<class T>
    inline js_object_ptr<T>::js_object_ptr(const T &obj) {
        ptr = new T(obj);
    }

    template<class T>
    inline js_object_ptr<T> &js_object_ptr<T>::operator=(const js_object_ptr<T> &other) {
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

    template<class T>
    template<class U>
    inline js_object_ptr<T> &js_object_ptr<T>::operator=(const js_object_ptr<U> &other) {
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

    template<class T>
    template<class U>
    inline js_object_ptr<T> &js_object_ptr<T>::operator=(U value) {
        ptr->operator=(value);

        return *this;
    }

    template<class T>
    inline js_object_ptr<T> &js_object_ptr<T>::operator=(const char *value) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::string, T>,
                      "this operator overload is only available when T = raw::js_object or T = raw::string");
        ptr->operator=(value);

        return *this;
    }

    template<class T>
    inline js_object_ptr<T> &js_object_ptr<T>::operator=(std::string value) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::string, T>,
                      "this operator overload is only available when T = raw::js_object or T = raw::string");
        ptr->operator=(value);

        return *this;
    }

    template<class T>
    inline js_object_ptr<T> &js_object_ptr<T>::operator=(int value) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "this operator overload is only available when T = raw::js_object or T = raw::number");
        ptr->operator=(value);

        return *this;
    }

    template<class T>
    inline js_object_ptr<T> &js_object_ptr<T>::operator=(double value) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "this operator overload is only available when T = raw::js_object or T = raw::number");
        ptr->operator=(value);

        return *this;
    }

    template<class T>
    inline js_object_ptr<T> &js_object_ptr<T>::operator=(bool value) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::boolean, T>,
                      "this operator overload is only available when T = raw::js_object or T = raw::boolean");
        ptr->operator=(value);

        return *this;
    }

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

    template<class T>
    inline js_object_ptr<T> &js_object_ptr<T>::operator=(const Napi::Value &value) {
        delete ptr;
        ptr = new T(value);

        return *this;
    }

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

    template<class T>
    template<class U>
    inline auto js_object_ptr<T>::operator[](const U &searchValue) {
        return ptr->operator[](searchValue);
    }

    template<class T>
    template<class U>
    inline js_object_ptr<T> js_object_ptr<T>::operator+(const js_object_ptr<U> &other) {
        if constexpr (std::is_same_v<raw::string, U> || std::is_same_v<raw::string, T>) {
            std::string s(ptr->toString());
            s.append(other->toString());

            return s;
        } else {
            return *ptr + *other.ptr;
        }
    }

    template<class T>
    template<class U>
    inline js_object_ptr<T> js_object_ptr<T>::operator+(U val) {
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

    template<class T>
    [[nodiscard]] inline js_object_ptr<T> js_object_ptr<T>::operator-(const js_object_ptr<T> &other) const {
        return this->operator-(other.operator double());
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline js_object_ptr<T> js_object_ptr<T>::operator-(const U &val) const {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "operator- is only available when T = raw::js_object and its type is number or T = raw::number");
        if constexpr (std::is_same_v<raw::number, T>) {
            return ptr->operator-(val);
        } else {
            if (ptr->isNumber()) {
                return this->as<raw::number>()->operator-(val);
            } else {
                throw argumentMismatchException(
                        "operator- is only available when T = raw::js_object and its type is number or T = raw::number");
            }
        }
    }

    template<class T>
    [[nodiscard]] inline js_object_ptr<T> js_object_ptr<T>::operator*(const js_object_ptr<T> &other) const {
        return this->operator*(other.operator double());
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline js_object_ptr<T> js_object_ptr<T>::operator*(const U &val) const {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "operator* is only available when T = raw::js_object and its type is number or T = raw::number");
        if constexpr (std::is_same_v<raw::number, T>) {
            return ptr->operator*(val);
        } else {
            if (ptr->isNumber()) {
                return this->as<raw::number>()->operator*(val);
            } else {
                throw argumentMismatchException(
                        "operator* is only available when T = raw::js_object and its type is number or T = raw::number");
            }
        }
    }

    template<class T>
    [[nodiscard]] inline js_object_ptr<T> js_object_ptr<T>::operator/(const js_object_ptr<T> &other) const {
        return this->operator/(other.operator double());
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline js_object_ptr<T> js_object_ptr<T>::operator/(const U &val) const {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "operator/ is only available when T = raw::js_object and its type is number or T = raw::number");
        if constexpr (std::is_same_v<raw::number, T>) {
            return ptr->operator/(val);
        } else {
            if (ptr->isNumber()) {
                return this->as<raw::number>()->operator/(val);
            } else {
                throw argumentMismatchException(
                        "operator/ is only available when T = raw::js_object and its type is number or T = raw::number");
            }
        }
    }

    template<class T>
    template<class U>
    inline js_object_ptr<T> &js_object_ptr<T>::operator+=(U val) {
        return this->operator=(this->operator+(val));
    }

    template<class T>
    template<class U>
    inline js_object_ptr<T> &js_object_ptr<T>::operator-=(U val) {
        return this->operator=(this->operator-(val));
    }

    template<class T>
    template<class U>
    inline js_object_ptr<T> &js_object_ptr<T>::operator*=(U val) {
        return this->operator=(this->operator*(val));
    }

    template<class T>
    template<class U>
    inline js_object_ptr<T> &js_object_ptr<T>::operator/=(U val) {
        return this->operator=(this->operator/(val));
    }

    template<class T>
    inline const js_object_ptr<T> js_object_ptr<T>::operator++(int n) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "operator++ is only available when T = number or T = js_object and its type is number");
        if constexpr (std::is_same_v<raw::number, T>) {
            ptr->operator++(n);
        } else {
            if (ptr->isNumber()) {
                ((raw::number *) ptr)->operator++(n);
            } else {
                throw argumentMismatchException(
                        "operator++ is only available when T = number or T = js_object and its type is number");
            }
        }

        return *this;
    }

    template<class T>
    inline const js_object_ptr<T> js_object_ptr<T>::operator--(int n) {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "operator-- is only available when T = number or T = js_object and its type is number");
        if constexpr (std::is_same_v<raw::number, T>) {
            ptr->operator--(n);
        } else {
            if (ptr->isNumber()) {
                ((raw::number *) ptr)->operator--(n);
            } else {
                throw argumentMismatchException(
                        "operator-- is only available when T = number or T = js_object and its type is number");
            }
        }

        return *this;
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator<(const U &val) const {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "operator < is only available if T is one of raw::js_object, raw::number and their type is number");
        if constexpr (std::is_same_v<raw::number, T>) {
            return ptr->operator<(val);
        } else {
            if (ptr->isNumber()) {
                return ((raw::number *) ptr)->operator<(val);
            } else {
                throw argumentMismatchException(
                        "operator < is only available if T is one of raw::js_object, raw::number and their type is number");
            }
        }
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator>(const U &val) const {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "operator > is only available if T is one of raw::js_object, raw::number and their type is number");
        if constexpr (std::is_same_v<raw::number, T>) {
            return ptr->operator>(val);
        } else {
            std::vector<raw::number> v;
            if (ptr->isNumber()) {
                return ((raw::number *) ptr)->operator>(val);
            } else {
                throw argumentMismatchException(
                        "operator > is only available if T is one of raw::js_object, raw::number and their type is number");
            }
        }
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator<=(const U &val) const {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "operator <= is only available if T is one of raw::js_object, raw::number and their type is number");
        if constexpr (std::is_same_v<raw::number, T>) {
            return ptr->operator<=(val);
        } else {
            if (ptr->isNumber()) {
                return ((raw::number *) ptr)->operator<=(val);
            } else {
                throw argumentMismatchException(
                        "operator <= is only available if T is one of raw::js_object, raw::number and their type is number");
            }
        }
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator>=(const U &val) const {
        static_assert(std::is_same_v<raw::js_object, T> || std::is_same_v<raw::number, T>,
                      "operator >= is only available if T is one of raw::js_object, raw::number and their type is number");
        if constexpr (std::is_same_v<raw::number, T>) {
            return ptr->operator>=(val);
        } else {
            if (ptr->isNumber()) {
                return ((raw::number *) ptr)->operator>=(val);
            } else {
                throw argumentMismatchException(
                        "operator >= is only available if T is one of raw::js_object, raw::number and their type is number");
            }
        }
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator<(const js_object_ptr<U> &val) const {
        static_assert(std::is_same_v<raw::js_object, U> || std::is_same_v<raw::number, U>,
                      "operator < is only available if U is one of raw::js_object, raw::number and their type is number");
        if constexpr (std::is_same_v<raw::number, U>) {
            return this->operator<(val.operator double());
        } else {
            if (val->isNumber()) {
                return this->operator<(val.asNumber().operator double());
            } else {
                throw argumentMismatchException(
                        "operator < is only available if U is one of raw::js_object, raw::number and their type is number");
            }
        }
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator>(const js_object_ptr<U> &val) const {
        static_assert(std::is_same_v<raw::js_object, U> || std::is_same_v<raw::number, U>,
                      "operator > is only available if U is one of raw::js_object, raw::number and their type is number");
        if constexpr (std::is_same_v<raw::number, U>) {
            return this->operator>(val.operator double());
        } else {
            if (val->isNumber()) {
                return this->operator>(val.asNumber().operator double());
            } else {
                throw argumentMismatchException(
                        "operator > is only available if U is one of raw::js_object, raw::number and their type is number");
            }
        }
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator<=(const js_object_ptr<U> &val) const {
        static_assert(std::is_same_v<raw::js_object, U> || std::is_same_v<raw::number, U>,
                      "operator <= is only available if U is one of raw::js_object, raw::number and their type is number");
        if constexpr (std::is_same_v<raw::number, U>) {
            return this->operator<=(val.operator double());
        } else {
            if (val->isNumber()) {
                return this->operator<=(val.asNumber().operator double());
            } else {
                throw argumentMismatchException(
                        "operator <= is only available if U is one of raw::js_object, raw::number and their type is number");
            }
        }
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator>=(const js_object_ptr<U> &val) const {
        static_assert(std::is_same_v<raw::js_object, U> || std::is_same_v<raw::number, U>,
                      "operator >= is only available if U is one of raw::js_object, raw::number and their type is number");
        if constexpr (std::is_same_v<raw::number, U>) {
            return this->operator>=(val);
        } else {
            if (val->isNumber()) {
                return this->operator>=(val.asNumber().operator double());
            } else {
                throw argumentMismatchException(
                        "operator >= is only available if U is one of raw::js_object, raw::number and their type is number");
            }
        }
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline U *js_object_ptr<T>::as() const {
        return (U *) ptr;
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline js_object_ptr<U> js_object_ptr<T>::to() const {
        return js_object_ptr<U>(new U(*((U *) ptr)));
    }

    template<class T>
    [[nodiscard]] inline js_object_ptr<raw::js_object> js_object_ptr<T>::asRawObject() const {
        return to<raw::js_object>();
    }

    template<class T>
    [[nodiscard]] inline js_object_ptr<raw::string> js_object_ptr<T>::asString() const {
        // Throw compile-time error if asString is not called on a raw::js_object
        static_assert(std::is_same_v<raw::js_object, T>, "asString can only be called on a raw object");
        if (ptr->getType() != raw::js_type::string)
            throw argumentMismatchException("asString can only be called on a raw object of type string");
        return to<raw::string>();
    }

    template<class T>
    [[nodiscard]] inline js_object_ptr<raw::number> js_object_ptr<T>::asNumber() const {
        static_assert(std::is_same_v<raw::js_object, T>, "asNumber can only be called on a raw object");
        if (ptr->getType() != raw::js_type::number)
            throw argumentMismatchException("asNumber can only be called on a raw object of type number");
        return to<raw::number>();
    }

    template<class T>
    [[nodiscard]] inline js_object_ptr<raw::boolean> js_object_ptr<T>::asBoolean() const {
        static_assert(std::is_same_v<raw::js_object, T>, "asBoolean can only be called on a raw object");
        if (ptr->getType() != raw::js_type::boolean)
            throw argumentMismatchException("asBoolean can only be called on a raw object of type boolean");
        return to<raw::boolean>();
    }

    template<class T>
    [[nodiscard]] inline js_object_ptr<raw::array> js_object_ptr<T>::asArray() const {
        static_assert(std::is_same_v<raw::js_object, T>, "asArray can only be called on a raw object");
        if (ptr->getType() != raw::js_type::string)
            throw argumentMismatchException("asArray can only be called on a raw object of type array");
        return to<raw::array>();
    }

    template<class T>
    [[nodiscard]] inline js_object_ptr<raw::object> js_object_ptr<T>::asObject() const {
        static_assert(std::is_same_v<raw::js_object, T>, "asObject can only be called on a raw object");
        if (ptr->getType() != raw::js_type::object)
            throw argumentMismatchException("asObject can only be called on a raw object of type object");
        return to<raw::object>();
    }

#ifdef NAPI_VERSION
    template<class T>
    [[nodiscard]] inline js_object_ptr<raw::function> js_object_ptr<T>::asFunction() const {
        static_assert(std::is_same_v<raw::js_object, T>, "asFunction can only be called on a raw object");
        if (ptr->getType() != raw::js_type::string)
            throw argumentMismatchException("asFunction can only be called on a raw object of type function");
        return to<raw::function>();
    }
#endif //NAPI_VERSION

    // Overloaded operators for std::string, bool and double
    // Only available for the specific classes storing these

    template<class T>
    [[nodiscard]] inline js_object_ptr<T>::operator std::string() const {
        static_assert(std::is_same_v<raw::string, T> || std::is_same_v<raw::js_object, T>,
                      "operator std::string can only be used with a string type");
        if constexpr (std::is_same_v<raw::string, T>) {
            return ptr->toString();
        } else { // T = raw::js_object
            if (ptr->isString()) {
                return ((raw::string *) ptr)->toString();
            } else {
                throw argumentMismatchException("Can not use operator std::string on non-string type");
            }
        }
    }

    template<class T>
    [[nodiscard]] inline js_object_ptr<T>::operator bool() const {
        static_assert(std::is_same_v<raw::boolean, T> || std::is_same_v<raw::js_object, T>,
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

    template<class T>
    [[nodiscard]] inline js_object_ptr<T>::operator double() const {
        static_assert(std::is_same_v<raw::number, T> || std::is_same_v<raw::js_object, T>,
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

    template<class T>
    [[nodiscard]] inline T *js_object_ptr<T>::operator->() const {
        return ptr;
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator==(const js_object_ptr<U> &other) const {
        return ptr->operator==(*other.ptr);
    }

    template<class T>
    template<class U>
    [[nodiscard]] inline bool js_object_ptr<T>::operator!=(const js_object_ptr<U> &other) const {
        return !this->operator==(other);
    }

    template<class T>
    [[nodiscard]] inline T *js_object_ptr<T>::get() const {
        return ptr;
    }

    template<class T>
    inline void js_object_ptr<T>::reset() {
        delete ptr;
        ptr = nullptr;
    }

    template<class T>
    inline js_object_ptr<T>::~js_object_ptr() {
        delete ptr;
    }

    // js_object_ptr definitions end ============================================

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
}

#endif //NAPI_TOOLS_VAR_TYPE_HPP
