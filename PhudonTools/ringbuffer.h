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

  // 消费者清空并提取所有数据
  size_t pop_all(std::vector<T> &outBatch) {
    return pop_batch(outBatch, Capacity);
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

// 工业级 Min-Max 极值抽样算法（用于高频趋势曲线与示波器绘制降采样）
// 核心逻辑：将像素显示宽度划分 N 个 Bucket，每个 Bucket 精确抽取 [First, Min, Max, Last] 极值点，
// 在压缩 95%+ 数据量的同时 100% 保留波峰、波谷与噪声毛刺。
class MinMaxDownsampler {
public:
  static void process(const double *keys, const double *values, int totalCount,
                      int targetPixelWidth, std::vector<double> &outKeys,
                      std::vector<double> &outValues) {
    outKeys.clear();
    outValues.clear();

    if (totalCount <= 0 || !keys || !values) {
      return;
    }

    // 若数据量未超出像素计数的 2 倍，无需抽样，直接完整保留
    const int threshold = targetPixelWidth * 2;
    if (totalCount <= threshold || targetPixelWidth <= 0) {
      outKeys.assign(keys, keys + totalCount);
      outValues.assign(values, values + totalCount);
      return;
    }

    outKeys.reserve(targetPixelWidth * 4);
    outValues.reserve(targetPixelWidth * 4);

    const double minX = keys[0];
    const double maxX = keys[totalCount - 1];
    const double rangeX = maxX - minX;

    if (rangeX <= 0) {
      outKeys.assign(keys, keys + totalCount);
      outValues.assign(values, values + totalCount);
      return;
    }

    const double bucketWidth = rangeX / targetPixelWidth;
    int curIdx = 0;

    for (int b = 0; b < targetPixelWidth; ++b) {
      const double bucketStart = minX + b * bucketWidth;
      const double bucketEnd = bucketStart + bucketWidth;

      if (curIdx >= totalCount) break;
      int firstIdx = curIdx;
      int minIdx = curIdx;
      int maxIdx = curIdx;
      int lastIdx = curIdx;
      double minY = values[curIdx];
      double maxY = values[curIdx];

      int countInBucket = 0;
      while (curIdx < totalCount && keys[curIdx] < bucketEnd) {
        double val = values[curIdx];
        if (val < minY) {
          minY = val;
          minIdx = curIdx;
        }
        if (val > maxY) {
          maxY = val;
          maxIdx = curIdx;
        }
        lastIdx = curIdx;
        curIdx++;
        countInBucket++;
      }

      if (countInBucket == 0) {
        continue;
      }

      // 将该 Bucket 中的关键特征点（按时间先后顺序）收集
      // 保持时间单调递增，防止 QCustomPlot 绘图线段倒勾
      int indices[4] = {firstIdx, minIdx, maxIdx, lastIdx};
      // 简单插入排序去重并按索引升序排列
      for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
          if (indices[i] > indices[j]) {
            std::swap(indices[i], indices[j]);
          }
        }
      }

      int prevIdx = -1;
      for (int i = 0; i < 4; ++i) {
        int idx = indices[i];
        if (idx != prevIdx && idx < totalCount) {
          outKeys.push_back(keys[idx]);
          outValues.push_back(values[idx]);
          prevIdx = idx;
        }
      }
    }
  }
};

#endif // RINGBUFFER_H
