const native = require('./build/Release/napi_tools');

console.log("Native addon:", native);
console.log("TestString result:", native.testString("abc"));
