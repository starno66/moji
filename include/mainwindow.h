#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// 前置声明，避免头文件循环依赖
class GitManager;
class FileWatcher;
class ChapterListModel;
class CommitModel;
class CommitDetailWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // ====== 文件菜单 ======
    void onOpenWorkspace();          // 打开/切换工作区

    // ====== 章节面板 ======
    void onChapterSelected(const QModelIndex &index);
    void onCreateChapter();
    void onRenameChapter();
    void onDeleteChapter();

    // ====== Commit 面板 ======
    void onCommitSelected(const QModelIndex &index);
    void onFilterChanged(int comboIndex);
    void onRollbackToCommit();
    void onCommitChanges();

    // ====== 文件监控 ======
    void onExternalFileChanged(const QString &path);

private:
    // UI 初始化
    void initModels();           // 创建 Model 并绑定到 View
    void initDetailWidget();     // 绑定 CommitDetailWidget 到 UI 的 label
    void connectSignals();       // 连接所有信号槽
    void setupMenus();           // 代码创建菜单栏

    // 数据刷新
    void refreshChapters();      // 重新扫描章节文件夹
    void refreshCommits();       // 重新加载 commit 列表
    void updateStatusBar();      // 更新状态栏信息

    // 核心操作
    bool openWorkspace(const QString &path);

    // ---------- Designer 生成的 UI 指针 ----------
    Ui::MainWindow *ui;

    // ---------- 核心组件 ----------
    GitManager   *m_gitManager;    // Git 操作引擎
    FileWatcher  *m_fileWatcher;   // 文件系统监控

    // ---------- 数据模型 ----------
    ChapterListModel *m_chapterModel;
    CommitModel      *m_commitModel;

    // ---------- 详情控制器 ----------
    CommitDetailWidget *m_commitDetail;

    // ---------- 当前状态 ----------
    QString m_currentChapter;   // 当前选中的章节名（空=未选中）
    QString m_workspacePath;    // 工作区路径（空=未打开）
};

#endif // MAINWINDOW_H
