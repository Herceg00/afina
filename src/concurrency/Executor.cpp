#include <afina/concurrency/Executor.h>
#include <chrono>

namespace Afina {
namespace Concurrency {


Executor::Executor(std::string name, int size) {
    max_queue_size = size;
    std::unique_lock<std::mutex> _lock(mutex);
    state = State::kRun;
    for (size_t id = 0; id < low_watermark; id++) {
        //std::thread(perform, this); doesnt work
        running_threads++;
        std::thread([this]{perform(this, false);});
    }
}

Executor::~Executor() {
    if (state == State::kRun) {
        bool await = true;
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


void perform(Executor *executor, bool has_personal_task) {

    std::unique_lock<std::mutex> lock(executor->mutex);
    while (executor->state == Executor::State::kRun) {
        if (executor->tasks.empty() && !has_personal_task) {
            if (executor->empty_condition.wait_for(lock, std::chrono::milliseconds(executor->idle_time)) == std::cv_status::timeout) {
                if (executor->running_threads > executor->low_watermark) {
                    break;
                }
                else {
                    executor->empty_condition.wait(lock);
                    if (executor->state != Executor::State::kRun) {
                        break;
                    }
                }
            }
        }

        //Pop task from the queue
        auto task = executor->tasks.front();
        executor->tasks.pop_front();
        executor->busy_threads++;

        /* Perform a task*/
        lock.unlock();
        task();
        has_personal_task = false;
        lock.lock();

        executor->busy_threads--;

        if (executor->state != Executor::State::kRun) {
            break;
        }
    }
    executor->running_threads--;
    if (executor->running_threads == 0){
        executor->await_cv.notify_one();
    }
}



}
} // namespace Afina
