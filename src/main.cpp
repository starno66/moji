#include "mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>

static const char *GLOBAL_STYLE = R"(
QMainWindow { background-color: #f6f8fa; }
QWidget { font-size: 14px; color: #1f2328; }

QMenuBar { background: #24292f; color: #ffffff; padding: 1px 6px; font-size: 14px; }
QMenuBar::item { padding: 3px 8px; border-radius: 4px; }
QMenuBar::item:selected { background: #3a3f47; }
QMenu { background: #ffffff; border: 1px solid #d0d7de; border-radius: 6px;
        padding: 4px; font-size: 14px; }
QMenu::item { padding: 5px 24px 5px 12px; border-radius: 4px; }
QMenu::item:selected { background: #f3f4f6; }
QMenu::separator { height: 1px; background: #d0d7de; margin: 3px 6px; }

QLabel#chapterTitleLabel, QLabel#historyTitleLabel, QLabel#commitDetailTitle {
  font-size: 15px; font-weight: bold; color: #1f2328; padding: 1px 0; }
QLabel#filterLabel { font-size: 14px; font-weight: bold; color: #1f2328; }

QListView { background: #ffffff; border: none; border-radius: 6px;
            padding: 4px; outline: none; font-size: 15px; }
QListView::item { padding: 5px 10px; border-radius: 4px; }
QListView::item:hover { background: #f3f4f6; }
QListView::item:selected { background: #ddf4ff; color: #0969da; }
QListView::item:selected:!active { background: #eaeef2; color: #1f2328; }

QComboBox { background: #f6f8fa; border: none; border-radius: 6px;
            padding: 3px 10px; min-width: 60px; font-size: 14px; }
QComboBox#branchCombo { min-width: 200px; }
QComboBox:hover { background: #eaeef2; }
QComboBox::drop-down { border: none; width: 18px; }
QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #d0d7de;
    border-radius: 6px; padding: 4px; outline: none; font-size: 14px; }
QComboBox QAbstractItemView::item { padding: 5px 10px; border-radius: 4px; min-height: 22px; }
QComboBox QAbstractItemView::item:hover { background: #f3f4f6; }

QPushButton { background-color: #0969da; color: #ffffff;
              border: none; border-radius: 6px;
              padding: 4px 14px; font-size: 14px; font-weight: bold; }
QPushButton:hover { background-color: #0550ae; }
QPushButton:pressed { background-color: #033d8b; }
QPushButton:disabled { background-color: #96c5f7; color: rgba(255,255,255,200); }

QTextBrowser { background: #ffffff; border: none; border-radius: 6px;
              padding: 12px; font-size: 15px; }

QSplitter::handle { background: transparent; }

QStatusBar { background: #ffffff; border-top: 1px solid #eaeef2;
            color: #656d76; font-size: 13px; padding: 2px 8px; }
QToolTip { background: #24292f; color: #ffffff; border-radius: 4px;
          padding: 4px 8px; font-size: 13px; }
QLineEdit { background: #ffffff; border: none; border-radius: 6px;
           padding: 4px 10px; font-size: 14px; }
QLineEdit:focus { background: #ffffff; }
QGroupBox { font-weight: bold; border: none; border-radius: 6px;
           margin-top: 10px; padding: 14px 10px 10px 10px; font-size: 14px; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }

QDialog { min-width: 500px; min-height: 200px; }
QDialog QLabel { font-size: 14px; }
QDialog QLineEdit { font-size: 14px; padding: 6px 12px; }
QDialog QPushButton { font-size: 14px; padding: 6px 22px; }
)";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("Moji");
    a.setApplicationName("Moji");

    // 让 Qt 使用系统原生字体 + 最佳渲染质量
    QFont sysFont = QApplication::font();
    sysFont.setStyleStrategy(QFont::PreferQuality);
    QApplication::setFont(sysFont);

    a.setWindowIcon(QIcon("moji.png"));
    a.setStyleSheet(QLatin1String(GLOBAL_STYLE));
    MainWindow w;
    w.setWindowIcon(QIcon("moji.png"));
    w.show();
    return QApplication::exec();
}
