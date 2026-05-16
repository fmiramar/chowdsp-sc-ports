#pragma once

#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
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
