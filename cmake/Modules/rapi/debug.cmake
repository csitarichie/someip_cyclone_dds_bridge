# Used internally for verbose debug
macro(rapi_debug_log_)
    if(RAPI_CMAKE_DEBUG)
        message(STATUS "DBG: ${ARGN}")
    endif()
endmacro()

macro(rapi_message_ mode)
    message(${mode} "RAPI: ${ARGN}")
endmacro()

macro(rapi_message_ts_ MSG_)
    string(TIMESTAMP _TS "%Y-%m-%dT%H:%M:%S")
    message("${_TS} ${MSG_}")
endmacro()

if (RAPI_CMAKE_DEBUG_SCRIPTS)
    set(RAPI_DEBUG_COMMAND_LOG_SCRIPT "cat")
else ()
    set(RAPI_DEBUG_COMMAND_LOG_SCRIPT "true")
endif ()
