#pragma once

#include <filesystem>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#ifdef IN
#pragma push_macro("IN")
#undef IN
#define CHOWDSP_SC_PORTS_RESTORE_IN 1
#endif
#ifdef OUT
#pragma push_macro("OUT")
#undef OUT
#define CHOWDSP_SC_PORTS_RESTORE_OUT 1
#endif
#include <windows.h>
#ifdef CHOWDSP_SC_PORTS_RESTORE_IN
#pragma pop_macro("IN")
#undef CHOWDSP_SC_PORTS_RESTORE_IN
#endif
#ifdef CHOWDSP_SC_PORTS_RESTORE_OUT
#pragma pop_macro("OUT")
#undef CHOWDSP_SC_PORTS_RESTORE_OUT
#endif
#elif defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#endif

namespace sharedpaths
{
inline std::filesystem::path moduleDirectory(const void* symbolAddress)
{
#if defined(_WIN32)
    HMODULE moduleHandle = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast<LPCSTR>(symbolAddress),
                           &moduleHandle)
        != 0) {
        char pathBuffer[MAX_PATH] {};
        const auto length = GetModuleFileNameA(moduleHandle, pathBuffer, MAX_PATH);
        if (length > 0)
            return std::filesystem::path(pathBuffer).parent_path();
    }
#elif defined(__APPLE__) || defined(__linux__)
    Dl_info info {};
    if (dladdr(symbolAddress, &info) != 0 && info.dli_fname != nullptr)
        return std::filesystem::path(info.dli_fname).parent_path();
#endif

    return std::filesystem::current_path();
}
} // namespace sharedpaths
