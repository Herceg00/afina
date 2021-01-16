#include <afina/coroutine/Engine.h>

#include <cstring>
#include <csetjmp>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char Stack = 0;
    if (&Stack > StackBottom) {
        ctx.Hight = &Stack;
    } else {
        ctx.Low = &Stack;
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
    std::size_t stack_size = ctx.Hight - ctx.Low;
    std::memcpy(ctx.Low, std::get<0>(ctx.Stack), stack_size);
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    auto it = alive;
    if (it && it == cur_routine) {
        it = it->next;
    }
    if (it) {
        sched(it);
    }
}

void Engine::sched(void *routine_) {
    if (routine_ == nullptr or routine_ == cur_routine) {
       yield();
    }
    if (setjmp(cur_routine -> Environment)){
        return;
    }
    Store(*cur_routine);
    cur_routine = (context *)routine_;
    Restore(*(context *)routine_);
}

} // namespace Coroutine
} // namespace Afina
