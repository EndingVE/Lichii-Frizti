// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace VideoCommon {

/// Reads again the switches that were read at static initialisation, before main loaded
/// nx2_switches.txt (so the file never reached them). Called by the frontend right after
/// LoadNx2Switches; the real environment variable keeps working either way.
void ReloadEnvSwitches();

} // namespace VideoCommon
