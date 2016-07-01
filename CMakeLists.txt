cmake_minimum_required(VERSION 2.8.12)

project(ktuner)
set(CMAKE_AUTORCC ON)
find_package(ECM 1.4.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMMarkAsTest)
include(ECMMarkNonGuiExecutable)
include(FeatureSummary)
include(CheckIncludeFiles)

find_package(KF5 REQUIRED COMPONENTS
    Plasma
    Declarative
    XmlGui
    I18n
)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Multimedia
    Widgets
    QuickWidgets
    Qml
    Charts
)

find_library(FFTW REQUIRED)

add_subdirectory(src)
