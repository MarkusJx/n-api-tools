# n-api-tools
A toolbox containing C++ classes to be used with node-addon-api. 

## Usage
First, include the file in your project. CMake example:
```cmake
execute_process(COMMAND node -p "require('n-api-tools').include"
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
    return napi_tools::promises::Promise<void>::create(info.Env(), [] {
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
    return napi_tools::promises::Promise<std::string>::create(info.Env(), [] {
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
the arguments do not match. Supported type names are: ``STRING``, ``NUMBER``,
``FUNCTION``, ``OBJECT``, ``BOOLEAN``, ``ARRAY``.

Example:
```c++
void someFunction(const Napi::CallbackInfo &info) {
    using namespace napi_tools;
    // Expect a function, a string, a number and a boolean
    CHECK_ARGS(FUNCTION, STRING, NUMBER, BOOLEAN);
}
```