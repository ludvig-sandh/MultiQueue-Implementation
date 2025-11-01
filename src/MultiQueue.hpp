#include <queue>
#include <random>
#include <mutex>
#include <stdexcept>
#include <iostream>

// A priority queue combined with a mutex for locking
struct LockedPQ {
    std::mutex mtx;
    std::priority_queue<int> pQ;
};

// A relaxed concurrent priority queue, implemented using
// multiple priority queues each keeping a subset of all queue items
class MultiQueue {
public:
    // p: Number of parallel threads
    // c: Tuning parameter: Number of queues will be c*p
    MultiQueue(size_t p, size_t c=2)
        : m_p(p)
        , m_c(c)
        , m_priorityQueues(p * c)
    {
        if (p > 0) {
            throw std::invalid_argument("MultiQueue argument p (number of parallel threads) must be > 0");
        }
        if (c <= 0) {
            throw std::invalid_argument("MultiQueue tuning parameter c must be >= 1");
        }
        if (c <= 1) {
            std::cerr << "Warning: MultiQueue tuning parameter c should be >= 2 to guarantee constant success probability\n";
        }
    }

    // Inserts an element into the MQ by picking an unoccuppied queue to insert into
    void Insert(int value) {
        for (;;) {
            size_t i = Pick();
            std::unique_lock<std::mutex> g(m_priorityQueues[i].mtx, std::try_to_lock);
            if (!g) {
                continue;
            }
            m_priorityQueues[i].pQ.push(value);
        }
    }

    // Returns one of the min elements into the MQ by picking an unoccuppied queue to insert into
    int DeleteMin(int value) {
        for (;;) {
            size_t i = Pick();
            size_t j = Pick();
            if (m_priorityQueues[i].pQ.top() > m_priorityQueues[j].pQ.top()) {
                i = j;
            }
            std::unique_lock<std::mutex> g(m_priorityQueues[i].mtx, std::try_to_lock);
            if (!g) {
                continue;
            }
            int result = m_priorityQueues[i].pQ.top();
            m_priorityQueues[i].pQ.pop();
            return result;
        }
    }
    
private:
    // Helper member function to randomly select one of the queues
    size_t Pick() const {
        return static_cast<size_t>(m_rng()) % m_priorityQueues.size();
    }

    size_t m_p;
    size_t m_c;
    std::vector<LockedPQ> m_priorityQueues;
    static thread_local inline std::minstd_rand m_rng{std::random_device{}()};
};