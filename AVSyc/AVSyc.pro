QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../ReadWriteLib/ReadWrite.cpp \
    ReadWrite.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ../ReadWriteLib/ReadWrite.h \
    ReadWrite.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../x64/release/ -lReadWriteLib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../x64/debug/ -lReadWriteLib

INCLUDEPATH += $$PWD/../x64/Release
DEPENDPATH += $$PWD/../x64/Release

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../x64/release/libReadWriteLib.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../x64/debug/libReadWriteLib.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../x64/release/ReadWriteLib.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../x64/debug/ReadWriteLib.lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../x64/release/ -lReadWriteLib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../x64/debug/ -lReadWriteLib

INCLUDEPATH += $$PWD/../x64/Debug
DEPENDPATH += $$PWD/../x64/Debug

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../x64/release/libReadWriteLib.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../x64/debug/libReadWriteLib.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../x64/release/ReadWriteLib.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../x64/debug/ReadWriteLib.lib
