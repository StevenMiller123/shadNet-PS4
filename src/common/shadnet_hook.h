#ifdef __cplusplus

template <auto f>
struct HookContinueWrapperImpl;

template <class ReturnType, class... Args, ReturnType (*func)(Args...)>
struct HookContinueWrapperImpl<func> {
    static ReturnType wrap(Args... args) {
        return func(args...);
    }
};

#define SHADNET_HOOK_CONTINUE(func, ...) HookContinueWrapperImpl<func>::wrap(__VA_ARGS__)

#endif