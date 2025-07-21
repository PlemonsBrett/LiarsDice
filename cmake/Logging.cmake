# Logging.cmake
# CMake module for configuring comprehensive logging with spdlog

# Configure logging compile definitions
function(configure_logging_definitions target)
    if(LIARSDICE_ENABLE_LOGGING)
        target_compile_definitions(${target} PRIVATE LIARSDICE_ENABLE_LOGGING)
        
        # Map CMake log level to spdlog level constants
        string(TOUPPER "${LIARSDICE_LOG_LEVEL}" LOG_LEVEL_UPPER)
        set(LOG_LEVEL_MAP_TRACE "SPDLOG_LEVEL_TRACE")
        set(LOG_LEVEL_MAP_DEBUG "SPDLOG_LEVEL_DEBUG")
        set(LOG_LEVEL_MAP_INFO "SPDLOG_LEVEL_INFO")
        set(LOG_LEVEL_MAP_WARN "SPDLOG_LEVEL_WARN")
        set(LOG_LEVEL_MAP_ERROR "SPDLOG_LEVEL_ERROR")
        set(LOG_LEVEL_MAP_CRITICAL "SPDLOG_LEVEL_CRITICAL")
        set(LOG_LEVEL_MAP_OFF "SPDLOG_LEVEL_OFF")
        
        if(DEFINED LOG_LEVEL_MAP_${LOG_LEVEL_UPPER})
            target_compile_definitions(${target} PRIVATE 
                SPDLOG_ACTIVE_LEVEL=${LOG_LEVEL_MAP_${LOG_LEVEL_UPPER}})
            message(STATUS "Setting ${target} log level to ${LOG_LEVEL_UPPER}")
        else()
            target_compile_definitions(${target} PRIVATE 
                SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
            message(WARNING "Unknown log level '${LIARSDICE_LOG_LEVEL}', defaulting to INFO")
        endif()
        
        # Enable additional spdlog features
        target_compile_definitions(${target} PRIVATE
            SPDLOG_NO_EXCEPTIONS         # Disable exceptions in spdlog
        )
        
        # Performance optimizations
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            target_compile_definitions(${target} PRIVATE
                SPDLOG_NO_NAME           # Disable logger names in release
                SPDLOG_NO_REGISTRY       # Disable global registry in release
            )
        endif()
    else()
        target_compile_definitions(${target} PRIVATE 
            SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF
            LIARSDICE_LOGGING_DISABLED)
    endif()
endfunction()

# Link logging libraries to target
function(link_logging_libraries target)
    if(LIARSDICE_ENABLE_LOGGING)
        if(TARGET spdlog::spdlog)
            target_link_libraries(${target} PRIVATE spdlog::spdlog)
            message(STATUS "Linked ${target} with spdlog::spdlog")
        else()
            message(WARNING "spdlog::spdlog target not found for ${target}")
        endif()
        
        
        if(TARGET nlohmann_json::nlohmann_json)
            target_link_libraries(${target} PRIVATE nlohmann_json::nlohmann_json)
            message(STATUS "Linked ${target} with nlohmann_json::nlohmann_json")
        else()
            message(WARNING "nlohmann_json::nlohmann_json target not found for ${target}")
        endif()
    endif()
endfunction()

# Complete logging setup function
function(setup_logging target)
    configure_logging_definitions(${target})
    link_logging_libraries(${target})
endfunction()

# Print logging configuration status
function(print_logging_status)
    message(STATUS "")
    message(STATUS "Logging Configuration:")
    message(STATUS "  Logging enabled: ${LIARSDICE_ENABLE_LOGGING}")
    if(LIARSDICE_ENABLE_LOGGING)
        message(STATUS "  Compile-time log level: ${LIARSDICE_LOG_LEVEL}")
        message(STATUS "  spdlog found: ${spdlog_FOUND}")
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            message(STATUS "  Release optimizations: enabled")
        endif()
    endif()
    message(STATUS "")
endfunction()