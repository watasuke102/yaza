#include "zwin/shm/shm_buffer.hpp"

#include <pthread.h>
#include <sys/mman.h>
#include <zwin-protocol.h>

#include <cassert>
#include <csignal>
#include <cstdint>

#include "common.hpp"
#include "zwin/shm/shm_pool.hpp"

namespace yaza::zwin::shm_buffer {
struct ShmBuffer {
  wl_resource*       resource;
  ssize_t            size;
  uintptr_t          offset;
  shm_pool::ShmPool* pool;
};

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct zwn_buffer_interface kImpl = {
    .destroy = destroy,
};

void destroy_buffer(wl_resource* resource) {
  auto* buffer = static_cast<ShmBuffer*>(wl_resource_get_user_data(resource));
  shm_pool::unref(buffer->pool, false);
  free(buffer);
}
}  // namespace

ShmBuffer* new_buffer(wl_client* client, uint32_t id, shm_pool::ShmPool* pool,
    off_t size, off_t offset) {
  auto* buffer = zalloc<shm_buffer::ShmBuffer*>(sizeof(shm_buffer::ShmBuffer));
  if (buffer == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  buffer->size   = size;
  buffer->offset = offset;
  buffer->pool   = pool;

  buffer->resource = wl_resource_create(client, &zwn_buffer_interface, 1, id);
  if (buffer->resource == nullptr) {
    wl_client_post_no_memory(client);
    free(buffer);
    return nullptr;
  }
  wl_resource_set_implementation(
      buffer->resource, &shm_buffer::kImpl, buffer, destroy_buffer);
  return buffer;
}

namespace {
/* This once_t is used to synchronize installing the SIGBUS handler
 * and creating the TLS key. This will be done in the first call
 * shm_buffer::begin_access which can happen from any thread */
pthread_once_t   shm_sigbus_once = PTHREAD_ONCE_INIT;
pthread_key_t    shm_sigbus_data_key;
struct sigaction shm_old_sigbus_action;
struct ShmSigbusData {
  shm_pool::ShmPool* current_pool;
  int                access_count;
  int                fallback_mapping_used;
};
void reraise_sigbus() {
  /* If SIGBUS is raised for some other reason than accessing
   * the pool then we'll uninstall the signal handler so we can
   * reraise it. This would presumably kill the process */
  sigaction(SIGBUS, &shm_old_sigbus_action, nullptr);
  (void)std::raise(SIGBUS);
}
void handle_sigbus(int /*signum*/, siginfo_t* info, void* /*context*/) {
  auto* sigbus_data =
      static_cast<ShmSigbusData*>(pthread_getspecific(shm_sigbus_data_key));

  if (sigbus_data == nullptr) {
    reraise_sigbus();
    return;
  }

  auto* pool = sigbus_data->current_pool;
  if (pool == nullptr ||  //
      static_cast<char*>(info->si_addr) < pool->data ||
      static_cast<char*>(info->si_addr) >= pool->data + pool->size)  // NOLINT
  {
    reraise_sigbus();
    return;
  }

  sigbus_data->fallback_mapping_used = 1;

  if (mmap(pool->data, pool->size, PROT_READ | PROT_WRITE,
          MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, 0, 0) == MAP_FAILED) {
    reraise_sigbus();
  }
}
void destroy_sigbus_data(void* data) {
  auto* sigbus_data = static_cast<ShmSigbusData*>(data);
  free(sigbus_data);
}
void init_sigbus_data_key() {
  struct sigaction new_action;
  new_action.sa_sigaction = handle_sigbus,
  new_action.sa_flags     = SA_SIGINFO | SA_NODEFER,
  sigemptyset(&new_action.sa_mask);
  sigaction(SIGBUS, &new_action, &shm_old_sigbus_action);
  pthread_key_create(&shm_sigbus_data_key, destroy_sigbus_data);
}
}  // namespace

/**
 * Mark that the given SHM buffer is about to be accessed
 *
 * \param buffer The SHM buffer
 *
 * An SHM buffer is a memory-mapped file given by the client.
 * According to POSIX, reading from a memory-mapped region that
 * extends off the end of the file will cause a SIGBUS signal to be
 * generated. Normally this would cause the compositor to terminate.
 * In order to make the compositor robust against clients that change
 * the size of the underlying file or lie about its size, you should
 * protect access to the buffer by calling this function before
 * reading from the memory and call shm_buffer::end_access
 * afterwards. This will install a signal handler for SIGBUS which
 * will prevent the compositor from crashing.
 *
 * After calling this function the signal handler will remain
 * installed for the lifetime of the compositor process. Note that
 * this function will not work properly if the compositor is also
 * installing its own handler for SIGBUS.
 *
 * If a SIGBUS signal is received for an address within the range of
 * the SHM pool of the given buffer then the client will be sent an
 * error event when shm_buffer::end_access is called. If the signal
 * is for an address outside that range then the signal handler will
 * reraise the signal which would will likely cause the compositor to
 * terminate.
 *
 * It is safe to nest calls to these functions as long as the nested
 * calls are all accessing the same buffer. The number of calls to
 * shm_buffer::end_access must match the number of calls to
 * shm_buffer::begin_access. These functions are thread-safe and it
 * is allowed to simultaneously access different buffers or the same
 * buffer from multiple threads.
 */
void begin_access(ShmBuffer* buffer) {
  shm_pool::ShmPool* pool = buffer->pool;

  if (pool->sigbuf_is_impossible) {
    return;
  }

  pthread_once(&shm_sigbus_once, init_sigbus_data_key);

  auto* sigbus_data =
      static_cast<ShmSigbusData*>(pthread_getspecific(shm_sigbus_data_key));
  if (sigbus_data == nullptr) {
    sigbus_data = zalloc<ShmSigbusData*>(sizeof(*sigbus_data));
    if (sigbus_data == nullptr) {
      return;
    }

    pthread_setspecific(shm_sigbus_data_key, sigbus_data);
  }

  assert(sigbus_data->current_pool == nullptr ||
         sigbus_data->current_pool == pool);

  sigbus_data->current_pool = pool;
  sigbus_data->access_count++;
}

/**
 * Ends the access to a buffer started by shm_buffer::begin_access
 *
 * \param buffer The SHM buffer
 *
 * This should be called after shm_buffer::begin_access once the
 * buffer is no longer being accessed. If a SIGBUS signal was
 * generated in-between these two calls then the resource for the
 * given buffer will be sent an error.
 */
void end_access(ShmBuffer* buffer) {
  auto* pool = buffer->pool;

  if (pool->sigbuf_is_impossible) {
    return;
  }

  auto* sigbus_data =
      static_cast<ShmSigbusData*>(pthread_getspecific(shm_sigbus_data_key));
  assert(sigbus_data && sigbus_data->access_count >= 1);

  if (--sigbus_data->access_count == 0) {
    if (sigbus_data->fallback_mapping_used) {
      wl_resource_post_error(buffer->resource, ZWN_SHM_ERROR_INVALID_FD,
          "error accessing SHM buffer");
      sigbus_data->fallback_mapping_used = 0;
    }

    sigbus_data->current_pool = nullptr;
  }
}
ShmBuffer* get_buffer(struct wl_resource* resource) {
  if (resource == nullptr) {
    return nullptr;
  }
  if (wl_resource_instance_of(resource, &zwn_buffer_interface, &kImpl)) {
    return static_cast<ShmBuffer*>(wl_resource_get_user_data(resource));
  }
  return nullptr;
}
void* get_buffer_data(ShmBuffer* buffer) {
  if (buffer->pool->external_refcount &&
      (buffer->pool->size != buffer->pool->new_size)) {
    LOG_WARN(
        "Buffer address requested when its parent pool has an external "
        "reference and a deferred resize pending.");
  }
  return buffer->pool->data + buffer->offset;  // NOLINT
}
ssize_t get_buffer_size(ShmBuffer* buffer) {
  return buffer->size;
}
}  // namespace yaza::zwin::shm_buffer
