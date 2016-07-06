# KTuner
KTuner aims to be a fast and accurate instrument tuner with an abundantly clear
user interface.

The intent of this project is to teach myself how to write an application using
KDE Frameworks 5, Qt5 and QML, while working on something of practical use. The
main GUI elements are written in QML, embedded as QQuickWidgets in a (QWidget-
based) KXmlGui window.

## Dependencies
* Qt 5.7.0:
  * Core
  * Multimedia
  * Widgets
  * QuickWidgets
  * Qml
  * Charts
* KF5:
  * Declarative
  * XmlGui
  * I18n
* FFTW3

## Build Instructions
```
$ git clone https://github.com/sfranzen/ktuner.git
$ cd ktuner
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=/usr
$ make
$ sudo make install
```
