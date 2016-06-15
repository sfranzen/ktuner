set(ktuner_SRCS
    main.cpp
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
                      Qt5::Widgets
                      Qt5::Qml
                      KF5::Plasma
                      KF5::Declarative
                      fftw3
)

install(TARGETS ktuner DESTINATION ${KDE_INSTALL_BINDIR})

