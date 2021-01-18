#include <afina/coroutine/Engine.h>

#include <cstring>
#include <csetjmp>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char stack_end;
    ctx.Hight = ctx.Low = StackBottom;
    if (&stack_end > StackBottom) {
        ctx.Hight = &stack_end;
    }
    else {
        ctx.Low = &stack_end;
    }
    std::size_t stack_size = ctx.Hight - ctx.Low;
    if (stack_size > std::get<1>(ctx.Stack) || stack_size * 2 < std::get<1>(ctx.Stack)) {
        delete[] std::get<0>(ctx.Stack);
        std::get<1>(ctx.Stack) = stack_size;
        std::get<0>(ctx.Stack) = new char[stack_size];
    }
    std::memcpy(std::get<0>(ctx.Stack), ctx.Low, stack_size);

}

void Engine::Restore(context &ctx) {
    char stack_end;
    if ((&stack_end >= ctx.Low) && (&stack_end <= ctx.Hight)) {
        Restore(ctx);
    }

    char* &buffer = std::get<0>(ctx.Stack);
    std::size_t stack_size = ctx.Hight - ctx.Low;

    memcpy(ctx.Low, buffer, stack_size);
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    context * next_coro = alive;
    if (next_coro != nullptr){
        if (next_coro == cur_routine){
            next_coro = next_coro ->next;
        }
        sched(next_coro);
    }
}

void Engine::sched(void *routine_) {
    if (cur_routine == routine_) {
        return yield();
    }

    if (routine_) {
        if (cur_routine && cur_routine != idle_ctx) {
            if (setjmp(cur_routine->Environment) > 0) {
                return;
            }
            Store(*cur_routine);
        }

        cur_routine = static_cast<context *>(routine_);
        Restore(*cur_routine);
    } else {
        yield();
    }

}

} // namespace Coroutine
} // namespace Afina
