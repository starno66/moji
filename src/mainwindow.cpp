#include "mainwindow.h"
#include "ui_mainwindow.h"          // Designer 生成的 UI 头文件
#include "gitmanager.h"
#include "filewatcher.h"
#include "chapterlistmodel.h"
#include "commitmodel.h"
#include "commitdetailwidget.h"
#include "environmentsetupdialog.h"
#include "aidialog.h"

#include <QActionGroup>
#include <QApplication>
#include <QFileDialog>      // 选择文件夹对话框
#include <QProcess>
#include <QInputDialog>     // 输入文本对话框
#include <QMessageBox>      // 消息提示框
#include <QSettings>        // 持久化存储（记住上次工作区）
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QTimer>
#include <QVBoxLayout>

extern QString makeStyleSheet(int scale);

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
    initBranchBar();

    // 第4步：用代码创建菜单栏
    setupMenus();

    // 第5步：连接所有信号槽
    connectSignals();

    // 定时轮询文件变更（QFileSystemWatcher 在 Windows 上不够可靠）
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(2000);
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);

    // 第6步：首次启动检测 Git 环境（在工作区恢复之前）
    {
        QSettings settings;
        if (!settings.contains("setup/configured")) {
            if (!GitManager::isGitInstalled()) {
                EnvironmentSetupDialog wizard(this);
                wizard.exec();
            }
            // 只有 Git 实际可用时才标记已配置，避免用户跳过向导后无法再次进入
            if (GitManager::isGitInstalled())
                settings.setValue("setup/configured", true);
        }
    }

    // 第7步：恢复上次工作区
    QSettings settings;
    QString lastPath = settings.value("workspace/lastPath").toString();
    bool opened = false;
    if (!lastPath.isEmpty() && QDir(lastPath + "/.git").exists()) {
        opened = openWorkspace(lastPath);
    }

    restoreGeometry(settings.value("window/geometry").toByteArray());

    if (!opened) {
        ui->newChapterBtn->setEnabled(false);
        ui->renameChapterBtn->setEnabled(false);
        ui->deleteChapterBtn->setEnabled(false);
        ui->commitBtn->setEnabled(false);
    }

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
    ui->commitListView->setItemDelegate(new CommitDelegate(this));
    ui->commitListView->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::initDetailWidget()
{
    m_commitDetail = new CommitDetailWidget(this);
    m_commitDetail->bind(ui->commitDetailBrowser);
}

void MainWindow::initBranchBar()
{
    // 分支栏作为独立行，插入到 historyLayout 中（标题行下方、分割器上方）
    QVBoxLayout *historyLayout = ui->historyLayout;
    if (!historyLayout) return;

    // 创建分支栏水平布局
    auto *branchBar = new QHBoxLayout();
    branchBar->setContentsMargins(0, 2, 0, 4);

    auto *branchLabel = new QLabel("当前分支:");
    QFont boldFont = branchLabel->font();
    boldFont.setBold(true);
    branchLabel->setFont(boldFont);
    branchBar->addWidget(branchLabel);

    m_branchCombo = new QComboBox();
    m_branchCombo->setObjectName("branchCombo");
    m_branchCombo->setMinimumWidth(300);
    m_branchCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    branchBar->addWidget(m_branchCombo);

    m_newBranchBtn = new QPushButton("新建分支");
    m_newBranchBtn->setObjectName("newBranchBtn");
    branchBar->addWidget(m_newBranchBtn);

    branchBar->addSpacing(8);

    m_mergeBtn = new QPushButton("合并到当前");
    m_mergeBtn->setObjectName("mergeBtn");
    branchBar->addWidget(m_mergeBtn);

    branchBar->addStretch();

    // 插入到 header 和 splitter 之间（索引 1）
    historyLayout->insertLayout(1, branchBar);
}

void MainWindow::refreshBranches()
{
    if (m_workspacePath.isEmpty()) return;

    m_branchCombo->blockSignals(true);
    m_branchCombo->clear();

    QStringList branches = m_gitManager->listBranches();
    for (const QString &b : branches)
        m_branchCombo->addItem(b);

    // 选中当前分支
    m_currentBranch = m_gitManager->currentBranch();
    int idx = m_branchCombo->findText(m_currentBranch);
    if (idx >= 0) m_branchCombo->setCurrentIndex(idx);

    m_branchCombo->blockSignals(false);
    CommitDelegate::setCurrentBranch(m_currentBranch);
    // 获取当前分支独有 commit 的 hash 集合
    if (m_currentBranch != "main") {
        QString mb = m_gitManager->mergeBase(m_currentBranch, "main");
        if (!mb.isEmpty()) {
            QProcess proc;
            proc.setWorkingDirectory(m_workspacePath);
            proc.start("git",
                {"log", "--format=%H", QString("%1..HEAD").arg(mb.left(7))});
            if (proc.waitForFinished(10000) && proc.exitCode() == 0) {
                CommitDelegate::setBranchOnlyHashes(
                    QString::fromUtf8(proc.readAllStandardOutput())
                        .split('\n', Qt::SkipEmptyParts));
            }
        }
    } else {
        CommitDelegate::setBranchOnlyHashes({});
    }
}

void MainWindow::onCreateBranch()
{
    if (m_workspacePath.isEmpty()) return;

    bool ok;
    QString name = QInputDialog::getText(this, "新建分支",
        "请输入分支名称（例如 feedback-张三）:",
        QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    name = name.trimmed().replace(' ', '-');
    if (!m_gitManager->createBranch(name)) {
        QMessageBox::critical(this, "错误", "创建分支失败");
        return;
    }

    if (!m_gitManager->switchBranch(name)) {
        QMessageBox::critical(this, "错误", "切换分支失败");
        return;
    }

    refreshBranches();
    refreshCommits();
    updateStatusBar();
    statusBar()->showMessage(QString("已创建并切换到分支: %1").arg(name), 3000);
}

void MainWindow::onSwitchBranch()
{
    if (m_workspacePath.isEmpty() || !m_branchCombo) return;

    QString target = m_branchCombo->currentText();
    if (target.isEmpty() || target == m_currentBranch) return;

    if (!m_gitManager->switchBranch(target)) {
        QMessageBox::critical(this, "错误",
            QString("无法切换到分支 %1，请先处理未提交的变更").arg(target));
        refreshBranches();
        return;
    }

    refreshBranches();
    refreshCommits();
    updateStatusBar();
    statusBar()->showMessage(QString("已切换到分支: %1").arg(target), 3000);
}

void MainWindow::onMergeBranch()
{
    if (m_workspacePath.isEmpty()) return;

    // 列出所有非当前分支
    QStringList branches = m_gitManager->listBranches();
    branches.removeAll(m_gitManager->currentBranch());
    if (branches.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可合并的分支");
        return;
    }

    bool ok;
    QString source = QInputDialog::getItem(this, "合并分支",
        "选择要合并到当前分支的分支:", branches, 0, false, &ok);
    if (!ok) return;

    auto answer = QMessageBox::question(this, "确认合并",
        QString("将 \"%1\" 合并到 \"%2\"，确定继续？")
            .arg(source, m_gitManager->currentBranch()),
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    if (!m_gitManager->mergeBranch(source)) {
        QMessageBox::critical(this, "合并失败", "合并失败，可能存在冲突需要手动处理");
        return;
    }

    refreshCommits();
    updateStatusBar();
    statusBar()->showMessage(QString("已合并 %1 到当前分支").arg(source), 5000);

    // 询问是否删除已合并的分支
    auto delAnswer = QMessageBox::question(this, "删除分支",
        QString("合并完成。是否删除分支 \"%1\"？").arg(source),
        QMessageBox::Yes | QMessageBox::No);
    if (delAnswer == QMessageBox::Yes) {
        if (m_gitManager->deleteBranch(source)) {
            refreshBranches();
            statusBar()->showMessage(
                QString("已删除分支: %1").arg(source), 3000);
        } else {
            QMessageBox::warning(this, "删除失败",
                QString("无法删除分支 %1").arg(source));
        }
    }
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

    // ---- 远程菜单 ----
    QMenu *remoteMenu = menuBar()->addMenu("远程(&R)");

    QAction *pushAction = remoteMenu->addAction("推送到远程仓库(&P)");
    pushAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
    connect(pushAction, &QAction::triggered, this, &MainWindow::onPushToRemote);

    // ---- AI 菜单 ----
    QMenu *aiMenu = menuBar()->addMenu("AI(&A)");

    QAction *aiAction = aiMenu->addAction("AI 写作助手(&C)");
    aiAction->setShortcut(QKeySequence("Ctrl+I"));
    connect(aiAction, &QAction::triggered, this, [this]() {
        if (!m_aiDialog) {
            m_aiDialog = new AiDialog(nullptr);
            m_aiDialog->setWindowFlags(Qt::Window);
            m_aiDialog->setAttribute(Qt::WA_DeleteOnClose);
            connect(m_aiDialog, &QDialog::destroyed, this, [this]() {
                m_aiDialog = nullptr;
            });
        }
        // 传递当前 commit 历史作为上下文
        if (m_commitModel->rowCount() > 0) {
            QStringList ctx;
            for (int i = 0; i < m_commitModel->rowCount(); ++i) {
                CommitInfo ci = m_commitModel->commitAt(i);
                ctx.append(QString("[%1] %2 | %3 | %4")
                    .arg(i + 1)
                    .arg(ci.dateTime.toLocalTime().toString("yyyy-MM-dd hh:mm"))
                    .arg(ci.message)
                    .arg(ci.files.join(", ")));
            }
            m_aiDialog->setCommitContext(ctx.join('\n'));
        }
        m_aiDialog->show();
        m_aiDialog->raise();
        m_aiDialog->activateWindow();
    });

    // ---- 查看菜单 ----
    QMenu *viewMenu = menuBar()->addMenu("查看(&V)");

    QMenu *fontMenu = viewMenu->addMenu("字体大小");
    auto *fontGroup = new QActionGroup(this);
    fontGroup->setExclusive(true);

    int curScale = QSettings().value("ui/fontScale", 0).toInt();

    auto makeFontAction = [&](const QString &label, int scale) {
        QAction *act = fontMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(scale == curScale);
        act->setData(scale);
        fontGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, scale]() {
            onSetFontScale(scale);
        });
    };
    makeFontAction("小", -1);
    makeFontAction("中", 0);
    makeFontAction("大", 1);
}

void MainWindow::connectSignals()
{
    // 点击章节 → 切换当前章节
    connect(ui->chapterListView, &QListView::clicked,
            this, &MainWindow::onChapterSelected);

    // 点击 commit → 显示详情
    connect(ui->commitListView, &QListView::clicked,
            this, &MainWindow::onCommitSelected);

    // 右键 commit → 上下文菜单
    connect(ui->commitListView, &QListView::customContextMenuRequested,
            this, &MainWindow::onCommitContextMenu);

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

    // 分支栏
    connect(m_newBranchBtn, &QPushButton::clicked,
            this, &MainWindow::onCreateBranch);
    connect(m_branchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ onSwitchBranch(); });
    connect(m_mergeBtn, &QPushButton::clicked,
            this, &MainWindow::onMergeBranch);
}

QString MainWindow::branchTaggedMessage(const QString &base) const
{
    if (m_currentBranch != "main")
        return QString("%1（来自\"%2\"）").arg(base, m_currentBranch);
    return base;
}

// ==================== 文件菜单 ==================== //

void MainWindow::onOpenWorkspace()
{
    // 让用户选择：本地 or 远程
    QStringList items;
    items << "打开本地工作区" << "克隆远程仓库";

    bool ok;
    QString choice = QInputDialog::getItem(this, "打开工作区",
        "请选择打开方式：", items, 0, false, &ok);
    if (!ok) return;

    if (choice == "打开本地工作区") {
        QString path = QFileDialog::getExistingDirectory(this, "选择工作区目录");
        if (path.isEmpty()) return;
        openWorkspace(path);
    } else {
        // 输入远程链接
        QString url = QInputDialog::getText(this, "克隆远程仓库",
            "请输入远程仓库 HTTPS 链接：",
            QLineEdit::Normal, "https://github.com/", &ok);
        if (!ok || url.isEmpty() || url == "https://github.com/") return;

        // 选择克隆到哪个目录
        QString parentDir = QFileDialog::getExistingDirectory(this,
            "选择克隆目标目录");
        if (parentDir.isEmpty()) return;

        // 从 URL 中提取仓库名
        QString repoName = "repo";
        int lastSlash = url.lastIndexOf('/');
        if (lastSlash >= 0) {
            repoName = url.mid(lastSlash + 1);
            if (repoName.endsWith(".git"))
                repoName.chop(4);
        }
        QString targetPath = parentDir + "/" + repoName;

        // 检查目标目录是否已存在
        if (QDir(targetPath).exists()) {
            QMessageBox::warning(this, "已存在",
                QString("目录 \"%1\" 已存在，请选择其他位置。").arg(targetPath));
            return;
        }

        // 克隆
        statusBar()->showMessage("正在克隆远程仓库...");
        QApplication::processEvents();

        if (!m_gitManager->cloneRepo(url, targetPath)) {
            QMessageBox::critical(this, "克隆失败",
                "克隆失败，请检查链接和网络连接。");
            statusBar()->clearMessage();
            return;
        }

        statusBar()->showMessage("克隆成功，正在打开工作区...", 2000);
        openWorkspace(targetPath);
    }
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
    m_commitDetail->setWorkspacePath(path);

    // 启动文件监控
    m_fileWatcher->watchWorkspace(path);
    m_pollTimer->start();

    // 刷新 UI
    refreshChapters();
    refreshCommits();
    refreshBranches();
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
    m_gitManager->stageAndCommit(
        branchTaggedMessage(QString("%1: 新建章节").arg(name)));

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

    m_gitManager->commit(
        branchTaggedMessage(QString("%1: 重命名自 %2").arg(newName, m_currentChapter)));
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

    m_gitManager->commit(
        branchTaggedMessage(QString("%1: 删除章节").arg(m_currentChapter)));

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
    if (info.message.contains("回退至")) {
        QMessageBox::warning(this, "不可回退",
            "该 commit 本身是一次回退操作，不能再次回退到此处。");
        return;
    }
    QString seqNum = QString::number(idx.row() + 1);
    QString targetHash = info.hash.left(7);

    // 二次确认
    auto answer = QMessageBox::question(this, "确认回退",
        QString("将章节 \"%1\" 回退到版本:\n\n[%2] %3\n\n"
                "这将覆盖该章节文件夹中的所有文件，确定继续？")
            .arg(m_currentChapter, seqNum, info.message),
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    // 执行回退
    if (!m_gitManager->restoreFolder(m_currentChapter, info.hash)) {
        QMessageBox::critical(this, "回退失败", "无法回退章节");
        return;
    }

    // commit message 存 hash（永久可追溯），显示时由 model 动态翻译为当前序号
    m_gitManager->stageAndCommit(
        branchTaggedMessage(QString("%1: 回退至 %2").arg(m_currentChapter, targetHash)));

    refreshChapters();
    refreshCommits();
    updateStatusBar();
    ui->chapterListView->viewport()->repaint();
    statusBar()->showMessage(
        QString("章节 \"%1\" 已回退到版本 %2")
            .arg(m_currentChapter, seqNum), 5000);
}

void MainWindow::onCommitContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->commitListView->indexAt(pos);
    if (!index.isValid()) return;

    CommitInfo info = m_commitModel->commitAt(index.row());

    QMenu menu(this);
    QAction *editAction = menu.addAction("修改 Commit Message");
    QAction *deleteAction = menu.addAction("删除此 Commit");
    QAction *chosen = menu.exec(ui->commitListView->viewport()->mapToGlobal(pos));

    if (chosen == editAction) {
        bool ok;
        QString newMsg = QInputDialog::getText(this, "修改 Commit Message",
            QString("编辑 Commit [%1] 的提交信息:").arg(index.row() + 1),
            QLineEdit::Normal, info.message, &ok);
        if (!ok || newMsg.isEmpty() || newMsg == info.message) return;

        if (!m_gitManager->amendMessage(info.hash, newMsg)) {
            QMessageBox::critical(this, "修改失败",
                "无法修改该 Commit 信息。可能存在冲突，请手动处理。");
            return;
        }
        refreshCommits();
        updateStatusBar();
        statusBar()->showMessage(QString("已修改 Commit [%1]").arg(index.row() + 1), 3000);
        return;
    }

    if (chosen == deleteAction) {
        auto answer = QMessageBox::question(this, "确认删除",
            QString("确定要删除 Commit [%1] %2 吗？\n\n"
                    "此操作将永久移除该提交记录，但不会影响文件内容。\n"
                    "如果已推送到远程，需要强制推送才能同步。")
                .arg(QString::number(index.row() + 1), info.message),
            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes) return;

        if (!m_gitManager->dropCommit(info.hash)) {
            QMessageBox::critical(this, "删除失败",
                "无法删除该 Commit。可能存在冲突，请手动处理。");
            return;
        }
        refreshCommits();
        updateStatusBar();
        statusBar()->showMessage(QString("已删除 Commit [%1]").arg(index.row() + 1), 3000);
    }
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

    if (m_gitManager->stageAndCommit(branchTaggedMessage(msg))) {
        refreshChapters();
        refreshCommits();
        updateStatusBar();
        ui->chapterListView->viewport()->repaint();
        statusBar()->showMessage("提交成功", 3000);
    } else {
        QMessageBox::critical(this, "提交失败",
            "提交失败，请检查状态栏错误信息");
    }
}

// ==================== 远程推送 ==================== //

void MainWindow::onPushToRemote()
{
    if (m_workspacePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先打开工作区");
        return;
    }

    // 检查远程仓库是否已配置
    if (!m_gitManager->hasRemote()) {
        bool ok;
        QString url = QInputDialog::getText(this, "配置远程仓库",
            "该项目尚未关联远程仓库。\n请输入远程仓库 HTTPS 链接：",
            QLineEdit::Normal, "https://github.com/", &ok);
        if (!ok || url.isEmpty() || url == "https://github.com/")
            return;

        if (!m_gitManager->addRemote(url)) {
            QMessageBox::critical(this, "配置失败",
                "无法添加远程仓库，请检查链接是否正确。");
            return;
        }
        statusBar()->showMessage("远程仓库已配置", 3000);
    }

    // 推送
    statusBar()->showMessage("正在推送到远程仓库...");
    QApplication::processEvents();

    if (m_gitManager->push()) {
        statusBar()->showMessage("推送成功", 5000);
    } else {
        QMessageBox::critical(this, "推送失败",
            "推送失败，请检查网络连接和远程仓库权限。\n详情见状态栏。");
    }
}

// ==================== 文件监控 ==================== //

void MainWindow::onExternalFileChanged(const QString &path)
{
    Q_UNUSED(path);
    refreshChapters();
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
    int idx = ui->filterCombo->findData(currentFilter);
    if (idx >= 0) ui->filterCombo->setCurrentIndex(idx);
    ui->filterCombo->blockSignals(false);
}

void MainWindow::refreshCommits()
{
    if (m_workspacePath.isEmpty()) {
        m_commitModel->clear();
        m_commitDetail->setCommitList({});
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
    m_commitDetail->setCommitList(commits);

    // 更新分支独有 hash 集合，保持红色标识实时
    if (m_currentBranch != "main") {
        QString mb = m_gitManager->mergeBase(m_currentBranch, "main");
        if (!mb.isEmpty()) {
            QProcess proc;
            proc.setWorkingDirectory(m_workspacePath);
            proc.start("git", {"log", "--format=%H",
                QString("%1..HEAD").arg(mb.left(7))});
            if (proc.waitForFinished(10000) && proc.exitCode() == 0) {
                CommitDelegate::setBranchOnlyHashes(
                    QString::fromUtf8(proc.readAllStandardOutput())
                        .split('\n', Qt::SkipEmptyParts));
            }
        }
    }
}

void MainWindow::updateStatusBar()
{
    if (m_workspacePath.isEmpty()) {
        statusBar()->showMessage("请打开一个工作区目录");
        return;
    }

    QString branch = m_gitManager->currentBranch();
    QString filter = ui->filterCombo->currentData().toString();
    bool dirty = filter.isEmpty()
        ? m_gitManager->hasUncommittedChanges()
        : m_gitManager->hasUncommittedChanges(filter);
    int chapterCount = m_chapterModel->rowCount();

    // 扫描目录检测每个章节的未提交状态
    QSet<QString> dirtySet;
    QDir wsDir(m_workspacePath);
    QStringList dirs = wsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &d : dirs) {
        if (d == ".git") continue;
        if (m_gitManager->hasUncommittedChanges(d))
            dirtySet.insert(d);
    }
    m_chapterModel->setDirtyChapters(dirtySet);

    ui->commitBtn->setText(dirty
        ? "提交变更（有未提交的更改）"
        : "提交变更");

    QString msg = QString("工作区: %1 | 分支: %2 | %3 | 章节数: %4")
        .arg(m_workspacePath,
             branch,
             dirty ? "有未提交变更" : "已全部提交")
        .arg(chapterCount);

    statusBar()->showMessage(msg);
}

void MainWindow::onSetFontScale(int scale)
{
    QSettings().setValue("ui/fontScale", scale);
    qApp->setStyleSheet(makeStyleSheet(scale));
}
