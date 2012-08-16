#include "linux.h"
#include "uv.h"

#include <stdint.h>
#include <string.h> // memmove
#include <alsa/asoundlib.h>
#include <pthread.h>

#define CHECK(fn, msg)\
{\
  int _err_c = fn;\
  if (_err_c < 0) {\
    fprintf(stderr, "%s (%d)\n", msg, _err_c);\
    abort();\
  }\
}

#define RECOVER(device, fn, res)\
{\
  int _err_r;\
  do {\
    _err_r = fn;\
    if (_err_r < 0) {\
      CHECK(snd_pcm_recover(device, _err_r, 1), "Recover failed")\
    }\
  } while (_err_r < 0);\
  res = _err_r;\
}

namespace vock {
namespace audio {

PlatformUnit::PlatformUnit(Kind kind, double rate) : kind_(kind), rate_(rate) {
  // Init thread
  uv_sem_init(&sem_, 0);
  pthread_create(&loop_, NULL, PlatformUnit::Loop, this);

  // Init device
  snd_pcm_stream_t dir;

  if (kind == kInputUnit) {
    dir = SND_PCM_STREAM_CAPTURE;
  } else {
    dir = SND_PCM_STREAM_PLAYBACK;
  }

  CHECK(snd_pcm_open(&device_, "hw:0,0", dir, 0), "Failed to open device")
  CHECK(snd_pcm_hw_params_malloc(&params_), "Failed to allocate parameters")
  CHECK(snd_pcm_hw_params_any(device_, params_), "Failed to bind parameters")
  CHECK(snd_pcm_hw_params_set_access(device_,
                                     params_,
                                     SND_PCM_ACCESS_RW_INTERLEAVED),
        "Failed to set access")
  CHECK(snd_pcm_hw_params_set_format(device_, params_, SND_PCM_FORMAT_S16_LE),
        "Failed to set format")
  CHECK(snd_pcm_hw_params_set_rate(device_, params_, rate, 0),
        "Failed to set rate")
  CHECK(snd_pcm_hw_params_get_channels(params_, &channels_),
        "Set channels failed")

  input_rate_ = rate;

  // Init buffer
  buff_size_ = rate / 100;
  buff_ = new int16_t[channels_ * buff_size_];

  // Apply other params
  CHECK(snd_pcm_hw_params_set_buffer_size(device_, params_, buff_size_ * 4),
        "Failed to set buffer size")
  CHECK(snd_pcm_hw_params_set_period_size(device_, params_, buff_size_, 0),
        "Failed to set period size")
  CHECK(snd_pcm_hw_params(device_, params_),
        "Failed to apply params")
  snd_pcm_hw_params_free(params_);
  CHECK(snd_pcm_prepare(device_), "Failed to prepare device");
}


PlatformUnit::~PlatformUnit() {
  pthread_cancel(loop_);
  uv_sem_destroy(&sem_);
  snd_pcm_drain(device_);
  snd_pcm_close(device_);
  delete[] buff_;
}


void* PlatformUnit::Loop(void* arg) {
  PlatformUnit* unit = reinterpret_cast<PlatformUnit*>(arg);
  int avail;

  while (1) {
    // Wait for start
    uv_sem_wait(&unit->sem_);

    if (unit->kind_ == kInputUnit) {
      while (1) {
        // Break on stop
        if (!unit->active_) break;
        RECOVER(unit->device_, snd_pcm_avail_update(unit->device_), avail)
        if (avail == 0) snd_pcm_wait(unit->device_, 2);

        unit->input_cb_(unit->input_arg_, unit->buff_size_ * 2);
      }
    } else if (unit->kind_ == kOutputUnit) {
      int written;
      int16_t* buff;

      while (1) {
        // Break on stop
        if (!unit->active_) break;
        RECOVER(unit->device_, snd_pcm_avail_update(unit->device_), avail)
        if (avail == 0) snd_pcm_wait(unit->device_, 2);

        buff = unit->buff_;
        unit->output_cb_(unit->output_arg_,
                         reinterpret_cast<char*>(buff),
                         unit->buff_size_ * 2);

        // Non-interleaved mono -> Interleaved multi-channel
        int i = unit->buff_size_ - 1;
        for (; i >= 0; i--) {
          for (size_t j = 0; j < unit->channels_; j++) {
            buff[i * unit->channels_ + j] = buff[i];
          }
        }

        RECOVER(unit->device_,
                snd_pcm_writei(unit->device_, buff, unit->buff_size_),
                written);
        assert(written == unit->buff_size_);
      }
    }
  }

  return NULL;
}


void PlatformUnit::Start() {
  if (active_) return;
  snd_pcm_pause(device_, 0);
  active_ = true;
  uv_sem_post(&sem_);
}


void PlatformUnit::Stop() {
  if (!active_) return;
  active_ = false;
  snd_pcm_pause(device_, 1);
}


void PlatformUnit::Render(char* out, size_t size) {
  size_t read;
  size_t i, j;
  int16_t* buff = buff_;
  int16_t* iout = reinterpret_cast<int16_t*>(out);

  // Read to internal buffer first
  RECOVER(device_, snd_pcm_readi(device_, buff, size / 2), read);
  assert(read == size / 2);

  // Get channel from interleaved stream
  for (i = 0, j = 0; i < size * channels_ / 2; i += channels_, j++) {
    iout[j] = buff[i];
  }
}


double PlatformUnit::GetInputSampleRate() {
  return input_rate_;
}


void PlatformUnit::SetInputCallback(InputCallbackFn cb, void* arg) {
  input_cb_ = cb;
  input_arg_ = arg;
}


void PlatformUnit::SetOutputCallback(OutputCallbackFn cb, void* arg) {
  output_cb_ = cb;
  output_arg_ = arg;
}

} // namespace audio
} // namespace vock

