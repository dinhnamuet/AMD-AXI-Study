#pragma once

#include <stdatomic.h>
#include "gcc.h"
#include "xil_types.h"

struct msgq {
	size_t msg_size;
	uint32_t max_msgs;
	atomic_uint used_msgs;
	uint8_t *buffer_start;
	uint8_t *buffer_end;
	uint8_t *read_ptr;
	uint8_t *write_ptr;
};

#define MSGQ_INITIALIZER(q_buffer, q_msg_size, q_max_msgs)                                         \
	{                                                                                          \
		.msg_size = q_msg_size,                                                            \
		.max_msgs = q_max_msgs,                                                            \
		.used_msgs = ATOMIC_VAR_INIT(0),                                                   \
		.buffer_start = q_buffer,                                                          \
		.buffer_end = q_buffer + ((q_max_msgs) * (q_msg_size)),                            \
		.read_ptr = q_buffer,                                                              \
		.write_ptr = q_buffer,                                                             \
	}

#define MSGQ_DEFINE(name, msg_size, max_msgs, msg_align)                                           \
	static uint8_t __aligned(msg_align) msgbuf_##name[(msg_size) * (max_msgs)];                \
	struct msgq name = MSGQ_INITIALIZER(msgbuf_##name, msg_size, max_msgs)

#define MSGQ_DECLARE(name) extern struct msgq name

/**
 * @brief Peek at the next message without consuming it.
 *
 * The returned pointer remains valid until msgq_skip() is called.
 * The slot is not released to producers until msgq_skip() is called.
 * Must be paired with exactly one msgq_skip() call on success.
 *
 * @return Pointer to message, NULL if empty.
 */
const void *msgq_peek(struct msgq *msgq);
/**
 * @brief Release the slot returned by the last msgq_peek() call.
 *
 * Advances read_ptr and decrements used_msgs, making the slot available
 * for producers. Must be called exactly once after a successful msgq_peek().
 */
void msgq_skip(struct msgq *msgq);
int msgq_get(struct msgq *msgq, void *out, size_t size);
int msgq_put(struct msgq *msgq, const void *data);
/**
 * @brief Put message into ring buffer. Safe from any context.
 *
 * Wraps msgq_put() in a PRIMASK-based critical section (interrupt
 * disable/restore), providing mutual exclusion among multiple producers
 * (ISRs or Thread mode) on a single-core MCU.
 *
 * @return 0 on success, -1 if full.
 */
int msgq_put_safe(struct msgq *msgq, const void *data);
void msgq_reset(struct msgq *msgq);
