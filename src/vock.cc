#include "audio/binding.h"
#include "opus/binding.h"

#include "node.h"

namespace vock {

using namespace node;

static void Init(v8::Handle<v8::Object> target) {
  vock::audio::Audio::Init(target);
  vock::opus::Opus::Init(target);
}

NODE_MODULE(vock, Init);

} // namespace vock
