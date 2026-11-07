// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "video_core/nx2_cmdpage_diag.h"
#include "video_core/nx2_download_trace.h"
#include "video_core/nx2_env.h"
#include "video_core/nx2_profile.h"

namespace Tegra {
void ReloadDmaPusherSwitches();
namespace Engines {
void ReloadMaxwell3DSwitches();
}
} // namespace Tegra

namespace VideoCommon {

void ReloadEnvSwitches() {
    Nx2Profile::Reload();
    Tegra::Nx2CmdPageDiag::Reload();
    Nx2DownloadTrace::Reload();
    Tegra::ReloadDmaPusherSwitches();
    Tegra::Engines::ReloadMaxwell3DSwitches();
}

} // namespace VideoCommon
