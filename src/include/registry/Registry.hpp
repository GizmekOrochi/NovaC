#pragma once

namespace novac::registry {

enum class DuplicatePolicy {
    Error,
    Replace,
    Ignore
};

enum class RegisterStatus {
    Inserted,
    Replaced,
    Ignored
};

} // namespace novac::registry