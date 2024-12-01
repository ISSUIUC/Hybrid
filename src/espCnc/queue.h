#pragma once

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

/**
 * @brief The default maximum length for a Queue.
 */

/**
 * @brief A Thread-safe Queue containing a single data type.
 *
 * @tparam T The data type stored in the queue.
 * @tparam length The maximum length of the queue. Defaults to QUEUE_LENGTH.
 */
template<typename T, uint64_t length>
class Queue {
// private:
public:
    // todo probably have the Queue Store the timestamps too
    uint8_t queue_area[sizeof(StaticQueue_t) + 64];
    uint8_t buffer[sizeof(T) * length]{};
    QueueHandle_t queue{};

public:
    Queue() {
        queue = xQueueCreateStatic(length, sizeof(T), buffer, (StaticQueue_t*) &queue_area[32]);
    }
    Queue(const Queue&) = delete;
    Queue(Queue&&) = delete;

    bool send(T value) {
        return xQueueSendToBack(queue, &value, 0) == pdTRUE;
    }

    bool send_wait(T value) {
        return xQueueSendToBack(queue, &value, portMAX_DELAY) == pdTRUE;
    }

    bool receive(T* out) {
        return xQueueReceive(queue, out, 0) == pdTRUE;
    }

    bool receive_wait(T* out) {
        return xQueueReceive(queue, out, portMAX_DELAY) == pdTRUE;
    }
};
