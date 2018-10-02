#include <mbed.h>

#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_sdram.h"

#include "application.h"
#include "debug/class.h"
#include "debug/funcs.h"

#include "test_dma.h"

// Singleton called by interrupt handlers - stays null in tests
app::Application* volatile global_app;

int main() {
  Serial serial(USBTX, USBRX);

  app::debug::Debug dbg(serial);

  dbg.printf("\nInit tests...\n");

  HAL_Init();
  BSP_SDRAM_Init();
  app::debug::init(dbg);

  dbg.printf("\nBegin tests...\n");

  test_dma(dbg);

  dbg.printf("Tests complete.\n");
}
