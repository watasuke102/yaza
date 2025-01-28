#include "zwin/shm/shm.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <zwin-protocol.h>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "util/convert.hpp"
#include "zwin/shm/shm_pool.hpp"

namespace yaza::zwin::shm {
namespace {
void create_pool(wl_client* client, wl_resource* resource, uint32_t id,
    int32_t fd, wl_array* size_array) {
  struct stat stat_buffer;
  off_t       size                 = 0;
  int         seals                = 0;
  bool        sigbuf_is_impossible = false;

  if (util::convert_wl_array(size_array, &size) != 0) {
    wl_resource_post_error(
        resource, ZWN_SHM_ERROR_INVALID_SIZE, "requested size is invalid");
    close(fd);
    return;
  }
  if (size <= 0) {
    wl_resource_post_error(resource, ZWN_SHM_ERROR_INVALID_SIZE,
        "requested size (%ld) is invalid", size);
    close(fd);
    return;
  }

  seals = fcntl(fd, F_GET_SEALS);
  if (seals == -1) {
    seals = 0;
  }
  if ((seals & F_SEAL_SHRINK) && fstat(fd, &stat_buffer) >= 0) {
    sigbuf_is_impossible = stat_buffer.st_size >= size;
  }

  shm_pool::new_pool(client, resource, id, fd, size, sigbuf_is_impossible);
  close(fd);
}
constexpr struct zwn_shm_interface kImpl = {
    .create_pool = create_pool,
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwn_shm_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}

}  // namespace yaza::zwin::shm
