#ifndef _SRC_AUDIO_CORE_H_
#define _SRC_AUDIO_CORE_H_

#include "au.h"

#include "node.h"
#include "node_object_wrap.h"
#include "speex/speex_echo.h"

namespace vock {
namespace audio {

using namespace node;

class Audio : public ObjectWrap {
 public:
  Audio(Float64 rate, size_t frame_size);
  ~Audio();

  static void Init(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> New(const v8::Arguments& arg);
  static v8::Handle<v8::Value> Start(const v8::Arguments& arg);
  static v8::Handle<v8::Value> Stop(const v8::Arguments& arg);
  static v8::Handle<v8::Value> Enqueue(const v8::Arguments& arg);
  static v8::Handle<v8::Value> CancelEcho(const v8::Arguments& arg);
  static v8::Handle<v8::Value> GetRms(const v8::Arguments& arg);
  static v8::Handle<v8::Value> ApplyGain(const v8::Arguments& arg);

  static void InputAsyncCallback(uv_async_t* async, int status);
  static void InputReadyCallback(uv_async_t* async, int status);
  static void OutputReadyCallback(uv_async_t* async, int status);

 protected:
  HALUnit* unit_;
  SpeexEchoState* echo_state_;
  size_t frame_size_;

  uv_async_t in_async_;
  uv_async_t inready_async_;
  uv_async_t outready_async_;
};

} // namespace audio
} // namespace vock

#endif // _SRC_AUDIO_CORE_H_
