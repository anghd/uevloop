/** \file object-pool.h
  *
  * \brief Defines object pools, arrays of pre-allocated objects for dynamic use
  */

#ifndef UEL_OBJECT_POOL_H
#define	UEL_OBJECT_POOL_H

#include "uevloop/utils/circular-queue.h"

/// \cond
#include <stdint.h>
#include <stdlib.h>
/// \endcond

/** \brief Pre-allocated memory bound to speciffic types suitable for providing
  * dynamic object management in the stack.
  *
  * Object pools are arrays of objects that are pre-allocated in the stack at
  * compile time as an alternative to runtime memory allocation for dynamic
  * object management.
  *
  * To efficiently release and acquire objects from a pool, their addresses are
  * kept in a circular queue that is fully populated during initialisation.
  */
typedef struct uel_objpool uel_objpool_t;
struct uel_objpool {
    //! The buffer that contains each object managed by this pool.
    uint8_t *buffer;
    //! The queue containing the addresses for each object in the pool.
    uel_cqueue_t queue;
};

/** \brief Initialises an object pool
  *
  * \param pool The pool to be initialised
  * \param size_log2n The number of objects in the pool in its log2 form
  * \param item_size The size of each object in the pool. If special alignment
  * is required, it must be included in this value.
  * \param buffer The buffer that contains each object in the pool. Must be
  * `2**size_log2n * item_size` long.
  * \param queue_buffer A void pointer array that will be used as the buffer to
  * the object pointer queue. Must be `2**size_log2n` long.
  */
void uel_objpool_init(
    uel_objpool_t *pool,
    size_t size_log2n,
    size_t item_size,
    uint8_t *buffer,
    void **queue_buffer
);

/** \brief Acquires an object from the pool.
  *
  * \param pool The pool from where to acquire the object
  * \return A pointer to the acquired object or NULL if the pool is depleted
  */
void *uel_objpool_acquire(uel_objpool_t *pool);

/** \brief Releases an object to the pool
  *
  * \param pool The pool where the object will be released to
  * \param element The element to be returned to the pool
  * \return Whether the object could be released
  */
bool uel_objpool_release(uel_objpool_t *pool, void *element);

/** \brief Checks if a pool is depleted
  *
  * \param pool The pool to be verified
  * \return Whether the pool is empty (*i.e.*: All addresses have been given out)
  */
bool uel_objpool_is_empty(uel_objpool_t *pool);

/** \brief Declares the necessary buffers to back an object pool, so the
  * programmer doesn't have to reason much about it.
  *
  * Use this macro as a shortcut to create the required buffers for an object pool.
  * This will declare two buffers in the calling scope.
  *
  * \param type The type of the objects the pool will contain
  * \param size_log2n The number of elements the pool will contain in log2 form
  * \param id A valid identifier for the pools.
  */
#define UEL_DECLARE_OBJPOOL_BUFFERS(type, size_log2n, id)           \
    type id##_pool_buffer[(1<<size_log2n)];                         \
    void *id##_pool_queue_buffer[1<<size_log2n]

/** \brief Refers to a previously declared buffer set.
  *
  * This is a convenience macro to supply the buffers generated by
  * `UEL_DECLARE_OBJPOOL_BUFFERS` to the `uel_objpool_init` function.
  *
  * \param id The identifier used to declare the pool buffers
  */
#define UEL_OBJPOOL_BUFFERS(id)                                     \
    (uint8_t *)&id##_pool_buffer, id##_pool_queue_buffer

/** \brief Refers to a previously declared buffer set.
  *
  * This is a convenience macro to supply the buffers generated by
  * `UEL_DECLARE_OBJPOOL_BUFFERS` to the `uel_objpool_init` function.
  * Use this if the buffers were defined inside a local object, accessible in the
  * current scope.
  *
  * \param id The identifier used to declare the pool buffers
  * \param obj The object storing the pool buffers
  */
#define UEL_OBJPOOL_BUFFERS_IN(id, obj)                             \
    (uint8_t *)&obj.id##_pool_buffer, obj.id##_pool_queue_buffer

/** \brief Refers to a previously declared buffer set.
  *
  * This is a convenience macro to supply the buffers generated by
  * `UEL_DECLARE_OBJPOOL_BUFFERS` to the `uel_objpool_init` function.
  * Use this if the buffers were defined inside an object whose address is
  * accessible in the current scope
  *
  * \param id The identifier used to declare the pool buffers
  * \param obj The address of the object storing the pool buffers
  */
#define UEL_OBJPOOL_BUFFERS_AT(id, obj)                             \
    (uint8_t *)&obj->id##_pool_buffer, obj->id##_pool_queue_buffer

#endif	/* UEL_OBJECT_POOL_H */
