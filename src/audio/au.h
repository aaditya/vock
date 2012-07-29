#ifndef _SRC_AUDIO_AU_H_
#define _SRC_AUDIO_AU_H_

#include "node.h"
#include "node_buffer.h"
#include "portaudio/pa_ringbuffer.h"

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <speex/speex_resampler.h>

namespace vock {
namespace audio {

class HALUnit {
 public:
  enum UnitKind {
    kInputUnit,
    kOutputUnit
  };

  HALUnit(Float64 rate,
          uv_async_t* in_cb,
          uv_async_t* inready_cb,
          uv_async_t* outready_cb);
  ~HALUnit();

  int Init();

  int Start();
  int Stop();

  size_t GetReadSize();
  node::Buffer* Read(size_t size);
  void Put(char* data, size_t size);

  const char* err;
  OSStatus err_st;

 protected:
  AudioUnit CreateUnit(UnitKind kind, Float64 rate);

  static OSStatus InputCallback(void* arg,
                                AudioUnitRenderActionFlags* flags,
                                const AudioTimeStamp* ts,
                                UInt32 bus,
                                UInt32 frame_count,
                                AudioBufferList* data);
  static OSStatus OutputCallback(void* arg,
                                 AudioUnitRenderActionFlags* flags,
                                 const AudioTimeStamp* ts,
                                 UInt32 bus,
                                 UInt32 frame_count,
                                 AudioBufferList* data);

  static const int kInputBus = 1;
  static const int kOutputBus = 0;

  AudioUnit in_unit_;
  AudioUnit out_unit_;
  Float64 rate_;
  Float64 input_rate_;

  SpeexResamplerState* resampler_;

  PaUtilRingBuffer in_ring_;
  PaUtilRingBuffer out_ring_;
  // NOTE: Should be a power of two
  int16_t in_ring_buf_[128 * 1024];
  int16_t out_ring_buf_[128 * 1024];

  char in_buff_[10 * 1024];
  AudioBufferList in_list_;

  uv_async_t* in_cb_;
  uv_async_t* inready_cb_;
  uv_async_t* outready_cb_;
  bool inready_;
  bool outready_;
};

} // namespace audio
} // namespace vock

#endif // _SRC_AUDIO_AU_H_
