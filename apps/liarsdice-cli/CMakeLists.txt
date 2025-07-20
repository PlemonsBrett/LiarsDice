add_executable(liarsdice-cli main.cpp)

target_link_libraries(liarsdice-cli
    PRIVATE
        liarsdice::core
        liarsdice::warnings
)

target_compile_features(liarsdice-cli PRIVATE cxx_std_23)

# Enable IPO/LTO for release builds
enable_ipo(liarsdice-cli)

# Copy assets to build directory
add_custom_command(TARGET liarsdice-cli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:liarsdice-cli>/assets
)

# Installation
if(LIARSDICE_INSTALL)
    install(TARGETS liarsdice-cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets
        DESTINATION ${CMAKE_INSTALL_DATADIR}/liarsdice
    )
endif()