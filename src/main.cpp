#include "mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QSettings>
#include <QString>

QString makeStyleSheet(int scale)
{
    // scale: -1=小, 0=中, 1=大
    const int base   = 15 + scale;  // QWidget, QMenuBar, QMenu, QListView, QPushButton, QTextBrowser, QLineEdit, QGroupBox, QDialog
    const int title  = 16 + scale;  // 面板标题
    const int small  = 14 + scale;  // QComboBox, QStatusBar, QToolTip

    return QString(R"(
/* ====== 全局 ====== */
QMainWindow { background-color: #f5f7fa; }
QWidget { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
          font-size: %1px; color: #1e293b; }

/* ====== 菜单栏 ====== */
QMenuBar { background: #1e293b; color: #e2e8f0; padding: 2px 6px; font-size: %1px; }
QMenuBar::item { padding: 6px 10px; border-radius: 6px; }
QMenuBar::item:selected { background: rgba(255,255,255,0.1); }
QMenu { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px;
        padding: 4px; font-size: %1px; }
QMenu::item { padding: 6px 24px 6px 12px; border-radius: 6px; }
QMenu::item:selected { background: #f1f5f9; }
QMenu::separator { height: 1px; background: #e2e8f0; margin: 4px 6px; }

/* ====== 面板标题 ====== */
QLabel#chapterTitleLabel, QLabel#historyTitleLabel, QLabel#commitDetailTitle {
  font-size: %2px; font-weight: bold; color: #1e293b; padding: 2px 0; }
QLabel#filterLabel { font-size: %1px; font-weight: bold; color: #1e293b; }

/* ====== 列表视图 ====== */
QListView { background: transparent; border: none; padding: 2px; outline: none; font-size: %1px; }
QListView::item { padding: 8px 10px; border-radius: 8px; margin: 3px 0;
                  background: #f8fafc; border: none; }
QListView::item:hover { background: #f1f5f9; }
QListView::item:selected { background: #eef2ff; color: #4338ca; }
QListView::item:selected:!active { background: #eef2ff; color: #4338ca; }

/* ====== 下拉框 ====== */
QComboBox { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px;
            padding: 5px 10px; min-width: 60px; font-size: %3px; }
QComboBox#branchCombo { min-width: 200px; }
QComboBox:hover { background: #f1f5f9; }
QComboBox::drop-down { border: none; width: 18px; subcontrol-position: right center; }
QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #e2e8f0;
    border-radius: 8px; padding: 4px; outline: none; font-size: %3px; }
QComboBox QAbstractItemView::item { padding: 6px 10px; border-radius: 6px; min-height: 24px; }
QComboBox QAbstractItemView::item:hover { background: #f1f5f9; }

/* ====== 按钮 ====== */
QPushButton { background-color: #1e40af; color: #ffffff;
              border: none; border-radius: 8px;
              padding: 7px 18px; font-size: %1px; font-weight: 600; }
QPushButton:hover { background-color: #1e3a8a; }
QPushButton:pressed { background-color: #1e3a8a; }
QPushButton:disabled { background-color: #cbd5e1; color: #94a3b8; }

/* 次按钮（描边风格） */
QPushButton#renameChapterBtn, QPushButton#newBranchBtn,
QPushButton#mergeBtn {
  background-color: #ffffff; color: #475569;
  border: 1px solid #cbd5e1; font-weight: 600;
}
QPushButton#renameChapterBtn:hover, QPushButton#newBranchBtn:hover,
QPushButton#mergeBtn:hover {
  background-color: #f8fafc; border-color: #94a3b8;
}

/* 回退按钮 */
QPushButton#rollbackBtn {
  background-color: #ffffff; color: #1e40af;
  border: 1px solid #1e40af; font-weight: 600;
}
QPushButton#rollbackBtn:hover { background-color: #eef2ff; }
QPushButton#rollbackBtn:disabled {
  background-color: #ffffff; color: #94a3b8; border-color: #cbd5e1;
}

/* 删除按钮 */
QPushButton#deleteChapterBtn {
  background-color: #ffffff; color: #ef4444;
  border: 1px solid #fecaca; font-weight: 600;
}
QPushButton#deleteChapterBtn:hover {
  background-color: #fef2f2; border-color: #fca5a5;
}
QPushButton#deleteChapterBtn:disabled {
  background-color: #ffffff; color: #94a3b8; border-color: #cbd5e1;
}

/* ====== 文本浏览器 ====== */
QTextBrowser { background: #ffffff; border: none; border-radius: 10px;
              padding: 14px; font-size: %1px; }

/* ====== 分割器 ====== */
QSplitter::handle { background: transparent; }

/* ====== 状态栏 ====== */
QStatusBar { background: #ffffff; border-top: 1px solid #e2e8f0;
            color: #64748b; font-size: %3px; padding: 3px 10px; }

/* ====== 通用元素 ====== */
QToolTip { background: #1e293b; color: #ffffff; border-radius: 6px;
          padding: 5px 10px; font-size: %3px; }
QLineEdit { background: #ffffff; border: 1px solid #cbd5e1; border-radius: 8px;
           padding: 8px 12px; font-size: %1px; }
QLineEdit:focus { border-color: #4338ca; }
QGroupBox { font-weight: bold; border: none; border-radius: 10px;
           margin-top: 10px; padding: 16px 12px 12px 12px; font-size: %1px; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }

/* ====== 弹窗 ====== */
QDialog { min-width: 480px; }
QDialog QLabel { font-size: %1px; }
QDialog QLineEdit { font-size: %1px; padding: 8px 12px; }
QDialog QPushButton { font-size: %1px; padding: 8px 22px; }
)").arg(base).arg(title).arg(small);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("Moji");
    a.setApplicationName("Moji");

    // 让 Qt 使用系统原生字体 + 最佳渲染质量
    QFont sysFont = QApplication::font();
    sysFont.setStyleStrategy(QFont::PreferQuality);
    QApplication::setFont(sysFont);

    int scale = QSettings().value("ui/fontScale", 0).toInt();
    a.setWindowIcon(QIcon("moji.png"));
    a.setStyleSheet(makeStyleSheet(scale));
    MainWindow w;
    w.setWindowIcon(QIcon("moji.png"));
    w.show();
    return QApplication::exec();
}
