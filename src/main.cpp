#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);   // 创建 Qt 应用对象（必须）
    MainWindow w;                 // 创建主窗口
    w.show();                     // 显示窗口
    return QApplication::exec();  // 进入事件循环，等待用户操作
}
