LIBS     += -lOpengl32
QT       += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    camera.cpp \
    glwidget.cpp \
    light.cpp \
    main.cpp \
    mainwindow.cpp \
    material.cpp \
    trackball.cpp

HEADERS += \
    camera.h \
    glwidget.h \
    light.h \
    mainwindow.h \
    material.h \
    trackball.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    fgouraud.glsl \
    fnormal.glsl \
    fphong.glsl \
    ftexture.glsl \
    vgouraud.glsl \
    vnormal.glsl \
    vphong.glsl \
    vtexture.glsl

RESOURCES += \
    resources.qrc
