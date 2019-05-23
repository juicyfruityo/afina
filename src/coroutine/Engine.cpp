#include <afina/coroutine/Engine.h>

#include <cstring>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char cur_address;
    ctx.Hight = &cur_address;
    ctx.Low = StackBottom;
    if (ctx.Hight < ctx.Low) {
        auto tmp = ctx.Hight;
        ctx.Hight = ctx.Low;
        ctx.Low = tmp;
    }

    if (std::get<0>(ctx.Stack) != nullptr) {
        delete std::get<0>(ctx.Stack);
    }
    std::size_t length = ctx.Hight - ctx.Low;
    char *stack = new char[length];
    std::memcpy(stack, ctx.Low, length);
    ctx.Stack = std::make_tuple(stack, length);
}

void Engine::Restore(context &ctx) {
    char cur_address;
    // because we don't know what is direction of growing
    while ((ctx.Low < &cur_address) && (ctx.Hight > &cur_address)) {
        Restore(ctx);
    }

    std::memcpy(ctx.Low, std::get<0>(ctx.Stack), std::get<1>(ctx.Stack));
    cur_routine = &ctx;
    // TOASK: any number except 0?
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    Store(*cur_routine);
    auto routine_todo = alive;
    if (routine_todo == cur_routine) {
        if (alive->next != nullptr) {
            routine_todo = alive->next;
            sched(static_cast<void *>(routine_todo));
        }
    }
}

void Engine::sched(void *routine_) {
    if (setjmp(cur_routine->Environment) == 0) {
        Store(*cur_routine);
        context *ctx = static_cast<context *>(routine_);
        Restore(*ctx);
    }
}

} // namespace Coroutine
} // namespace Afina
