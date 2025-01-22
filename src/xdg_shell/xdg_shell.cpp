#include "xdg_shell/xdg_shell.hpp"

#include <xdg-shell-protocol.h>

#include "common.hpp"
#include "xdg_shell/xdg_wm_base.hpp"

namespace yaza::xdg_shell {
bool init(wl_display* display) {
  if (!wl_global_create(display, &xdg_wm_base_interface, 1, nullptr,
          xdg_shell::xdg_wm_base::bind)) {
    LOG_ERR("Failed to create global (xdg_wm_base)");
    return false;
  }
  return true;
}
}  // namespace yaza::xdg_shell
