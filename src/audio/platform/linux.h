#ifndef _SRC_AUDIO_PLATFORM_LINUX_
#define _SRC_AUDIO_PLATFORM_LINUX_

#include "uv.h"
#include <alsa/asoundlib.h>

namespace vock {
namespace audio {

typedef void (*InputCallbackFn)(void*, size_t);
typedef void (*OutputCallbackFn)(void*, char*, size_t);

class PlatformUnit {
 public:
  enum Kind {
    kInputUnit,
    kOutputUnit
  };

  PlatformUnit(Kind kind, double rate);
  ~PlatformUnit();

  void Start();
  void Stop();

  void Render(char* out, size_t size);

  double GetInputSampleRate();

  void SetInputCallback(InputCallbackFn cb, void* arg);
  void SetOutputCallback(OutputCallbackFn cb, void* arg);

 private:
  static void InputCallback(snd_async_handler_t* handler);
  static void OutputCallback(snd_async_handler_t* handler);

  void RenderOutput(snd_pcm_sframes_t frames);

  snd_pcm_t* device_;
  snd_pcm_hw_params_t* params_;
  snd_async_handler_t* handler_;

  Kind kind_;
  double rate_;
  double input_rate_;
  unsigned int channels_;

  int16_t* buff_;
  ssize_t buff_size_;

  InputCallbackFn input_cb_;
  void* input_arg_;

  OutputCallbackFn output_cb_;
  void* output_arg_;
};

} // namespace audio
} // namespace vock

#endif // _SRC_AUDIO_PLATFORM_LINUX_
