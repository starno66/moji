#include "mainwindow.h"
#include "ui_mainwindow.h"          // Designer 生成的 UI 头文件
#include "gitmanager.h"
#include "filewatcher.h"
#include "chapterlistmodel.h"
#include "commitmodel.h"
#include "commitdetailwidget.h"

#include <QFileDialog>      // 选择文件夹对话框
#include <QInputDialog>     // 输入文本对话框
#include <QMessageBox>      // 消息提示框
#include <QSettings>        // 持久化存储（记住上次工作区）
#include <QDir>
#include <QFileInfo>
#include <QMenuBar>
#include <QVBoxLayout>

/*
 * ==================== 构造函数 ====================
 */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_gitManager(new GitManager(this))
    , m_fileWatcher(new FileWatcher(this))
    , m_chapterModel(new ChapterListModel(this))
    , m_commitModel(new CommitModel(this))
    , m_commitDetail(nullptr)
{
    // 第1步：加载 Designer 设计的 UI
    ui->setupUi(this);

    // 第2步：设置垂直分割器初始比例
    ui->historySplitter->setSizes({300, 200});

    // 第3步：初始化模型和按钮状态
    initModels();
    initDetailWidget();

    // 第4步：用代码创建菜单栏
    setupMenus();

    // 第5步：连接所有信号槽
    connectSignals();

    // 第6步：蓝色按钮风格
    setStyleSheet(QString(
        "QPushButton {"
        "  background-color: #1A5CB5;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #154A91; }"
        "QPushButton:pressed { background-color: #10386E; }"
        "QPushButton:disabled {"
        "  background-color: #7FA8D4;"
        "  color: rgba(255,255,255,180);"
        "}"
    ));

    // 第7步：恢复上次工作区
    QSettings settings;
    QString lastPath = settings.value("workspace/lastPath").toString();
    if (!lastPath.isEmpty() && QDir(lastPath + "/.git").exists()) {
        openWorkspace(lastPath);
    }

    // 第6步：恢复窗口大小
    restoreGeometry(settings.value("window/geometry").toByteArray());

    // 第7步：初始未打开工作区，禁用部分按钮
    ui->newChapterBtn->setEnabled(false);
    ui->renameChapterBtn->setEnabled(false);
    ui->deleteChapterBtn->setEnabled(false);
    ui->commitBtn->setEnabled(false);
}

/*
 * ==================== 析构函数 ====================
 */

MainWindow::~MainWindow()
{
    // 保存窗口大小和工作区路径
    QSettings settings;
    settings.setValue("window/geometry", saveGeometry());
    if (!m_workspacePath.isEmpty())
        settings.setValue("workspace/lastPath", m_workspacePath);

    delete ui;
}

// ==================== 初始化 ==================== //

void MainWindow::initModels()
{
    /*
     * QListView::setModel():
     * 把 Model 绑定到 View。此后 Model 数据变化时，View 自动刷新。
     * 这是 Qt 模型/视图架构的核心。
     */
    ui->chapterListView->setModel(m_chapterModel);
    ui->commitListView->setModel(m_commitModel);
}

void MainWindow::initDetailWidget()
{
    m_commitDetail = new CommitDetailWidget(this);
    m_commitDetail->bind(ui->commitDetailBrowser);
}

void MainWindow::setupMenus()
{
    // ---- 文件菜单 ----
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");

    QAction *openAction = fileMenu->addAction("打开工作区(&O)");
    openAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenWorkspace);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("退出(&Q)");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // ---- 章节菜单 ----
    QMenu *chapterMenu = menuBar()->addMenu("章节(&C)");

    QAction *newAction = chapterMenu->addAction("新建章节(&N)");
    newAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(newAction, &QAction::triggered, this, &MainWindow::onCreateChapter);

    QAction *renameAction = chapterMenu->addAction("重命名章节(&R)");
    connect(renameAction, &QAction::triggered, this, &MainWindow::onRenameChapter);

    QAction *deleteAction = chapterMenu->addAction("删除章节(&D)");
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteChapter);
}

void MainWindow::connectSignals()
{
    // 点击章节 → 切换当前章节
    connect(ui->chapterListView, &QListView::clicked,
            this, &MainWindow::onChapterSelected);

    // 点击 commit → 显示详情
    connect(ui->commitListView, &QListView::clicked,
            this, &MainWindow::onCommitSelected);

    // 切换筛选下拉框 → 重新加载对应章节的 commit
    connect(ui->filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFilterChanged);

    // 章节按钮
    connect(ui->newChapterBtn, &QPushButton::clicked,
            this, &MainWindow::onCreateChapter);
    connect(ui->renameChapterBtn, &QPushButton::clicked,
            this, &MainWindow::onRenameChapter);
    connect(ui->deleteChapterBtn, &QPushButton::clicked,
            this, &MainWindow::onDeleteChapter);

    // Commit 按钮
    connect(ui->commitBtn, &QPushButton::clicked,
            this, &MainWindow::onCommitChanges);
    connect(ui->rollbackBtn, &QPushButton::clicked,
            this, &MainWindow::onRollbackToCommit);

    // Git 错误 → 状态栏显示
    connect(m_gitManager, &GitManager::errorOccurred,
            this, [this](const QString &op, const QString &detail) {
        statusBar()->showMessage(
            QString("错误 [%1]: %2").arg(op, detail), 8000);
    });

    // 文件变更 → 更新状态栏
    connect(m_fileWatcher, &FileWatcher::fileChanged,
            this, &MainWindow::onExternalFileChanged);
}

// ==================== 文件菜单 ==================== //

void MainWindow::onOpenWorkspace()
{
    // 弹出文件夹选择对话框
    QString path = QFileDialog::getExistingDirectory(this, "选择工作区目录");
    if (path.isEmpty())
        return;

    openWorkspace(path);
}

bool MainWindow::openWorkspace(const QString &path)
{
    m_gitManager->setWorkspacePath(path);

    // 如果不是 git 仓库，询问是否初始化
    if (!m_gitManager->isRepo()) {
        auto answer = QMessageBox::question(this, "初始化仓库",
            "该目录不是 Git 仓库，是否初始化？",
            QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            if (!m_gitManager->initRepo()) {
                QMessageBox::critical(this, "错误", "Git 仓库初始化失败");
                return false;
            }
        } else {
            return false;
        }
    }

    m_workspacePath = path;
    m_currentChapter.clear();
    m_commitDetail->clear();

    // 启动文件监控
    m_fileWatcher->watchWorkspace(path);

    // 刷新 UI
    refreshChapters();
    refreshCommits();
    updateStatusBar();

    // 启用按钮
    ui->newChapterBtn->setEnabled(true);
    ui->commitBtn->setEnabled(true);

    setWindowTitle(QString("墨迹 - %1").arg(path));
    statusBar()->showMessage(QString("已打开: %1").arg(path), 3000);

    // 持久化
    QSettings settings;
    settings.setValue("workspace/lastPath", path);

    return true;
}

// ==================== 章节面板 ==================== //

void MainWindow::onChapterSelected(const QModelIndex &index)
{
    m_currentChapter = m_chapterModel->chapterAt(index.row());

    // 让筛选下拉框同步到当前选中的章节
    int filterIdx = ui->filterCombo->findText(m_currentChapter);
    if (filterIdx >= 0)
        ui->filterCombo->setCurrentIndex(filterIdx);

    // 刷新 commit 列表（自动按选中章节筛选）
    refreshCommits();

    // 启用/禁用章节编辑按钮
    ui->renameChapterBtn->setEnabled(true);
    ui->deleteChapterBtn->setEnabled(true);

    updateStatusBar();
}

void MainWindow::onCreateChapter()
{
    if (m_workspacePath.isEmpty()) return;

    // 弹出输入框让用户输入章节名称
    bool ok;
    QString name = QInputDialog::getText(this, "新建章节",
        "章节名称:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    // 验证名称合法性
    if (name.contains('/') || name.contains('\\')) {
        QMessageBox::warning(this, "无效名称", "章节名称不能包含路径分隔符");
        return;
    }
    if (QDir(m_workspacePath).exists(name)) {
        QMessageBox::warning(this, "已存在", "该章节已存在");
        return;
    }

    // 创建文件夹
    QDir dir(m_workspacePath);
    dir.mkdir(name);

    // 创建 .gitkeep 文件（git 不跟踪空文件夹，所以加个占位文件）
    QFile placeholder(m_workspacePath + "/" + name + "/.gitkeep");
    if (!placeholder.open(QIODevice::WriteOnly))
        return;
    placeholder.close();

    // 提交
    m_gitManager->stageAndCommit(QString("%1: 新建章节").arg(name));

    refreshChapters();
    refreshCommits();
    updateStatusBar();
}

void MainWindow::onRenameChapter()
{
    if (m_currentChapter.isEmpty()) return;

    bool ok;
    QString newName = QInputDialog::getText(this, "重命名章节",
        QString("将 \"%1\" 重命名为:").arg(m_currentChapter),
        QLineEdit::Normal, m_currentChapter, &ok);
    if (!ok || newName.isEmpty() || newName == m_currentChapter) return;

    if (newName.contains('/') || newName.contains('\\')) {
        QMessageBox::warning(this, "无效名称", "章节名称不能包含路径分隔符");
        return;
    }
    if (QDir(m_workspacePath).exists(newName)) {
        QMessageBox::warning(this, "已存在", "目标名称已存在");
        return;
    }

    if (!m_gitManager->moveFolder(m_currentChapter, newName)) {
        QMessageBox::critical(this, "错误", "重命名失败");
        return;
    }

    m_gitManager->commit(QString("%1: 重命名自 %2").arg(newName, m_currentChapter));
    m_currentChapter = newName;

    refreshChapters();
    refreshCommits();
    updateStatusBar();
}

void MainWindow::onDeleteChapter()
{
    if (m_currentChapter.isEmpty()) return;

    auto answer = QMessageBox::question(this, "确认删除",
        QString("确定要删除章节 \"%1\" 吗？\n此操作不可撤销！").arg(m_currentChapter),
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    if (!m_gitManager->removeFolder(m_currentChapter)) {
        QMessageBox::critical(this, "错误", "删除失败");
        return;
    }

    m_gitManager->commit(QString("%1: 删除章节").arg(m_currentChapter));

    m_currentChapter.clear();
    m_commitDetail->clear();
    ui->renameChapterBtn->setEnabled(false);
    ui->deleteChapterBtn->setEnabled(false);
    ui->rollbackBtn->setEnabled(false);

    refreshChapters();
    refreshCommits();
    updateStatusBar();
}

// ==================== Commit 面板 ==================== //

void MainWindow::onCommitSelected(const QModelIndex &index)
{
    // 获取选中的 commit
    CommitInfo info = m_commitModel->commitAt(index.row());

    // 在详情面板显示
    m_commitDetail->showCommit(info);

    // 从 commit 涉及的文件中推断章节名（取路径的第一级目录）
    // 例如 "ch01-引言/main.tex" → "ch01-引言"
    if (m_currentChapter.isEmpty() && !info.files.isEmpty()) {
        QString firstPath = info.files.first();
        int slashIdx = firstPath.indexOf('/');
        if (slashIdx > 0) {
            QString chapterFromCommit = firstPath.left(slashIdx);
            // 验证这是有效的章节名（在章节列表中存在）
            int chapterIdx = m_chapterModel->chapters().indexOf(chapterFromCommit);
            if (chapterIdx >= 0) {
                m_currentChapter = chapterFromCommit;
                // 同步 UI
                QModelIndex chapIdx = m_chapterModel->index(chapterIdx, 0);
                ui->chapterListView->setCurrentIndex(chapIdx);
                ui->renameChapterBtn->setEnabled(true);
                ui->deleteChapterBtn->setEnabled(true);
            }
        }
    }

    // 只要确定了章节，就允许回退
    ui->rollbackBtn->setEnabled(!m_currentChapter.isEmpty());
}

void MainWindow::onFilterChanged(int comboIndex)
{
    Q_UNUSED(comboIndex);
    // 同步当前章节为筛选项
    QString filter = ui->filterCombo->currentData().toString();
    if (!filter.isEmpty())
        m_currentChapter = filter;
    refreshCommits();
}

void MainWindow::onRollbackToCommit()
{
    if (m_currentChapter.isEmpty()) return;

    // 获取当前选中的 commit
    QModelIndex idx = ui->commitListView->currentIndex();
    if (!idx.isValid()) return;

    CommitInfo info = m_commitModel->commitAt(idx.row());

    // 二次确认
    auto answer = QMessageBox::question(this, "确认回退",
        QString("将章节 \"%1\" 回退到版本:\n\n[%2] %3\n\n"
                "这将覆盖该章节文件夹中的所有文件，确定继续？")
            .arg(m_currentChapter, info.hash.left(7), info.message),
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    // 执行回退
    if (!m_gitManager->restoreFolder(m_currentChapter, info.hash)) {
        QMessageBox::critical(this, "回退失败", "无法回退章节");
        return;
    }

    // 回退本身产生一次新 commit，方便追踪
    m_gitManager->stageAndCommit(
        QString("%1: 回退至 %2").arg(m_currentChapter, info.hash.left(7)));

    refreshCommits();
    statusBar()->showMessage(
        QString("章节 \"%1\" 已回退到版本 %2")
            .arg(m_currentChapter, info.hash.left(7)), 5000);
}

void MainWindow::onCommitChanges()
{
    if (m_workspacePath.isEmpty()) return;

    if (!m_gitManager->hasUncommittedChanges()) {
        QMessageBox::information(this, "提示", "没有需要提交的变更");
        return;
    }

    // 让用户输入 commit message
    bool ok;
    QString msg = QInputDialog::getText(this, "提交变更",
        "Commit Message:", QLineEdit::Normal, "更新内容", &ok);
    if (!ok || msg.isEmpty()) return;

    if (m_gitManager->stageAndCommit(msg)) {
        refreshCommits();
        statusBar()->showMessage("提交成功", 3000);
    } else {
        QMessageBox::critical(this, "提交失败",
            "提交失败，请检查状态栏错误信息");
    }
}

// ==================== 文件监控 ==================== //

void MainWindow::onExternalFileChanged(const QString &path)
{
    Q_UNUSED(path);
    updateStatusBar();
}

// ==================== 数据刷新 ==================== //

void MainWindow::refreshChapters()
{
    if (m_workspacePath.isEmpty()) return;

    // 扫描工作区根目录下的所有子文件夹，排除 .git
    QDir dir(m_workspacePath);
    QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    entries.removeAll(".git");

    // 更新章节列表 Model
    m_chapterModel->setChapters(entries);

    // 同步更新筛选下拉框
    QString currentFilter = ui->filterCombo->currentData().toString();
    ui->filterCombo->blockSignals(true);  // 防止触发 onFilterChanged
    ui->filterCombo->clear();
    ui->filterCombo->addItem("全部", QString());
    for (const QString &chapter : entries) {
        ui->filterCombo->addItem(chapter, chapter);
    }
    // 恢复之前选中的筛选项
    int idx = ui->filterCombo->findData(currentFilter);
    if (idx >= 0) ui->filterCombo->setCurrentIndex(idx);
    ui->filterCombo->blockSignals(false);
}

void MainWindow::refreshCommits()
{
    if (m_workspacePath.isEmpty()) {
        m_commitModel->clear();
        return;
    }

    // 根据筛选下拉框当前选中的章节来决定查什么
    QString filter = ui->filterCombo->currentData().toString();
    QList<CommitInfo> commits;
    if (filter.isEmpty()) {
        commits = m_gitManager->log();   // 查全部
    } else {
        commits = m_gitManager->log(filter);  // 只查该章节
    }
    m_commitModel->setCommits(commits);
}

void MainWindow::updateStatusBar()
{
    if (m_workspacePath.isEmpty()) {
        statusBar()->showMessage("请打开一个工作区目录");
        return;
    }

    QString branch = m_gitManager->currentBranch();
    bool dirty = m_gitManager->hasUncommittedChanges();
    int chapterCount = m_chapterModel->rowCount();

    QString msg = QString("工作区: %1 | 分支: %2 | %3 | 章节数: %4")
        .arg(m_workspacePath,
             branch,
             dirty ? "有未提交变更" : "已全部提交")
        .arg(chapterCount);

    statusBar()->showMessage(msg);
}
