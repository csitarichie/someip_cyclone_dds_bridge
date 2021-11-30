rapi_add_component(
    TARGET config_cxx
    SOURCE
        yaml_wrapper.cpp
    INTERFACE_HEADER
        yaml_wrapper.hpp
        config.hpp
        config_and_logger.hpp
    DEPENDS
        adstutil_cxx
        adstlog_cxx
        yaml
        fmt
)

#adst_add_test(
#    TEST yaml_wrapper
#    SOURCE
#        yaml_wrapper_test.cpp
#    DEPENDS
#        config_cxx
#)
#configure_file(test/yaml_wrapper/sequence.yml sequence.yml COPYONLY)
#configure_file(test/yaml_wrapper/test_message.yml test_message.yml COPYONLY)
#
#adst_add_test(
#    TEST logger_and_config
#    SOURCE
#        logger_config_test.cpp
#    DEPENDS
#        config_cxx
#)
#configure_file(test/logger_and_config/config.yml config.yml COPYONLY)
