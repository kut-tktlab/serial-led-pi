/**
 * A wrapper code for creating a Node.js native addon.
 * Most of this code is taken from the official document of Node.js:
 * https://nodejs.org/api/addons.html
 *
 * Build:
 * $ node-gyp configure build
 */

#include <node.h>
extern "C" {
  #include "serialled.h"
}

namespace serialled {

using v8::Exception;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;

const int SUCCESS = 0;
const int FAILURE = -1;

inline int convertArgs(const FunctionCallbackInfo<Value>& args,
                       int argv[], const int n)
{
  Isolate* isolate = args.GetIsolate();

  if (args.Length() < n) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Wrong number of arguments")));
    return FAILURE;
  }

  // Convert the arguments
  for (int i = 0; i < n; i++) {
    if (!args[i]->IsNumber()) {
      isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Wrong arguments")));
      return FAILURE;
    }
    argv[i] = args[i]->NumberValue();
  }

  return SUCCESS;
}

void SetColor(const FunctionCallbackInfo<Value>& args) {
  int v[4];
  if (convertArgs(args, v, 4) == FAILURE) { return; }

  ledSetColor(v[0], v[1], v[2], v[3]);
}

void Setup(const FunctionCallbackInfo<Value>& args) {
  int v[2];
  if (convertArgs(args, v, 2) == FAILURE) { return; }

  int result = ledSetup(v[0], v[1]);

  Local<Number> ret = Number::New(args.GetIsolate(), result);
  args.GetReturnValue().Set(ret);
}

void Cleanup(const FunctionCallbackInfo<Value>& args) {
  ledCleanup();
}

void Send(const FunctionCallbackInfo<Value>& args) {
  ledSend();
}

void Init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "setColor", SetColor);
  NODE_SET_METHOD(exports, "setup",    Setup);
  NODE_SET_METHOD(exports, "cleanup",  Cleanup);
  NODE_SET_METHOD(exports, "send",     Send);
}

NODE_MODULE(addon, Init)

}  // namespace serialled
