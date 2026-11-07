// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <functional>
#include <map>
#include <vector>

#include <fmt/format.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#endif

#include "video_core/nx2_census.h"

namespace VideoCommon::Nx2Census {

#ifdef _WIN32

u32 CurrentCore() {
    return GetCurrentProcessorNumber();
}

std::string Topology() {
    using Get = BOOL(WINAPI*)(PSYSTEM_CPU_SET_INFORMATION, ULONG, PULONG, HANDLE, ULONG);
    const auto get = reinterpret_cast<Get>(reinterpret_cast<void*>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetSystemCpuSetInformation")));
    if (get == nullptr) {
        return {};
    }
    ULONG length = 0;
    get(nullptr, 0, &length, GetCurrentProcess(), 0);
    if (length == 0) {
        return {};
    }
    std::vector<u8> buffer(length);
    if (!get(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data()), length, &length,
             GetCurrentProcess(), 0)) {
        return {};
    }
    std::map<int, std::vector<u32>, std::greater<>> classes;
    for (ULONG offset = 0; offset + sizeof(SYSTEM_CPU_SET_INFORMATION) <= length;) {
        const auto* info =
            reinterpret_cast<const SYSTEM_CPU_SET_INFORMATION*>(buffer.data() + offset);
        if (info->Size == 0) {
            break;
        }
        if (info->Type == CpuSetInformation) {
            classes[info->CpuSet.EfficiencyClass].push_back(info->CpuSet.LogicalProcessorIndex);
        }
        offset += info->Size;
    }
    std::string text;
    for (auto& [efficiency, cores] : classes) {
        std::sort(cores.begin(), cores.end());
        std::string ranges;
        for (size_t i = 0; i < cores.size();) {
            size_t j = i;
            while (j + 1 < cores.size() && cores[j + 1] == cores[j] + 1) {
                ++j;
            }
            if (!ranges.empty()) {
                ranges += ',';
            }
            ranges += i == j ? fmt::format("{}", cores[i])
                             : fmt::format("{}-{}", cores[i], cores[j]);
            i = j + 1;
        }
        text += fmt::format("{}class {}: {}", text.empty() ? "" : "; ", efficiency, ranges);
    }
    return text;
}

namespace {
struct ThreadState {
    std::string name;
    u64 cpu_100ns{};
};

std::string ThreadName(HANDLE thread) {
    using GetDescription = HRESULT(WINAPI*)(HANDLE, PWSTR*);
    static const auto get = reinterpret_cast<GetDescription>(reinterpret_cast<void*>(
        GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "GetThreadDescription")));
    if (get == nullptr) {
        return {};
    }
    PWSTR wide = nullptr;
    std::string name;
    if (SUCCEEDED(get(thread, &wide)) && wide != nullptr) {
        for (const wchar_t* c = wide; *c != 0 && name.size() < 40; ++c) {
            name.push_back(*c < 128 ? static_cast<char>(*c) : '?');
        }
        LocalFree(wide);
    }
    return name;
}

u64 FileTime(const FILETIME& time) {
    return (u64{time.dwHighDateTime} << 32) | time.dwLowDateTime;
}
} // Anonymous namespace

std::string BusiestThreads(u64 window_ns, size_t top) {
    static std::map<DWORD, ThreadState> known;
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return {};
    }
    struct Entry {
        std::string name;
        double ms_per_s;
    };
    std::vector<Entry> entries;
    const bool first = known.empty();
    const DWORD process = GetCurrentProcessId();
    THREADENTRY32 thread_entry{};
    thread_entry.dwSize = sizeof(thread_entry);
    for (BOOL more = Thread32First(snapshot, &thread_entry); more;
         more = Thread32Next(snapshot, &thread_entry)) {
        if (thread_entry.th32OwnerProcessID != process) {
            continue;
        }
        const HANDLE thread =
            OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, thread_entry.th32ThreadID);
        if (thread == nullptr) {
            continue;
        }
        FILETIME created{}, exited{}, kernel{}, user{};
        if (GetThreadTimes(thread, &created, &exited, &kernel, &user)) {
            const u64 cpu = FileTime(kernel) + FileTime(user);
            auto [it, inserted] = known.try_emplace(thread_entry.th32ThreadID);
            if (inserted || it->second.name.empty()) {
                it->second.name = ThreadName(thread);
            }
            if (!inserted && window_ns != 0 && cpu >= it->second.cpu_100ns) {
                const double ms = static_cast<double>(cpu - it->second.cpu_100ns) * 100.0 / 1e6;
                const double per_second = ms * 1e9 / static_cast<double>(window_ns);
                if (per_second >= 20.0) {
                    entries.push_back({it->second.name.empty()
                                           ? fmt::format("tid{}", thread_entry.th32ThreadID)
                                           : it->second.name,
                                       per_second});
                }
            }
            it->second.cpu_100ns = cpu;
        }
        CloseHandle(thread);
    }
    CloseHandle(snapshot);
    if (first) {
        return {};
    }
    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b) { return a.ms_per_s > b.ms_per_s; });
    std::string text;
    for (size_t i = 0; i < entries.size() && i < top; ++i) {
        text += fmt::format("{}{} {:.0f}", text.empty() ? "" : ", ", entries[i].name,
                            entries[i].ms_per_s);
    }
    return text;
}

#else

u32 CurrentCore() {
    return 0;
}

std::string Topology() {
    return {};
}

std::string BusiestThreads(u64, size_t) {
    return {};
}

#endif

} // namespace VideoCommon::Nx2Census
