configure_file(${CMAKE_CURRENT_SOURCE_DIR}/main.py.in ${F1X_TOOLS_DIR}/f1x-bear)

include(GNUInstallDirs)
install(PROGRAMS ${F1X_TOOLS_DIR}/f1x-bear
    DESTINATION ${CMAKE_INSTALL_BINDIR})
