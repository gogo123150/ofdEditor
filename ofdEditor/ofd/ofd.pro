#-------------------------------------------------
#
# Project created by QtCreator 2017-04-13T16:23:15
#
#-------------------------------------------------

# 这个文件是为了建立ofd的文件解析，读取相关的

QT       += widgets xml xmlpatterns

TARGET = ofd
TEMPLATE = lib

DEFINES += OFD_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += ofd.cpp

HEADERS += ofd.h\
        ofd_global.h \
    DataTypes/basic_datatype.h

DESTDIR = ../bin     # 生成文件在这
MOC_DIR = ./moc     # Q_OBJECT 类转换后的文件
RCC_DIR = ./rcc     # .qrc 文件转换后存放路径
OBJECTS_DIR += ./tmp   # .obj 文件存放路径

unix {
    target.path = /usr/lib
    INSTALLS += target
}

win32 {


}
