const native = require('./build/Release/napi_tools.node');

console.log("Native addon:", native);
native.promiseTest().then((res) => {
    console.log(res);
});

native.setCallback((a, b) => {
    console.log(`Callback values: ${a}, ${b}`);
    //native.stopCallback();
});

native.callMeMaybe();

setTimeout(() => {
    process.exit();
    //native.stopCallback();
}, 5000);
