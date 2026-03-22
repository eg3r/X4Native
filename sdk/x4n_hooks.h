// ---------------------------------------------------------------------------
// x4n_hooks.h — Function Hook API
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Usage:
//   int h = x4n::hook::before<&X4GameFunctions::Fn>(
//       [](x4n::hook::HookControl& ctl, ArgType& arg) { ... });
//   int h = x4n::hook::after<&X4GameFunctions::Fn>(
//       [](RetType& result, ArgType arg) { ... });
//   x4n::hook::remove(h);
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

namespace x4n {
namespace hook {

// HookControl -- passed to before-hook callbacks
struct HookControl {
    int& skip_original;

    template<typename T>
    void set_result(const T& val) {
        if (_ctx->result)
            *static_cast<T*>(_ctx->result) = val;
        skip_original = 1;
    }

    explicit HookControl(X4HookContext* ctx)
        : skip_original(ctx->skip_original), _ctx(ctx) {}

private:
    X4HookContext* _ctx;
};

namespace detail {

// --- Type trait helpers ---
template<typename T> struct member_type;
template<typename T, typename C> struct member_type<T C::*> { using type = T; };

template<typename T> struct fn_traits;
template<typename R, typename... A> struct fn_traits<R(*)(A...)> {
    using ret = R;
    using args = std::tuple<A...>;
    static constexpr bool is_void_ret = std::is_void_v<R>;
    static constexpr size_t arity = sizeof...(A);
};

// --- Per-member-pointer static state (one per hooked field) ---
template<auto MPtr>
struct state {
    static inline void* trampoline = nullptr;
    static inline const char* name = nullptr;
    static inline bool detour_installed = false;
};

// --- Function name lookup from X4GameFunctions member offset ---
struct FuncNameEntry { size_t offset; const char* name; };

#define X4_FUNC(ret, name, params) { offsetof(X4GameFunctions, name), #name },
inline const FuncNameEntry g_func_names[] = {
#include "x4_game_func_list.inc"
#include "x4_internal_func_list.inc"
};
#undef X4_FUNC

inline const char* name_from_offset(size_t off) {
    for (const auto& e : g_func_names)
        if (e.offset == off) return e.name;
    return nullptr;
}

template<auto MPtr>
size_t get_offset() {
    static const size_t off = [] {
        X4GameFunctions dummy;
        std::memset(&dummy, 0, sizeof(dummy));
        auto* member = &(dummy.*MPtr);
        return static_cast<size_t>(
            reinterpret_cast<char*>(member) - reinterpret_cast<char*>(&dummy));
    }();
    return off;
}

template<auto MPtr>
const char* get_name() {
    if (!state<MPtr>::name)
        state<MPtr>::name = name_from_offset(get_offset<MPtr>());
    return state<MPtr>::name;
}

template<auto MPtr>
bool install_detour(void* detour_fn) {
    if (state<MPtr>::detour_installed) return true;
    void* t = ::x4n::detail::g_api->_ensure_detour(get_name<MPtr>(), detour_fn);
    if (!t) return false;
    state<MPtr>::trampoline = t;
    state<MPtr>::detour_installed = true;
    return true;
}

// --- Typed detour functions (generated per unique member pointer) ---

// Non-void return
template<auto MPtr, typename Ret, typename... Args>
Ret typed_detour(Args... args) {
    void* arg_ptrs[] = { static_cast<void*>(&args)..., nullptr };
    Ret result{};

    X4HookContext ctx{};
    ctx.function_name = state<MPtr>::name;
    ctx.args = arg_ptrs;
    ctx.result = &result;
    ctx.skip_original = 0;

    ::x4n::detail::g_api->_run_before_hooks(&ctx);

    if (!ctx.skip_original) {
        auto orig = reinterpret_cast<Ret(*)(Args...)>(state<MPtr>::trampoline);
        result = [&]<size_t... I>(std::index_sequence<I...>) {
            return orig(*static_cast<std::remove_reference_t<Args>*>(arg_ptrs[I])...);
        }(std::index_sequence_for<Args...>{});
    }

    ctx.result = &result;
    ::x4n::detail::g_api->_run_after_hooks(&ctx);

    return result;
}

// Void return
template<auto MPtr, typename... Args>
void typed_detour_void(Args... args) {
    void* arg_ptrs[] = { static_cast<void*>(&args)..., nullptr };

    X4HookContext ctx{};
    ctx.function_name = state<MPtr>::name;
    ctx.args = arg_ptrs;
    ctx.result = nullptr;
    ctx.skip_original = 0;

    ::x4n::detail::g_api->_run_before_hooks(&ctx);

    if (!ctx.skip_original) {
        auto orig = reinterpret_cast<void(*)(Args...)>(state<MPtr>::trampoline);
        [&]<size_t... I>(std::index_sequence<I...>) {
            orig(*static_cast<std::remove_reference_t<Args>*>(arg_ptrs[I])...);
        }(std::index_sequence_for<Args...>{});
    }

    ::x4n::detail::g_api->_run_after_hooks(&ctx);
}

// Select the right detour based on return type
template<auto MPtr, typename FnPtr> struct detour_selector;
template<auto MPtr, typename Ret, typename... Args>
struct detour_selector<MPtr, Ret(*)(Args...)> {
    static void* get() {
        if constexpr (std::is_void_v<Ret>)
            return reinterpret_cast<void*>(&typed_detour_void<MPtr, Args...>);
        else
            return reinterpret_cast<void*>(&typed_detour<MPtr, Ret, Args...>);
    }
};

// --- Before-hook adapter: user void(HookControl&, Args&...) -> raw int(X4HookContext*) ---
template<typename UserFn, typename... Args>
struct before_adapter_impl {
    static int raw(X4HookContext* ctx) {
        auto fn = reinterpret_cast<UserFn>(ctx->userdata);
        HookControl ctl(ctx);
        [&]<size_t... I>(std::index_sequence<I...>) {
            fn(ctl, *static_cast<Args*>(ctx->args[I])...);
        }(std::make_index_sequence<sizeof...(Args)>{});
        return 0;
    }
};

template<typename UserFn, typename FnPtr> struct before_adapter;
template<typename UserFn, typename Ret, typename... Args>
struct before_adapter<UserFn, Ret(*)(Args...)> {
    static constexpr X4HookCallback get() {
        return &before_adapter_impl<UserFn, Args...>::raw;
    }
};

// --- After-hook adapter (non-void): user void(Ret&, Args...) -> raw int(X4HookContext*) ---
template<typename UserFn, typename Ret, typename... Args>
struct after_adapter_nonvoid {
    static int raw(X4HookContext* ctx) {
        auto fn = reinterpret_cast<UserFn>(ctx->userdata);
        Ret& result = *static_cast<Ret*>(ctx->result);
        [&]<size_t... I>(std::index_sequence<I...>) {
            fn(result, *static_cast<Args*>(ctx->args[I])...);
        }(std::make_index_sequence<sizeof...(Args)>{});
        return 0;
    }
};

// --- After-hook adapter (void): user void(Args...) -> raw int(X4HookContext*) ---
template<typename UserFn, typename... Args>
struct after_adapter_void {
    static int raw(X4HookContext* ctx) {
        auto fn = reinterpret_cast<UserFn>(ctx->userdata);
        [&]<size_t... I>(std::index_sequence<I...>) {
            fn(*static_cast<Args*>(ctx->args[I])...);
        }(std::make_index_sequence<sizeof...(Args)>{});
        return 0;
    }
};

template<typename UserFn, typename FnPtr> struct after_adapter;
template<typename UserFn, typename Ret, typename... Args>
struct after_adapter<UserFn, Ret(*)(Args...)> {
    static constexpr X4HookCallback get() {
        if constexpr (std::is_void_v<Ret>)
            return &after_adapter_void<UserFn, Args...>::raw;
        else
            return &after_adapter_nonvoid<UserFn, Ret, Args...>::raw;
    }
};

} // namespace detail

// ---------------------------------------------------------------------------
// Public hook API
// ---------------------------------------------------------------------------

/// Install a before-hook on a game function. Returns hook ID (>0) or -1.
/// Callback: void(HookControl&, ArgTypes&...)
template<auto MemberPtr, typename BeforeFn>
int before(BeforeFn fn) {
    using MemberType = typename detail::member_type<decltype(MemberPtr)>::type;
    auto fp = +fn;  // convert stateless lambda to function pointer
    using FpType = decltype(fp);

    void* detour = detail::detour_selector<MemberPtr, MemberType>::get();
    if (!detail::install_detour<MemberPtr>(detour)) return -1;

    X4HookCallback raw_cb = detail::before_adapter<FpType, MemberType>::get();

    return ::x4n::detail::g_api->hook_before(
        detail::get_name<MemberPtr>(),
        raw_cb,
        reinterpret_cast<void*>(fp),
        ::x4n::detail::g_api);
}

/// Install an after-hook on a game function. Returns hook ID (>0) or -1.
/// Callback: void(RetType&, ArgTypes...) for non-void, void(ArgTypes...) for void.
template<auto MemberPtr, typename AfterFn>
int after(AfterFn fn) {
    using MemberType = typename detail::member_type<decltype(MemberPtr)>::type;
    auto fp = +fn;
    using FpType = decltype(fp);

    void* detour = detail::detour_selector<MemberPtr, MemberType>::get();
    if (!detail::install_detour<MemberPtr>(detour)) return -1;

    X4HookCallback raw_cb = detail::after_adapter<FpType, MemberType>::get();

    return ::x4n::detail::g_api->hook_after(
        detail::get_name<MemberPtr>(),
        raw_cb,
        reinterpret_cast<void*>(fp),
        ::x4n::detail::g_api);
}

/// Remove a hook by ID.
inline void remove(int hook_id) {
    ::x4n::detail::g_api->unhook(hook_id);
}

} // namespace hook
} // namespace x4n

