#pragma once

namespace volume {

// Returns the current output volume as a 0-100 percentage (best-effort;
// returns 100 if it cannot be determined).
int get_volume_percent();

// Sets the output volume to the given 0-100 percentage.
void set_volume_percent(int percent);

} // namespace volume
