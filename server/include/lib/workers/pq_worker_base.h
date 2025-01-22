#ifndef PQ_WORKER_BASE_H
#define PQ_WORKER_BASE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <iostream>
#include <functional>
#include <memory>

template <typename RequestType, typename Comparator = std::less<RequestType> >
class PriorityQueueWorkerBase {
protected:
    using QueueType = std::priority_queue<RequestType, std::vector<RequestType>, Comparator>;

    std::unique_ptr<QueueType> request_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_condition;
    std::thread processing_thread;
    std::atomic<bool> stop_processing{false};

    void initialize_queue(Comparator comp = Comparator{}) {
        request_queue = std::make_unique<QueueType>(comp);
    }

    RequestType pop_request() {
        RequestType req = request_queue->top();
        request_queue->pop();
        return req;
    }

    void process_data() {
        while (true) {
            RequestType req;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                queue_condition.wait(lock, [this]() { return !request_queue->empty() || stop_processing; });

                if (stop_processing && request_queue->empty()) break;

                req = pop_request();
            }

            process_request(req);
        }
        std::cout << "Stopped PriorityQueueWorkerBase..." << std::endl;
    }

    virtual void process_request(RequestType req) = 0;

public:
    PriorityQueueWorkerBase(Comparator comp = Comparator{}) {
        initialize_queue(comp);
    }

    virtual ~PriorityQueueWorkerBase() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop_processing = true;
            queue_condition.notify_all();
        }
        if (processing_thread.joinable()) {
            processing_thread.join();
        }
    }

    void start_processing() {
        processing_thread = std::thread(&PriorityQueueWorkerBase::process_data, this);
    }

    void push_request(RequestType req) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        request_queue->push(req);
        queue_condition.notify_one();
    }
};

#endif