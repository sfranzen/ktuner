set(ktuner_SRCS
    main.cpp
    mainwindow.cpp
    ktuner.cpp
    analyzer.cpp
    analysisresult.cpp
    note.cpp
    pitchtable.cpp
    config/ktunerconfigdialog.cpp
    ui/ui.qrc
    ../fonts/fonts.qrc
)

configure_file(version.h.in version.h @ONLY)
kconfig_add_kcfg_files(ktuner_SRCS config/ktunerconfig.kcfgc)
ki18n_wrap_ui(ktuner_SRCS config/audiosettings.ui)

add_executable(ktuner ${ktuner_SRCS})

target_link_libraries(ktuner
                      Qt5::Core
                      Qt5::Multimedia
                      Qt5::QuickWidgets
                      Qt5::Qml
                      Qt5::Charts
                      KF5::Plasma
                      KF5::Declarative
                      KF5::XmlGui
                      KF5::I18n
                      KF5::ConfigCore
                      KF5::ConfigGui
                      fftw3
)

install(TARGETS ktuner ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ktunerui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/ktuner)
install(FILES config/ktuner.kcfg DESTINATION ${KCFG_INSTALL_DIR})
