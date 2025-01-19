#include "zwin/shm/shm_pool.hpp"

#include <sys/mman.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-server.h>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>

#include "common.hpp"
#include "util/convert.hpp"
#include "zwin/shm/shm_buffer.hpp"

#ifndef MREMAP_MAYMOVE
#error "mremap(MREMAP_MAYMOVE) is not implemented but it is required"
#endif

namespace yaza::zwin::shm_pool {
namespace {
void* grow_mapping(shm_pool::ShmPool* pool) {
  void* data =
      mremap(pool->data_, pool->size_, pool->new_size_, MREMAP_MAYMOVE);
  return data;
}
void finish_resize(shm_pool::ShmPool* pool) {
  if (pool->size_ == pool->new_size_) {
    return;
  }

  void* data = grow_mapping(pool);
  if (data == MAP_FAILED) {
    wl_resource_post_error(
        pool->resource_, ZWN_SHM_ERROR_INVALID_FD, "failed mremap");
    return;
  }

  pool->data_ = static_cast<char*>(data);
  pool->size_ = pool->new_size_;
}

void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void create_buffer(wl_client* client, wl_resource* resource, uint32_t id,
    wl_array* offset_array, wl_array* size_array) {
  auto* pool = static_cast<ShmPool*>(wl_resource_get_user_data(resource));

  off_t offset = 0;
  off_t size   = 0;
  if (util::convert_wl_array(offset_array, &offset) != 0 ||
      util::convert_wl_array(size_array, &size) != 0) {
    wl_resource_post_error(
        resource, ZWN_SHM_ERROR_INVALID_SIZE, "requested size is invalid");
  }

  if (offset < 0 || size <= 0 || offset > pool->size_ - size) {
    wl_resource_post_error(resource, ZWN_SHM_ERROR_INVALID_SIZE,
        "requested size (%ld) is invalid", size);
    return;
  }

  if (shm_buffer::new_buffer(client, id, pool, size, offset)) {
    pool->internal_refcount_++;
  }
}
void resize(wl_client* /*client*/, wl_resource* resource, int32_t size) {
  auto* pool = static_cast<ShmPool*>(wl_resource_get_user_data(resource));

  if (size < pool->size_) {
    wl_resource_post_error(
        resource, ZWN_SHM_ERROR_INVALID_FD, "shrinking pool invalid");
    return;
  }

  pool->new_size_ = size;

  /* If the compositor has taken references on this pool it
   * may be caching pointers into it. In that case we
   * defer the resize (which may move the entire mapping)
   * until the compositor finishes dereferencing the pool.
   */
  if (pool->external_refcount_ == 0) {
    finish_resize(pool);
  }
}
const struct zwn_shm_pool_interface kImpl = {
    .destroy       = destroy,
    .create_buffer = create_buffer,
    .resize        = resize,
};

void destroy_pool(wl_resource* resource) {
  auto* pool = static_cast<ShmPool*>(wl_resource_get_user_data(resource));
  unref(pool, false);
}
}  // namespace

ShmPool* new_pool(wl_client* client, wl_resource* resource, uint32_t id,
    int32_t fd, off_t size, bool sigbuf_is_impossible) {
  auto* pool = zalloc<shm_pool::ShmPool*>(sizeof(shm_pool::ShmPool));
  if (pool == nullptr) {
    wl_client_post_no_memory(client);
    goto err;
  }

  pool->internal_refcount_    = 1;
  pool->external_refcount_    = 0;
  pool->size_                 = size;
  pool->new_size_             = size;
  pool->sigbuf_is_impossible_ = sigbuf_is_impossible;
  pool->data_ =
      static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
  if (pool->data_ == MAP_FAILED) {
    wl_resource_post_error(resource, ZWN_SHM_ERROR_INVALID_FD,
        "failed mmap fd %d: %s", fd, std::strerror(errno));
    goto err_free;
  }

  pool->resource_ = wl_resource_create(client, &zwn_shm_pool_interface, 1, id);
  if (!pool->resource_) {
    wl_client_post_no_memory(client);
    goto err_munmap;
  }

  wl_resource_set_implementation(pool->resource_, &kImpl, pool, destroy_pool);
  return pool;
err_munmap:
  munmap(pool->data_, pool->size_);
err_free:
  free(pool);
err:
  return nullptr;
}

void unref(ShmPool* pool, bool external) {
  if (external) {
    pool->external_refcount_--;
    assert(pool->external_refcount_ >= 0);
    if (pool->external_refcount_ == 0) {
      finish_resize(pool);
    }
  } else {
    pool->internal_refcount_--;
    assert(pool->internal_refcount_ >= 0);
  }

  if (pool->internal_refcount_ + pool->external_refcount_ > 0) {
    return;
  }

  munmap(pool->data_, pool->size_);
  free(pool);
}
}  // namespace yaza::zwin::shm_pool
