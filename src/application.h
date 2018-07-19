#pragma once

#include <mbed.h>
#include <mbed_events.h>

#include "hw/display.h"
#include "hw/recorder.h"
#include "hw/volatile_buffer.h"
#include "ui/canvas.h"
#include "ui/waterfall.h"

namespace app {

class Application {
 private:
  EventQueue event_queue;

  app::debug::Debug &dbg;

  float32_t powers[480] = {0};

  void ProcessingJob();
  void RenderJob();

 public:
  app::hw::Display &display;
  app::ui::Canvas &canvas;
  app::hw::Recorder &recorder;

  app::ui::Waterfall &waterfall;

  Application(app::debug::Debug &dbg,
              app::hw::Display &display,
              app::ui::Canvas &canvas,
              app::hw::Recorder &recorder,
              app::ui::Waterfall &waterfall);
  int Init();
  void Run();

  void HandleAudioInHalfTransferComplete();

  void HandleAudioInTransferComplete();

  void HandleAudioInError();

  void HandleLtdcUnderrun();

  void HandleLtdcReload();

  void HandleLtdcIRQ();
};

}  // namespace app
