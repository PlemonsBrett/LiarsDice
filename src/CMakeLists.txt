# Create the core library
add_library(liarsdice_core
    liarsdice/core/dice.cpp
    liarsdice/core/dice_impl.cpp
    liarsdice/core/player.cpp
    liarsdice/core/player_impl.cpp
    liarsdice/core/game.cpp
    liarsdice/core/game_impl.cpp
    liarsdice/di/service_container.cpp
)

# Add logging sources if enabled
if(LIARSDICE_ENABLE_LOGGING)
    target_sources(liarsdice_core PRIVATE
        liarsdice/logging/logger_manager.cpp
    )
endif()

# Add configuration sources if enabled
if(LIARSDICE_ENABLE_CONFIG)
    target_sources(liarsdice_core PRIVATE
        liarsdice/config/config_value.cpp
        liarsdice/config/config_manager.cpp
        liarsdice/config/config_sources.cpp
        liarsdice/config/game_config.cpp
    )
    
    # Add compile definition for nlohmann_json availability
    if(TARGET nlohmann_json::nlohmann_json)
        target_compile_definitions(liarsdice_core PRIVATE LIARSDICE_HAS_NLOHMANN_JSON)
        target_link_libraries(liarsdice_core PRIVATE nlohmann_json::nlohmann_json)
    endif()
    
    target_compile_definitions(liarsdice_core PRIVATE LIARSDICE_ENABLE_CONFIG)
endif()

add_library(liarsdice::core ALIAS liarsdice_core)

target_include_directories(liarsdice_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(liarsdice_core PUBLIC cxx_std_23)

# Link common warnings
target_link_libraries(liarsdice_core
    PRIVATE
        liarsdice::warnings
)

# Setup logging
setup_logging(liarsdice_core)

# Enable IPO/LTO for release builds
enable_ipo(liarsdice_core)

# Installation
if(LIARSDICE_INSTALL)
    install(TARGETS liarsdice_core
        EXPORT LiarsDiceTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()