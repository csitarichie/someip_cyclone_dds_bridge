#include "adstutil_cxx/error_handler.hpp"

#include <iostream>

namespace adst::common {

void onErrorFunc(const Error& error)
{
    std::cout << "[FATAL] " << error.errorCodeName_.second << std::endl;
    exit(error.errorCodeName_.first);
};

} // namespace adst::common
