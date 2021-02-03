# n-api-tools
A toolbox containing C++ classes to be used with node-addon-api. 

## Installation
```sh
npm install @markusjx/n-api-tools
```

## Usage
First, include the file in your project. CMake example:
```cmake
execute_process(COMMAND node -p "require('@markusjx/n-api-tools').include"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE N_API_TOOLS_DIR)

# Include the directory
include_directories(${N_API_TOOLS_DIR})
```

This library is header-only, just include it:
```c++
#include <napi_tools.hpp>
```
### Promises
#### Void promises
```c++
Napi::Promise returnsPromise(const Napi::CallbackInfo &info) {
    // Return a promise
    return napi_tools::promises::promise<void>(info.Env(), [] {
        // Sleep to pretend we're actually doing work
        std::this_thread::sleep_for(std::chrono::seconds(2));
    });
}
```

#### Non-void promises
Non-void promises support most basic types as numbers, strings, vectors or booleans.
```c++
Napi::Promise returnsPromise(const Napi::CallbackInfo &info) {
    // Return a promise
    return napi_tools::promises::promise<std::string>(info.Env(), [] {
        // Sleep to pretend we're actually doing work
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Return a string value
        return "some string";
    });
}
```

### Callbacks
Callbacks can be used to call javascript function even without supplying a ``Napi::Env``.
The ``napi_tools::callbacks::callback`` takes function-like template arguments,
e.g. ``<void()>`` or ``<int(std::string, float)>``
#### Void callbacks
Create a callback with no arguments:
```c++
// Create a void callback
napi_tools::callbacks::callback<void()> callback = nullptr;

// Create a callback setter.
// This function should be exported and called by javascript.
// info[0] should be a Napi::Function.
void setCallback(const Napi::CallbackInfo &info) {
    callback = callbacks::callback<void()>(info);
}

// Call a callback function
void callCallback() {
    // Call the function async
    callback();
}
```

Create a callback with arguments:
```c++
// Create a void callback with arguments
napi_tools::callbacks::callback<void(std::string, int)> callback = nullptr;

// Create a callback setter.
// This function should be exported and called by javascript.
// info[0] should be a Napi::Function.
void setCallback(const Napi::CallbackInfo &info) {
    callback = callbacks::callback<void(std::string, int)>(info);
}

// Call a callback function
void callCallback() {
    // Call the function async and supply some arguments
    callback("some string", 42);
}
```

### Non-void callbacks
Non-void callbacks return a ``std::promise`` to get a ``std::future``
from to get the result of the callback. **NOTE:** If you call ``std::future<T>::wait()``
in the main thread, you will create a deadlock as the callback cannot be called since
the main thread is waiting for the callback to be called. To call a callback function
from node.js use a ``napi_tools::promises::Promise<T>`` or some other technique to not
block the main thread. You can also supply a callback function to be called with the
function return type.

Create a callback with no arguments:
```c++
// Create a callback returning a number
napi_tools::callbacks::callback<int()> callback = nullptr;

// Create a callback setter.
// This function should be exported and called by javascript.
// info[0] should be a Napi::Function.
void setCallback(const Napi::CallbackInfo &info) {
    callback = callbacks::callback<int()>(info);
}

// Call a callback function.
void callCallback() {
    // Call the function async. It returns a std::promise
    std::promise<int> promise = callback();

    // Get the future
    std::future<int> future = promise.get_future();

    // Wait for the future to be resolved and get the result
    future.wait();
    int res = future.get();
}
```

Create a callback with no arguments and without ``std::future``:
```c++
// Create a callback returning a number
napi_tools::callbacks::callback<int()> callback = nullptr;

// Create a callback setter.
// This function should be exported and called by javascript.
// info[0] should be a Napi::Function.
void setCallback(const Napi::CallbackInfo &info) {
    callback = callbacks::callback<int()>(info);
}

// Call a callback function.
void callCallback() {
    // Call the callback and supply a callback function
    callback([] (int res) {
        // The lambde will be called when the callback function finished.
        // The result of the callback will be stored in res.
    });
}
```

Create a callback with arguments:
```c++
// Create a callback returning a number
napi_tools::callbacks::callback<int(std::string, int)> callback = nullptr;

// Create a callback setter.
// This function should be exported and called by javascript.
// info[0] should be a Napi::Function.
void setCallback(const Napi::CallbackInfo &info) {
    callback = callbacks::callback<int(std::string, int)>(info);
}

// Call a callback function.
void callCallback() {
    // Call the function async. It returns a std::promise
    std::promise<int> promise = callback("some string", 42);

    // Get the future
    std::future<int> future = promise.get_future();

    // Wait for the future to be resolved and get the result
    future.wait();
    int res = future.get();
}
```

Create a callback with arguments and without ``std::future``:
```c++
// Create a callback returning a number
napi_tools::callbacks::callback<int(std::string, int)> callback = nullptr;

// Create a callback setter.
// This function should be exported and called by javascript.
// info[0] should be a Napi::Function.
void setCallback(const Napi::CallbackInfo &info) {
    callback = callbacks::callback<int(std::string, int)>(info);
}

// Call a callback function.
void callCallback() {
    // Call the function and supply a callback function
    callback("some string", 42, [] (int res) {
        // The result will be stored in res
    });
}
```

## Custom classes/structs as arguments/return types
In order to pass custom classes or structs to node.js or receive them, your class or struct
must implement the ``static Napi::Value toNapiValue(Napi::Env, T)`` function
to pass the class as an argument and the ``static T fromNapiValue(Napi::Value)``
function to get the class returned from the javascript process.

A custom implementation may look like this:
```c++
class custom_class {
public:
    // Some attributes
    std::string s1, s2;
    int i;
    std::vector<int> ints;

    // The toNapiValue function
    static Napi::Value toNapiValue(const Napi::Env &env, const custom_class &c) {
        Napi::Object obj = Napi::Object::New(env);

        // Convert the attributes using Napi::Value::From
        obj.Set("s1", Napi::Value::From(env, c.s1));
        obj.Set("s2", Napi::Value::From(env, c.s1));
        obj.Set("i",  Napi::Value::From(env, c.i));

        // You may also use the included conversion functions.
        // These can handle most basic types as strings, integers,
        // but also std::vector and std::map, plus classes with
        // the toNapiValue and/or fromNapiValue function(s)
        obj.Set("ints", napi_tools::util::conversions::cppValToValue(env, ints));
        return obj;
    }

    // The fromNapiValue function
    static custom_class fromNapiValue(const Napi::Value &val) {
        // Assuming val is an object, you may test if it actually is
        Napi::Object obj = val.ToObject();

        // Set all values
        custom_class c;
        c.s1 = obj.Get("s1").ToString();
        c.s2 = obj.Get("s2").ToString();
        c.i = obj.Get("i").ToNumber();

        // napi_tools also has functions to convert from napi values:
        c.ints = napi_tools::util::conversions::convertToCpp<std::vector<int>>(obj.Get("ints"));
        return c;
    }
};
```

## Other tools
### Catch all exceptions
To catch all exceptions possibly thrown by c++ to throw them in the javascript process,
use the ``TRY``/``CATCH_EXCEPTIONS`` macros:
```c++
void someFunction(const Napi::CallbackInfo &info) {
    TRY
        // Some code that may throw an exception
    CATCH_EXCEPTIONS
}
```

### Check argument types
To check if the number of supplied arguments and the argument types match,
use the ``CHECK_ARGS(...)`` macro. A ``Napi::Error`` will be thrown if
the arguments do not match. Supported type names are: ``string``, ``number``,
``function``, ``object``, ``boolean``, ``array``, ``undefined``, ``null``.

Example:
```c++
void someFunction(const Napi::CallbackInfo &info) {
    using namespace napi_tools;
    // Expect a function, a string, a number and a boolean
    CHECK_ARGS(function, string, number, boolean);
}
```

It is also possible to allow multiple types as an input:
```c++
void someFunction(const Napi::CallbackInfo &info) {
    using namespace napi_tools;
    // Expect a string, number, null or undefined
    CHECK_ARGS(string | number | null | undefined);
}
```

### Export functions
To make exporting functions in your ``ìnit`` method easier, n-api-tools provides the
``EXPORT_FUNCTION(exports, env, fn)`` macro. This macro will "export" functions using
their C++ function name to javascript.

Example:
```c++
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    EXPORT_FUNCTION(exports, env, func1);
    EXPORT_FUNCTION(exports, env, func2);
    EXPORT_FUNCTION(exports, env, anotherFunc);

    return exports;
}

// Initialize the module
NODE_API_MODULE(some_module, InitAll)
```
