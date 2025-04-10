#include "loop.hpp"
#include "src/audio.hpp"
#include <cstdint>

// Start recording a new loop
void LoopTrack::startRecording() {
  // Clear the events and start recording
  event_count_ = 0;
  // Set the recording flag
  recording_ = true;
  // Clear the playing flag
  playing_ = false;
  // Reset the loop length
  record_start_sample_ = 0;

  last_position_in_loop_ = 0;
}

void LoopTrack::stopRecording(uint32_t time_when_stopped) {
  // Stop recording and start playing
  recording_ = false;
  // Set the playing flag
  playing_ = true;
  // Calculate the loop length
  if (record_start_sample_ != 0) {
    loop_length_ = time_when_stopped - record_start_sample_;
  } else { // no events
    loop_length_ = 0;
  }

  if (event_count_ == 0) {
    playing_ = false;
  }

  last_position_in_loop_ = 0;
}

void LoopTrack::addEvent(uint8_t drum_id, uint16_t normalized_velocity,
                         uint32_t current_absolute_sample_time) {

  if (!recording_)
    return;
  if (record_start_sample_ == 0 && event_count_ == 0) {
    record_start_sample_ = current_absolute_sample_time;
  }

  // Add the event if space available
  if (event_count_ < events_.size()) {
    uint32_t relative_timestamp =
        current_absolute_sample_time - record_start_sample_;

    events_[event_count_++] = {relative_timestamp, drum_id,
                               normalized_velocity};
  } else {
    // stop recording automatically if full?
    stopRecording(current_absolute_sample_time);
  }
}

void LoopTrack::tick(uint32_t current_sample_time) {
  if (!playing_ || loop_length_ == 0 || event_count_ == 0)
    return;

  // this will defo break after the 32 bit timer wraps around
  uint32_t elapsed_time = current_sample_time - record_start_sample_;

  uint32_t current_position_in_loop = elapsed_time % loop_length_;

  for (size_t i = 0; i < event_count_; i++) {
    const auto &e =
        events_[i]; // remember that e.timestamp is RELATIVE, not absolute
    bool event_should_trigger = false;

    if (last_position_in_loop_ < current_position_in_loop) {
      if (e.timestamp > last_position_in_loop_ &&
          e.timestamp <= current_position_in_loop) {
        event_should_trigger = true;
      }
    }
    // wrap condition
    // ie. last = 400, current = 10, length = 401
    else if (current_position_in_loop < last_position_in_loop_) {
      if (e.timestamp > last_position_in_loop_ ||
          e.timestamp <= current_position_in_loop) {
        event_should_trigger = true;
      }
    }
    // handles first tick
    else if (last_position_in_loop_ == current_position_in_loop) {
      if (e.timestamp == current_position_in_loop) {
        event_should_trigger = true;
      }
    }
    // If the event should trigger, play it
    if (event_should_trigger) {
      audioEngine.triggerVoiceFromLoop(e.drum_id, e.velocity);
    }
  }
  last_position_in_loop_ = current_position_in_loop;
}

void LoopTrack::clear() {
  // Clear the loop
  event_count_ = 0;
  playing_ = false;
  recording_ = false;
  loop_length_ = 0;
  record_start_sample_ = 0;
  last_position_in_loop_ = 0;
}

// Getters
bool LoopTrack::isRecording() const { return recording_; }
bool LoopTrack::isPlaying() const { return playing_; }
uint32_t LoopTrack::getLength() const { return loop_length_; }
