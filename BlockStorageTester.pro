# 注意：编译环境要使用 Qt 安装器所安装的qt环境，否则可能出错
QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# 包含 Crypto++ 头文件路径
# INCLUDEPATH += $$PWD/cryptopp

# 包含 homebrew 的 qt-postgreSQL 路径
LIBS += -L/opt/homebrew/opt/libpq/lib -lpq
# INCLUDEPATH += /opt/homebrew/opt/libpq/include


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 添加所有 .cpp 源码文件到项目中
SOURCES += \
    DatabaseService.cpp \
    main.cpp \
    mainwindow.cpp


HEADERS += \
    BlockInfo.h \
    DatabaseService.h \
    HashAlgorithm.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

RC_ICONS = icons/logo.ico
