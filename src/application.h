#pragma once

#include "hw/display.h"
#include "hw/recorder.h"
#include "hw/volatile_buffer.h"
#include "ui/canvas.h"
#include "ui/waterfall.h"

namespace app {

class Application {
 private:
  app::debug::Debug &dbg;

  float32_t powers[480] = {0};

  bool processing_job_ready = false;
  bool render_job_ready = false;

  void ProcessingJob();
  void RenderJob();

 public:
  app::hw::Display &display;
  app::ui::Painter &canvas;
  app::hw::Recorder &recorder;

  app::ui::Waterfall &waterfall;

  Application(app::debug::Debug &dbg,
              app::hw::Display &display,
              app::ui::Painter &canvas,
              app::hw::Recorder &recorder,
              app::ui::Waterfall &waterfall);
  int Init();
  void Loop();

  void HandleAudioInHalfTransferComplete();

  void HandleAudioInTransferComplete();

  void HandleAudioInError();

  void HandleLtdcUnderrun();

  void HandleLtdcReload();

  void HandleLtdcIRQ();
};

}  // namespace app
