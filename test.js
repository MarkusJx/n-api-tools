const native = require('./build/Release/napi_tools.node');

console.log("Native addon:", native);
native.promiseTest().then((res) => {
    console.log(res);
});

native.setCallback((a, b) => {
    console.log(`Callback values: ${a}, ${b}`);
    //native.stopCallback();
});

native.setIntCallback((a) => {
    console.log("Value to return: " + a);
    return 10;
});

native.setVecCallback((v) => {
   console.log(`Array: ${JSON.stringify(v)}`);
   return 123;
});

native.setCustomCallback((a) => {
   console.log("Custom struct: " + JSON.stringify(a));
   return a;
});

native.callMeMaybe();

setTimeout(() => {
    process.exit();
    //native.stopCallback();
}, 5000);
