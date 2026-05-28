#include "environmentsetupdialog.h"
#include "gitmanager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QFrame>
#include <QProcess>
#include <QProgressBar>
#include <QTimer>

EnvironmentSetupDialog::EnvironmentSetupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("环境配置向导");
    setMinimumWidth(440);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // ── 标题 ──
    auto *titleLabel = new QLabel(
        "<style>"
        ".step{display:inline-block;width:24px;height:24px;border-radius:12px;"
        "background:#1e40af;color:#fff;text-align:center;line-height:24px;"
        "font-size:14px;font-weight:bold;margin-right:8px;}"
        "</style>"
        "<h3 style='margin-bottom:8px;'>欢迎使用墨迹</h3>"
        "<p style='color:#475569;line-height:1.6;'>"
        "<span class='step'>1</span> 安装 Git — 版本管理核心<br><br>"
        "<span class='step'>2</span> 配置作者信息<br><br>"
        "完成以上两步即可开始使用。</p>");
    titleLabel->setWordWrap(true);
    mainLayout->addWidget(titleLabel);

    // ── 安装区域 ──
    m_installBtn = new QPushButton("一键安装 Git");
    mainLayout->addWidget(m_installBtn);

    m_statusLabel = new QLabel("准备就绪，点击上方按钮安装 Git");
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    // ── 进度条 ──
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0);  // 不确定模式（动画滚动条）
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setStyleSheet(
        "QProgressBar{background:#e2e8f0;border:none;border-radius:3px;}"
        "QProgressBar::chunk{background:#1e40af;border-radius:3px;}");
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar);

    // ── 分隔线 ──
    auto *separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(separator);

    // ── 作者信息 ──
    auto *authorGroup = new QGroupBox("作者信息");
    auto *formLayout = new QFormLayout(authorGroup);
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("例如：张三");
    m_emailEdit = new QLineEdit();
    m_emailEdit->setPlaceholderText("例如：zhangsan@example.com");
    formLayout->addRow("姓名：", m_nameEdit);
    formLayout->addRow("邮箱：", m_emailEdit);
    mainLayout->addWidget(authorGroup);

    // ── 保存按钮 ──
    m_saveBtn = new QPushButton("保存配置，开始使用");
    mainLayout->addWidget(m_saveBtn);

    // ── 动画定时器 ──
    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(500);
    connect(m_progressTimer, &QTimer::timeout,
            this, &EnvironmentSetupDialog::onProgressTimer);

    // ── 连接信号 ──
    connect(m_installBtn, &QPushButton::clicked,
            this, &EnvironmentSetupDialog::onInstallGit);
    connect(m_saveBtn, &QPushButton::clicked,
            this, &EnvironmentSetupDialog::onSaveConfig);

    m_gitInstalled = GitManager::isGitInstalled();
    updateState();
}

void EnvironmentSetupDialog::updateState()
{
    m_installBtn->setEnabled(!m_gitInstalled && !m_installing);
    if (m_gitInstalled) {
        m_installBtn->setText("✓ Git 已安装");
        m_installBtn->setStyleSheet("background-color:#16a34a;color:#fff;border:none;"
            "border-radius:8px;padding:6px 16px;font-size:15px;font-weight:600;");
    }
    m_nameEdit->setEnabled(m_gitInstalled);
    m_emailEdit->setEnabled(m_gitInstalled);
    m_saveBtn->setEnabled(m_gitInstalled);
}

void EnvironmentSetupDialog::onProgressTimer()
{
    // 动画：安装中. → 安装中.. → 安装中...
    m_progressDot = (m_progressDot + 1) % 4;
    QString dots(m_progressDot, '.');
    m_installBtn->setText("正在安装" + dots);
}

void EnvironmentSetupDialog::onInstallGit()
{
    if (m_installing) return;

    // 检查 winget 是否可用
    QProcess testProc;
    testProc.start("winget", {"--version"});
    testProc.waitForFinished(5000);
    bool wingetAvailable = (testProc.exitCode() == 0);

    if (!wingetAvailable) {
        m_statusLabel->setText("未检测到 winget 包管理器。\n"
            "请手动下载安装 Git：https://git-scm.com\n"
            "安装完成后重启本应用。");
        QMessageBox::information(this, "提示",
            "自动安装需要 winget（Windows 包管理器）。\n\n"
            "您的系统可能未安装或版本过旧。请手动下载：\n"
            "https://git-scm.com\n\n"
            "下载后运行安装程序，保持默认选项即可。");
        return;
    }

    // 开始异步安装
    m_installing = true;
    m_installBtn->setEnabled(false);
    m_installBtn->setText("正在安装");
    m_installBtn->setStyleSheet("");  // 清除绿色样式
    m_statusLabel->setText("正在通过 winget 下载 Git，请稍候...");
    m_progressBar->show();
    m_progressTimer->start();

    m_installProcess = new QProcess(this);
    m_installProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_installProcess, &QProcess::readyRead,
            this, &EnvironmentSetupDialog::onInstallProgress);
    connect(m_installProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) {
        onInstallFinished(code);
    });

    m_installProcess->start("winget", {
        "install", "--id", "Git.Git", "-e",
        "--accept-source-agreements",
        "--accept-package-agreements"
    });
}

void EnvironmentSetupDialog::onInstallProgress()
{
    if (!m_installProcess) return;
    QString output = QString::fromUtf8(m_installProcess->readAll()).trimmed();
    if (!output.isEmpty()) {
        // 取最后一行有意义的内容显示
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        QString last = lines.last().trimmed();
        if (last.length() > 80)
            last = last.left(77) + "...";
        m_statusLabel->setText(last);
    }
}

void EnvironmentSetupDialog::onInstallFinished(int exitCode)
{
    m_progressTimer->stop();
    m_progressBar->hide();
    m_installing = false;

    if (m_installProcess) {
        m_installProcess->deleteLater();
        m_installProcess = nullptr;
    }

    if (exitCode == 0) {
        m_gitInstalled = true;
        m_installBtn->setText("✓ Git 已安装");
        m_statusLabel->setText("Git 安装成功！请填写您的作者信息，然后保存配置。");
    } else {
        m_installBtn->setText("一键安装 Git");
        m_statusLabel->setText("安装失败（错误码: " + QString::number(exitCode)
            + "）。请尝试手动安装 Git。");
        QMessageBox::warning(this, "安装失败",
            "自动安装未能完成。请手动下载安装 Git：\n\n"
            "https://git-scm.com\n\n"
            "安装完成后重启本应用。");
    }

    updateState();
}

void EnvironmentSetupDialog::onSaveConfig()
{
    QString name = m_nameEdit->text().trimmed();
    QString email = m_emailEdit->text().trimmed();

    if (name.isEmpty() || email.isEmpty()) {
        QMessageBox::warning(this, "信息不完整",
                             "请填写姓名和邮箱后再保存。");
        return;
    }

    if (!email.contains('@') || !email.contains('.')) {
        QMessageBox::warning(this, "邮箱格式错误",
                             "请输入有效的邮箱地址（例如：zhangsan@example.com）。");
        return;
    }

    if (!GitManager::configureUser(name, email)) {
        QMessageBox::critical(this, "配置失败",
            "无法保存 Git 用户信息。请检查 Git 是否正确安装。");
        return;
    }

    // 标记已配置，下次启动不再弹出
    QSettings settings;
    settings.setValue("setup/configured", true);

    accept();
}
