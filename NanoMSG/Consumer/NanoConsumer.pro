QT -= core
QT -= gui

SOURCES += main.cpp


CONFIG += c++11

LIBS +=  -lnanomsg


HEADERS += \
    BlockingConcurrentQueue.hpp \
    ConcurrentQueue.hpp

