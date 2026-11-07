// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <string>

#include "common/common_types.h"

// EDEN_NX2_GPU_PROFILE: where the process's threads run. The profile log shows what the GPU
// threads do, not whether the host gave them a fast core or a free one.
namespace VideoCommon::Nx2Census {

/// Logical processor the calling thread runs on now (0 when unknown).
u32 CurrentCore();

/// Efficiency class of each logical processor, e.g. "class 1: 0-7; class 0: 8-23" (hybrid CPUs
/// rank their P cores above the E cores). Empty when the host does not say.
std::string Topology();

/// CPU time of each thread of the process since the previous call, as "name ms/s" for the
/// busiest ones, `window_ns` apart. Empty on the first call (nothing to compare with).
std::string BusiestThreads(u64 window_ns, size_t top);

} // namespace VideoCommon::Nx2Census
