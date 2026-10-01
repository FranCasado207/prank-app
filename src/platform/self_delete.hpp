#pragma once

namespace platform {

// Deletes the currently running executable from disk. On POSIX this happens
// immediately; on Windows it is delegated to a short-lived detached helper
// since a running .exe cannot delete itself directly.
void self_delete();

} // namespace platform
