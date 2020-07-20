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

#define NAPI_VERSION 4

#include <napi.h>
#include <regex>
#include <utility>
#include <map>
#include <type_traits>

#define call(name, object, ...) Get(name).As<Napi::Function>().Call(object, {__VA_ARGS__})
#define create(...) As<Napi::Function>().New({__VA_ARGS__})

#define TRY try {
#define CATCH_EXCEPTIONS } catch (const std::exception &e) {throw Napi::Error::New(info.Env(), e.what());} catch (...) {throw Napi::Error::New(info.Env(), "An unknown error occurred");}

#define CHECK_ARGS(...) ::util::checkArgs(info, ::util::removeNamespace(__FUNCTION__), {__VA_ARGS__})
#define CHECK_LENGTH(len) if (info.Length() != len) throw Napi::TypeError::New(info.Env(), ::util::removeNamespace(__FUNCTION__) + " requires " + std::to_string(len) + " arguments")

namespace napi_tools {
    enum type {
        STRING,
        NUMBER,
        FUNCTION,
        OBJECT,
        BOOLEAN,
        ARRAY
    };

    class exception : public std::exception {
    public:
#ifdef __APPLE__
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

        /*
         * The js object base class
         */
        class js_object {
        public:
            /**
             * Get the value as a n-api value
             *
             * @param env the target environment
             * @return the n-api value
             */
            [[nodiscard]] virtual Napi::Value getValue(const Napi::Env &env) const {
                return env.Undefined();
            }

            /**
             * Check if two objects are equal
             *
             * @return false
             */
            [[nodiscard]] virtual bool operator==(const js_object_ptr<js_object> &) const {
                return false;
            }

            /**
             * Get the type of this object
             *
             * @return the type of this obect
             */
            [[nodiscard]] virtual js_type getType() const {
                return js_type::none;
            }

            [[maybe_unused]] virtual ~js_object() = default;
        };

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

        class function;
    }

/**
 * A pointer to a js object
 *
 * @tparam T the object type
 */
    template<class T>
    class js_object_ptr {
    public:
        static_assert(std::is_base_of_v<raw::js_object, T> || std::is_same_v<raw::js_object, T>);

        /**
         * The default constructor
         */
        inline js_object_ptr() {
            ptr = nullptr;
        }

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

        // Specialized constructors. Only available when T = raw::js_object

        /**
         * Create a js_object_ptr<raw::js_object> from a char array.
         * Will create a js_object_ptr<raw::string> and cast it to raw::js_object
         *
         * @param c the char array containing data
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        inline js_object_ptr(const char *c) {
            ptr = (T *) new raw::string(c);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a string.
         * Will create a js_object_ptr<raw::string> and cast it to raw::js_object
         *
         * @param s the string containing the data
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        inline js_object_ptr(const std::string &s) {
            ptr = (T *) new raw::string(s);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a bool value.
         * Will create a js_object_ptr<raw::boolean> and cast it to raw::js_object
         *
         * @param b the bool value
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        inline js_object_ptr(bool b) {
            ptr = (T *) new raw::boolean(b);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a integer value.
         * Will create a js_object_ptr<raw::number> and cast it to raw::js_object
         *
         * @param i the integer value
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        inline js_object_ptr(int i) {
            ptr = (T *) new raw::number(i);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a double value.
         * Will create a js_object_ptr<raw::number> and cast it to raw::js_object
         *
         * @param d the double value
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        inline js_object_ptr(double d) {
            ptr = (T *) new raw::number(d);
        }

        /**
         * Create a js_object_ptr<raw::js_object> from a vector<js_object_ptr<raw::js_object>> value.
         * Will create a js_object_ptr<raw::array> and cast it to raw::js_object
         *
         * @param data the data array
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        inline js_object_ptr(const std::vector<js_object_ptr<raw::js_object>> data) {
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
                    ptr = new T(*other.get());
                }
            }

            return *this;
        }

        template<class U>
        inline js_object_ptr<T> &operator=(const js_object_ptr<U> &other) {
            delete ptr;
            if (other.get() == nullptr) {
                ptr = nullptr;
            } else {
                ptr = new T(*other.as<T>());
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
        inline auto operator+(const js_object_ptr<T> &other) {
            return *ptr + *other.ptr;
        }

        /**
         * operator +
         *
         * @tparam U the type of the value to add
         * @param val the value to add
         * @return the result of the operation
         */
        template<class U>
        inline auto operator+(U val) {
            return *ptr + val;
        }

        /**
         * operator +
         *
         * @param other the object to substract
         * @return the result of the operation
         */
        inline auto operator-(const js_object_ptr<T> &other) {
            return *ptr - *other.ptr;
        }

        /**
         * operator -
         *
         * @tparam U the type of the value to substract
         * @param val the value to substract
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
            return js_object_ptr<U>((U *) ptr);
        }

        /**
         * Convert this to js_object_ptr<raw::js_object>
         *
         * @return the resulting pointer
         */
        [[nodiscard]] inline js_object_ptr<raw::js_object> toObject() const {
            return to<raw::js_object>();
        }

        // Convert to various types. Only available when T = raw::js_object

        /**
         * Convert this to js_object_ptr<raw::string>
         *
         * @return the resulting pointer
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        [[nodiscard]] inline js_object_ptr<raw::string> toString() const {
            return to<raw::string>();
        }

        /**
         * Convert this to js_object_ptr<raw::number>
         *
         * @return the resulting pointer
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        [[nodiscard]] inline js_object_ptr<raw::number> toNumber() const {
            return to<raw::number>();
        }

        /**
         * Convert this to js_object_ptr<raw::boolean>
         *
         * @return the resulting pointer
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        [[nodiscard]] inline js_object_ptr<raw::boolean> toBoolean() const {
            return to<raw::boolean>();
        }

        /**
         * Convert this to js_object_ptr<raw::array>
         *
         * @return the resulting pointer
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        [[nodiscard]] inline js_object_ptr<raw::array> toArray() const {
            return to<raw::array>();
        }

        /**
         * Convert this to js_object_ptr<raw::function>
         *
         * @return the resulting pointer
         */
        template<class = std::enable_if_t<std::is_same_v<raw::js_object, T>>>
        [[nodiscard]] inline js_object_ptr<raw::function> toFunction() const {
            return to<raw::function>();
        }

        // Overloaded operators for std::string, bool and double
        // Only available for the specific classes storing these

        /**
         * operator std::string
         *
         * @return the string value
         */
        template<class = std::enable_if_t<std::is_same_v<raw::string, T>>>
        [[nodiscard]] inline operator std::string() const {
            return *ptr;
        }

        /**
         * operator bool
         *
         * @return the bool value
         */
        template<class = std::enable_if_t<std::is_same_v<raw::boolean, T>>>
        [[nodiscard]] inline operator bool() const {
            return ptr->operator bool();
        }

        /**
         * operator double
         *
         * @return the double value
         */
        template<class = std::enable_if_t<std::is_same_v<raw::number, T>>>
        [[nodiscard]] inline operator double() const {
            return ptr->operator double();
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

    inline js_object getObject(const Napi::Value &val);

    namespace raw {
        /**
         * Undefined property
         */
        class undefined : public js_object {
        public:
            /**
             * Undefined constructor
             */
            undefined() = default;

            /**
             * Get the undefined value as javascript value
             *
             * @param env the environment to work in
             * @return the value
             */
            [[maybe_unused]] [[nodiscard]] Napi::Value getValue(const Napi::Env &env) const override {
                return env.Undefined();
            }

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if both are equal
             */
            [[nodiscard]] bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType();
            }

            /**
             * Get the type of this object
             *
             * @return the type of this object
             */
            [[nodiscard]] js_type getType() const override {
                return js_type::undefined;
            }

            /**
             * The destructor
             */
            ~undefined() override = default;
        };

        /**
         * null property
         */
        class null : public js_object {
        public:
            /**
             * The constructor
             */
            null() = default;

            /**
             * Get the null value as javascript value
             *
             * @param env the environment to work in
             * @return the value
             */
            [[maybe_unused]] [[nodiscard]] Napi::Value getValue(const Napi::Env &env) const override {
                return env.Null();
            }

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the other object is also null
             */
            [[nodiscard]] bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType();
            }

            /**
             * Get the type of this object
             *
             * @return the type
             */
            [[nodiscard]] js_type getType() const override {
                return js_type::null;
            }

            /**
             * The destructor
             */
            ~null() override = default;
        };

        /**
         * boolean type
         */
        class boolean : public js_object {
        public:
            /**
             * Construct boolean from a n-api value
             *
             * @param v the value to get the boolean from
             */
            [[maybe_unused]] boolean(const Napi::Value &v) : value(v.ToBoolean().Value()) {}

            /**
             * Construct a boolean from a n-api boolean
             *
             * @param b the boolean value to construct from
             */
            [[maybe_unused]] boolean(const Napi::Boolean &b) : value(b.Value()) {}

            /**
             * Construct a boolean from a bool value
             *
             * @param val the bool value
             */
            [[maybe_unused]] boolean(bool val) : value(val) {}

            /**
             * Get the n-api boolean value
             *
             * @param env the environment to worn in
             * @return the n-api boolean value
             */
            [[maybe_unused]] [[nodiscard]] Napi::Value getValue(const Napi::Env &env) const override {
                return Napi::Boolean::New(env, value);
            }

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the objects are equal
             */
            [[nodiscard]] bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType() && value == obj.as<boolean>()->operator bool();
            }

            /**
             * Get the type of the object
             *
             * @return the type
             */
            [[nodiscard]] js_type getType() const override {
                return js_type::boolean;
            }

            /**
             * Set the value
             *
             * @param val the new value
             * @return this
             */
            boolean &operator=(bool val) {
                value = val;
            }

            /**
             * Operator bool
             *
             * @return the bool value
             */
            operator bool() const {
                return value;
            }

            /**
             * Get the value
             *
             * @return the value
             */
            [[nodiscard]] bool getValue() const {
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
            /**
             * Create a number from a n-api number
             *
             * @param number the number
             */
            [[maybe_unused]] number(const Napi::Number &number) {
                value = number.DoubleValue();
            }

            /**
             * Create a number from a n-api value
             *
             * @param val the value
             */
            [[maybe_unused]] number(const Napi::Value &val) {
                value = val.ToNumber().DoubleValue();
            }

            /**
             * Create a number from a double value
             *
             * @param val the value
             */
            [[maybe_unused]] number(double val) {
                value = val;
            }

            /**
             * Create a number from a integer value
             *
             * @param val the value
             */
            [[maybe_unused]] number(int val) {
                value = val;
            }

            /**
             * Get the n-api value of this number
             *
             * @param env the environment to work in
             * @return the n-api value
             */
            [[maybe_unused]] [[nodiscard]] Napi::Value getValue(const Napi::Env &env) const override {
                return Napi::Number::New(env, value);
            }

            /**
             * Operator equal
             *
             * @param obj the object to compare with
             * @return true if the object is equal
             */
            [[nodiscard]] bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType() && value == obj.as<number>()->operator double();
            }

            /**
             * Get the type of this object
             *
             * @return the type
             */
            [[nodiscard]] js_type getType() const override {
                return js_type::number;
            }

            /**
             * Double operator
             *
             * @return the double value
             */
            operator double() const {
                return value;
            }

            /**
             * Get the double value
             *
             * @return the value
             */
            [[nodiscard]] [[maybe_unused]] double getValue() const {
                return value;
            }

            /**
             * Get the integer value
             *
             * @return the value
             */
            [[nodiscard]] [[maybe_unused]] int intValue() const {
                return (int) value;
            }

            /**
             * Set the value
             *
             * @param val the new value
             * @return this
             */
            number &operator=(double val) {
                value = val;

                return *this;
            }

            /**
             * Set the value
             *
             * @param val the new value
             * @return this
             */
            number &operator=(int val) {
                value = val;

                return *this;
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
            string() = default;

            /**
             * Create a string from a n-api string
             *
             * @param str the n-api string value
             */
            [[maybe_unused]] string(const Napi::String &str) : std::string(str.Utf8Value()) {}

            /**
             * Create a string from a n-api value
             *
             * @param value the value
             */
            [[maybe_unused]] string(const Napi::Value &value) : std::string(value.As<Napi::String>().Utf8Value()) {}

            /**
             * Replace a value with another value
             *
             * @param toReplace the value to replace
             * @param replacement the replacement value
             * @return this
             */
            [[maybe_unused]] string replace(const string &toReplace, const string &replacement) {
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
            [[maybe_unused]] string concat(Args...args) {
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
            [[maybe_unused]] [[nodiscard]] bool endsWith(const string &s) const {
                return this->ends_with(s);
            }

            /**
             * Check if this includes a substring
             *
             * @param s the substring to check for
             * @return true if the substring exists
             */
            [[maybe_unused]] [[nodiscard]] bool includes(const string &s) const {
                return this->find(s) != std::string::npos;
            }

            /**
             * Get the index of a substring
             *
             * @param s the string to search for
             * @return the index of the substring or -1 if not found
             */
            [[maybe_unused]] [[nodiscard]] int indexOf(const string &s) const {
                return (int) this->find_first_of(s);
            }

            /**
             * Get the index of a substring
             *
             * @param s the string to search for
             * @param fromIndex the starting index
             * @return the index of the substring or -1 if not found
             */
            [[maybe_unused]] [[nodiscard]] int indexOf(const string &s, int fromIndex) const {
                return (int) this->find_first_of(s, fromIndex + s.length());
            }

            /**
             * Get the last index of a substring
             *
             * @param s the string to search for
             * @param fromIndex the last index to search for
             * @return the index of the substring or -1 if not found
             */
            [[maybe_unused]] [[nodiscard]] int
            lastIndexOf(const string &s, size_t fromIndex = std::string::npos) const {
                return (int) this->find_last_of(s, fromIndex);
            }

            /**
             * Get the char at a position
             *
             * @param pos the position of the char to get
             * @return the char as a string
             */
            [[nodiscard]] string charAt(int pos) const {
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
            [[nodiscard]] string operator[](int pos) const {
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
            [[maybe_unused]] std::vector<string> match(const string &regex) {
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

            /**
             * Get this as a n-api string
             *
             * @param env the environment to work in
             * @return the n-api string
             */
            [[maybe_unused]] [[nodiscard]] Napi::String toNapiString(const Napi::Env &env) const {
                return Napi::String::New(env, *this);
            }

            /**
             * Get this as a n-api value
             *
             * @param env the environment to work in
             * @return the n-api value
             */
            [[maybe_unused]] [[nodiscard]] Napi::Value getValue(const Napi::Env &env) const override {
                return Napi::String::New(env, *this);
            }

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the two objects are equal
             */
            [[nodiscard]] bool operator==(const ::napi_tools::js_object &obj) const override {
                return obj->getType() == this->getType() && strcmp(this->c_str(), obj.as<string>()->c_str()) == 0;
            }

            /**
             * Get the type of this object
             *
             * @return the type
             */
            [[nodiscard]] js_type getType() const override {
                return js_type::string;
            }

            /**
             * Create a string from characters
             *
             * @tparam Args the argument types
             * @param args the characters
             * @return the string
             */
            template<class...Args>
            static string fromCharCode(Args...args) {
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
            explicit objLValue(std::function<void(::napi_tools::js_object)> setter, const ::napi_tools::js_object &ptr)
                    : setter(std::move(setter)), ptr(ptr) {}

            /**
             * The object setter
             *
             * @param val the value
             * @return this
             */
            objLValue &operator=(const ::napi_tools::js_object &val) {
                setter(val);
                ptr = val;
            }

            /**
             * operator ->
             *
             * @return the object
             */
            [[nodiscard]] ::napi_tools::js_object operator->() const {
                return ptr;
            }

            /**
             * Get the pointer
             *
             * @return the pointer
             */
            [[nodiscard]] ::napi_tools::js_object getValue() const {
                return ptr;
            }

            /**
             * Get the pointer
             *
             * @return the pointer
             */
            [[nodiscard]] operator ::napi_tools::js_object() const {
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
            /**
             * Create an array from an n-api array
             *
             * @param array the n-api array
             */
            array(const Napi::Array &array) : values() {
                for (uint32_t i = 0; i < array.Length(); i++) {
                    values.push_back(getObject(array[i]));
                }
            }

            /**
             * Create an array from a n-api value
             *
             * @param value the value
             */
            array(const Napi::Value &value) : values() {
                auto array = value.As<Napi::Array>();
                for (uint32_t i = 0; i < array.Length(); i++) {
                    values.push_back(getObject(array[i]));
                }
            }

            /**
             * Create an array from a vector of js_objects
             *
             * @param val the vector of js_objects
             */
            array(const std::vector<::napi_tools::js_object> &val) : values(val) {}

            /**
             * Get the value at an index
             *
             * @param index the index
             * @return the obj l-value
             */
            objLValue operator[](int index) {
                return objLValue([this, index](const ::napi_tools::js_object &newPtr) {
                    values.insert(values.begin() + index, newPtr);
                }, values.at(index));
            }

            /**
             * Get the length of the array
             *
             * @return the length
             */
            [[nodiscard]] size_t length() const {
                return values.size();
            }

            /**
             * Get the underlying vector
             *
             * @return the vector
             */
            [[nodiscard]] std::vector<::napi_tools::js_object> getValues() const {
                return values;
            }

            /**
             * Get this object as a n-api value
             *
             * @param env the environment to work in
             * @return the value
             */
            [[maybe_unused]] [[nodiscard]] Napi::Value getValue(const Napi::Env &env) const override {
                Napi::Array array = Napi::Array::New(env, values.size());
                for (size_t i = 0; i < values.size(); i++) {
                    array.Set((uint32_t) i, values[i]->getValue(env));
                }
                return array;
            }

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the two objects are equal
             */
            [[nodiscard]] bool operator==(const ::napi_tools::js_object &obj) const override {
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
            [[nodiscard]] js_type getType() const override {
                return js_type::array;
            }

            /**
             * The destructor
             */
            ~array() override = default;

        private:
            std::vector<::napi_tools::js_object> values;
        };

        /**
         * object type
         */
        class object : public js_object {
        public:
            /**
             * Create an object from an n-api object
             *
             * @param object the object
             */
            object(const Napi::Object &object) : contents() {
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
            object(const Napi::Value &value) : contents() {
                auto object = value.As<Napi::Object>();
                Napi::Array names = object.GetPropertyNames();
                for (uint32_t i = 0; i < names.Length(); i++) {
                    ::napi_tools::js_object obj = getObject(object[i]);
                    contents.insert(std::make_pair(names[i].operator Napi::Value().As<Napi::String>(), obj));
                    values.push_back(obj);
                }
            }

            /**
             * Get the length of the object
             *
             * @return the length
             */
            [[nodiscard]] size_t length() const {
                return values.size();
            }

            /**
             * Get a value by a key
             *
             * @param key the key
             * @return the object l-value
             */
            objLValue operator[](const std::string &key) {
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
            objLValue operator[](int index) {
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

            /**
             * Get this object as an n-api object
             *
             * @param env the environment to work with
             * @return the value
             */
            [[maybe_unused]] [[nodiscard]] Napi::Value getValue(const Napi::Env &env) const override {
                Napi::Object object = Napi::Object::New(env);
                for (const auto &p : contents) {
                    object.Set(p.first, p.second->getValue(env));
                }
                return object;
            }

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the two objects are equal
             */
            [[nodiscard]] bool operator==(const ::napi_tools::js_object &obj) const override {
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
            [[nodiscard]] js_type getType() const override {
                return js_type::object;
            }

            /**
             * The destructor
             */
            ~object() override = default;

        private:
            std::map<std::string, ::napi_tools::js_object> contents;
            std::vector<::napi_tools::js_object> values;
        };

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
            function(const Napi::Function &func) : fn(func) {}

            /**
             * Create a function from a n-api value
             *
             * @param value the value
             */
            function(const Napi::Value &value) : fn(value.As<Napi::Function>()) {}

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
            [[maybe_unused]] [[nodiscard]] Napi::Value getValue(const Napi::Env &) const override {
                return fn;
            }

            /**
             * Operator equals
             *
             * @param obj the object to compare with
             * @return true if the two objects are equal
             */
            [[nodiscard]] bool operator==(const ::napi_tools::js_object &obj) const override {
                return this->getType() == obj->getType() && this->fn == obj.as<function>()->fn;
            }

            /**
             * Get the type of the object
             *
             * @return the type
             */
            [[nodiscard]] js_type getType() const override {
                return js_type::function;
            }

        private:
            const Napi::Function fn;
        };
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
        ThreadSafeFunction(const Napi::ThreadSafeFunction &fn) : ts_fn(fn) {}

        // This API may be called from any thread.
        void blockingCall() const {
            napi_status status = ts_fn.BlockingCall();

            if (status != napi_ok) {
                Napi::Error::Fatal("ThreadEntry", "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
            }
        }

        // This API may be called from any thread.
        template<typename Callback>
        void blockingCall(Callback callback) const {
            napi_status status = ts_fn.BlockingCall(callback);

            if (status != napi_ok) {
                Napi::Error::Fatal("ThreadEntry", "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
            }
        }

        // This API may be called from any thread.
        template<typename DataType, typename Callback>
        void blockingCall(DataType *data, Callback callback) const {
            napi_status status = ts_fn.BlockingCall(data, callback);

            if (status != napi_ok) {
                Napi::Error::Fatal("ThreadEntry", "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
            }
        }

    private:
        const Napi::ThreadSafeFunction &ts_fn;
    };

    using thread_entry = std::function<js_object(const ThreadSafeFunction &)>;

    class Promise {
    public:
        static Napi::Promise New(const Napi::Env &env, const thread_entry &function) {
            Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);
            new Promise(env, deferred, function);

            return deferred.Promise();
        }

    private:
        Promise(const Napi::Env &env, const Napi::Promise::Deferred &deferred, const thread_entry &function) : deferred(
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

        void threadEntry() {
            ThreadSafeFunction threadSafeFunction(ts_fn);
            result = entry(threadSafeFunction);

            // Release the thread-safe function. This decrements the internal thread
            // count, and will perform finalization since the count will reach 0.
            ts_fn.Release();
        }

        void FinalizerCallback(Napi::Env env, void *) {
            // Join the thread
            nativeThread.join();

            // Resolve the Promise previously returned to JS via the CreateTSFN method.
            deferred.Resolve(result->getValue(env));

            //delete this;
        }

        ~Promise() = default;

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
        std::string removeNamespace(const std::string &str) {
            return str.substr(str.rfind(':') + 1);
        }

        void checkArgs(const Napi::CallbackInfo &info, const std::string &funcName, const std::vector<type> &types) {
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
}

#endif //NAPI_TOOLS_NAPI_TOOLS_HPP
