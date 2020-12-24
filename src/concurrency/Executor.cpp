#include <afina/concurrency/Executor.h>
#include <chrono>

namespace Afina {
namespace Concurrency {


Executor::Executor(std::string name, int size) {
    std::unique_lock<std::mutex> _lock(mutex);
    state = State::kRun;
    for (size_t id = 0; id < low_watermark; id++) {
        //std::thread(perform, this); doesnt work
        running_threads++;
        std::thread([this]{perform(this);});
    }
}

Executor::~Executor() {
    if (state == State::kRun) {
        bool await = false;
        Stop(await);
    }
}

void Executor::Stop(bool await) {
    std::unique_lock<std::mutex> lock(mutex);
    if (state == State::kRun){
        state = State::kStopping;
        //Notify sleeping threads in order to terminate
        empty_condition.notify_all();

        while (await && running_threads){
            await_cv.wait(lock);
        }
        state = State::kStopped;
    } else {
        return;
    }
}


void perform(Executor *executor) {
    std::unique_lock<std::mutex> lock(executor->mutex);
    while (executor->state == Executor::State::kRun) {
        if (executor->tasks.empty()) {
            if (executor->empty_condition.wait_for(lock, std::chrono::microseconds(100)) == std::cv_status::timeout) {
                if (executor->running_threads > executor->low_watermark) {
                    executor->running_threads -= 1;
                    break;
                }
                else {
                    executor->empty_condition.wait(lock);
                    if (executor->state != Executor::State::kRun) {
                        executor->running_threads -= 1;
                        break;
                    }
                }
            }
        }

        //Pop task from the queue

    }
}



}
} // namespace Afina
