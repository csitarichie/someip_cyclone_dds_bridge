#include <iostream>
#include "core/core.hpp"
#include "gw_system/gw_system.hpp"

static constexpr int EXPECTED_ERROR_CODE = 42;

int main()
{
  ConfigAndLogger confLog = {{[](const adst::common::Error& error) {
                                std::cerr << error.errorCodeName_.second << std::endl;
                                exit(EXPECTED_ERROR_CODE);
                              },
                              adst::common::Config::DEFAULT_NUMBER_OF_DISPATCHERS}};
    adst::ep::test_engine::core::Core rapiRuntime(confLog);
    std::cout << "rapi runtime created" << std::endl;
    rapiRuntime.init<GwSystemRoot>();
    std::cout << "rapi runtime initialised" << std::endl;
    rapiRuntime.run();
    std::cout << "rapi runtime finished" << std::endl;
}
