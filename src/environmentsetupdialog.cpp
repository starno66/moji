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
        "font-size:13px;font-weight:bold;margin-right:8px;}"
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
    m_installBtn->setEnabled(!m_gitInstalled);
    if (m_gitInstalled) {
        m_installBtn->setText("✓ Git 已安装");
        m_installBtn->setStyleSheet("background-color:#16a34a;color:#fff;border:none;"
            "border-radius:8px;padding:6px 16px;font-size:14px;font-weight:600;");
    }
    m_nameEdit->setEnabled(m_gitInstalled);
    m_emailEdit->setEnabled(m_gitInstalled);
    m_saveBtn->setEnabled(m_gitInstalled);
}

void EnvironmentSetupDialog::onInstallGit()
{
    m_installBtn->setEnabled(false);
    m_installBtn->setText("安装中...");
    m_statusLabel->setText("正在下载并安装 Git，请稍候（可能需要几分钟）...");

    if (GitManager::installGit()) {
        m_gitInstalled = true;
        m_installBtn->setText("✓ Git 已安装");
        m_statusLabel->setText("Git 安装成功！请填写您的作者信息，然后保存配置。");
    } else {
        m_installBtn->setText("一键安装 Git");
        m_installBtn->setEnabled(true);
        m_statusLabel->setText("安装失败。请尝试手动安装。");

        QMessageBox::warning(this, "安装失败",
            "自动安装失败。请手动下载安装 Git：\n\n"
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
