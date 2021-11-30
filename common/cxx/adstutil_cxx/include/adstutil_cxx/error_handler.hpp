#pragma once

#include <functional>

namespace adst::common {

/**
 * Used as input structure in the core for the OnError callback.
 */
struct Error
{
    using ErrorCodeName          = std::pair<int, std::string>;
    ErrorCodeName errorCodeName_ = {1, "UNKNOWN"};
};

/**
 * Default error callback implementation
 * @param error occurred error
 */
void onErrorFunc(const Error& error);

/**
 * Gets called by the core in case of a unrecoverable error.
 * This function shall never return (instead, it typically terminates the program).
 */
using OnErrorCallBack = std::function<void(const Error&)>; // this is no return.

} // namespace adst::common
