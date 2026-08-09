#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <atomic>
#include <vector>
#include <cstddef>
#include <utility>

// 高性能无锁环形缓冲区，预分配内存，零堆分配开销，适用于高频 CAN 数据处理
template <typename T, size_t Capacity = 16384>
class LockFreeRingBuffer {
public:
  LockFreeRingBuffer() : m_head(0), m_tail(0), m_buffer(Capacity) {}

  // 生产者压入单条数据（线程安全无锁）
  bool push(const T &item) {
    size_t tail = m_tail.load(std::memory_order_relaxed);
    size_t nextTail = (tail + 1) % Capacity;
    if (nextTail == m_head.load(std::memory_order_acquire)) {
      return false; // 缓冲区已满
    }
    m_buffer[tail] = item;
    m_tail.store(nextTail, std::memory_order_release);
    return true;
  }

  // 消费者批量弹出一批数据（线程安全无锁）
  size_t pop_batch(std::vector<T> &outBatch, size_t maxCount = Capacity) {
    size_t head = m_head.load(std::memory_order_relaxed);
    size_t tail = m_tail.load(std::memory_order_acquire);
    size_t count = 0;
    outBatch.clear();

    while (head != tail && count < maxCount) {
      outBatch.push_back(std::move(m_buffer[head]));
      head = (head + 1) % Capacity;
      count++;
    }
    m_head.store(head, std::memory_order_release);
    return count;
  }

  size_t size() const {
    size_t head = m_head.load(std::memory_order_relaxed);
    size_t tail = m_tail.load(std::memory_order_relaxed);
    if (tail >= head) return tail - head;
    return Capacity - (head - tail);
  }

  bool isEmpty() const {
    return m_head.load(std::memory_order_relaxed) == m_tail.load(std::memory_order_relaxed);
  }

  void clear() {
    m_head.store(0, std::memory_order_relaxed);
    m_tail.store(0, std::memory_order_relaxed);
  }

private:
  std::atomic<size_t> m_head;
  std::atomic<size_t> m_tail;
  std::vector<T> m_buffer;
};

#endif // RINGBUFFER_H
