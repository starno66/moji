QT += widgets

CONFIG += c++17

INCLUDEPATH += include

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/gitmanager.cpp \
    src/filewatcher.cpp \
    src/chapterlistmodel.cpp \
    src/commitmodel.cpp \
    src/commitdetailwidget.cpp \
    src/environmentsetupdialog.cpp

HEADERS += \
    include/mainwindow.h \
    include/gitmanager.h \
    include/filewatcher.h \
    include/chapterlistmodel.h \
    include/commitmodel.h \
    include/commitdetailwidget.h \
    include/environmentsetupdialog.h

FORMS += \
    ui/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
