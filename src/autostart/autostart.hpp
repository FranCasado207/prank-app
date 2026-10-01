#pragma once

namespace autostart {

// Idempotent: safe to call on every normal startup.
void install();

// Idempotent: safe to call even if no entry exists.
void uninstall();

} // namespace autostart
