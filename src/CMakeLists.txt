set(ktuner_SRCS
    main.cpp
    mainwindow.cpp
    ktuner.cpp
    analyzer.cpp
    analysisresult.cpp
    note.cpp
    pitchtable.cpp
    ui/ui.qrc
    ../res/resources.qrc
)

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
                      fftw3
)

install(TARGETS ktuner ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ktunerui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/ktuner)
