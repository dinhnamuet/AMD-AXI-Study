#include "msgq.h"
#include <string.h>

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define MSGQ_LOCK(key)                                                                             \
	do {                                                                                       \
		(key) = __getPRIMASK();                                                            \
		__disable_irq();                                                                   \
	} while (0)
#define MSGQ_UNLOCK(key) __setPRIMASK(key)
#elif defined(__GNUC__) && !defined(__MICROBLAZE__)
#define MSGQ_LOCK(key)                                                                             \
	do {                                                                                       \
		(key) = __get_PRIMASK();                                                           \
		__disable_irq();                                                                   \
	} while (0)
#define MSGQ_UNLOCK(key) __set_PRIMASK(key)
#elif defined(__MICROBLAZE__)
#include "mb_interface.h"
#define MSGQ_LOCK(key) \
	do { \
		(key) = mfmsr(); \
		microblaze_disable_interrupts(); \
	} while (0)
#define MSGQ_UNLOCK(key) mtmsr(key)
#else
#error "Unsupported compiler: define MSGQ_LOCK/MSGQ_UNLOCK for your toolchain"
#endif

int msgq_put(struct msgq *msgq, const void *data)
{
	if (atomic_load_explicit(&msgq->used_msgs, memory_order_acquire) >= msgq->max_msgs) {
		return -1;
	}

	memcpy(msgq->write_ptr, data, msgq->msg_size);
	msgq->write_ptr += msgq->msg_size;
	if (msgq->write_ptr >= msgq->buffer_end) {
		msgq->write_ptr = msgq->buffer_start;
	}
	atomic_fetch_add_explicit(&msgq->used_msgs, 1, memory_order_release);

	return 0;
}

const void *msgq_peek(struct msgq *msgq)
{
	if (atomic_load_explicit(&msgq->used_msgs, memory_order_acquire) == 0) {
		return NULL;
	}
	return msgq->read_ptr;
}

void msgq_skip(struct msgq *msgq)
{
	if (atomic_load_explicit(&msgq->used_msgs, memory_order_acquire) == 0) {
		return;
	}
	msgq->read_ptr += msgq->msg_size;
	if (msgq->read_ptr >= msgq->buffer_end) {
		msgq->read_ptr = msgq->buffer_start;
	}
	atomic_fetch_sub_explicit(&msgq->used_msgs, 1, memory_order_release);
}

int msgq_get(struct msgq *msgq, void *out, size_t size)
{
	const void *src;

	if (!out || !size) {
		return -1;
	}
	src = msgq_peek(msgq);
	if (!src) {
		return -1;
	}
	memcpy(out, src, (size > msgq->msg_size) ? msgq->msg_size : size);
	msgq_skip(msgq);
	return 0;
}

int msgq_put_safe(struct msgq *msgq, const void *data)
{
	int ret;
	uint32_t flags;

	MSGQ_LOCK(flags);
	ret = msgq_put(msgq, data);
	MSGQ_UNLOCK(flags);
	return ret;
}

/* Must be called with all producers disabled (interrupts off or before scheduler
 * starts). read_ptr and write_ptr are written non-atomically.
 */
void msgq_reset(struct msgq *msgq)
{
	atomic_store_explicit(&msgq->used_msgs, 0, memory_order_seq_cst);
	msgq->read_ptr = msgq->buffer_start;
	msgq->write_ptr = msgq->buffer_start;
}
