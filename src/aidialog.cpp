#include "aidialog.h"
#include <QCloseEvent>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QTextBrowser>
#include <QTextEdit>
#include <QVBoxLayout>

// ── 简单 Markdown → HTML：仅处理 **粗体** 和换行 ──
static QString simpleMarkdown(const QString &s)
{
    QStringList out;
    QStringList liBuf;   // 等待 <ul> 包裹的 <li>
    QStringList txtBuf;  // 等待 <p> 包裹的普通行

    auto flushText = [&]() {
        if (!txtBuf.isEmpty()) {
            out.append("<p>" + txtBuf.join("<br>") + "</p>");
            txtBuf.clear();
        }
    };
    auto flushList = [&]() {
        if (!liBuf.isEmpty()) {
            out.append("<ul style='margin:4px 0;'>" + liBuf.join("") + "</ul>");
            liBuf.clear();
        }
    };

    QStringList lines = s.split('\n');
    for (const QString &raw : lines) {
        QString line = raw.trimmed();
        if (line.isEmpty()) { flushText(); flushList(); continue; }

        line.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
        line.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<b>\\1</b>");
        line.replace(QRegularExpression("`([^`]+)`"), "<code>\\1</code>");

        if (line.startsWith("### ")) {
            flushText(); flushList();
            out.append("<p style='font-weight:bold;margin:14px 0 6px;'>" + line.mid(4) + "</p>");
        } else if (line.startsWith("## ")) {
            flushText(); flushList();
            out.append("<p style='font-weight:bold;margin:12px 0 5px;'>" + line.mid(3) + "</p>");
        } else if (line.startsWith("# ")) {
            flushText(); flushList();
            out.append("<p style='font-weight:bold;margin:12px 0 5px;'>" + line.mid(2) + "</p>");
        } else if (line.startsWith("- ")) {
            flushText();
            liBuf.append("<li>" + line.mid(2) + "</li>");
        } else {
            flushList();
            txtBuf.append(line);
        }
    }
    flushText();
    flushList();
    return out.join("");
}

AiDialog::AiDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("AI 写作助手");
    resize(820, 820);

    auto *lay = new QVBoxLayout(this);

    auto *keyRow = new QHBoxLayout();
    keyRow->addWidget(new QLabel("API Key:"));
    m_apiKeyEdit = new QLineEdit();
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setText(QSettings().value("ai/apiKey").toString());
    keyRow->addWidget(m_apiKeyEdit, 1);
    auto *saveBtn = new QPushButton("保存");
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        QSettings().setValue("ai/apiKey", m_apiKeyEdit->text().trimmed());
        m_statusLabel->setText("已保存");
    });
    keyRow->addWidget(saveBtn);
    lay->addLayout(keyRow);

    auto *modelRow = new QHBoxLayout();
    modelRow->addWidget(new QLabel("模型:"));
    m_modelCombo = new QComboBox();
    m_modelCombo->addItem("deepseek-v4-pro", "deepseek-v4-pro");
    m_modelCombo->addItem("deepseek-flash", "deepseek-flash");
    int idx = m_modelCombo->findData(
        QSettings().value("ai/model", "deepseek-v4-pro"));
    if (idx >= 0) m_modelCombo->setCurrentIndex(idx);
    modelRow->addWidget(m_modelCombo, 1);
    lay->addLayout(modelRow);

    m_chatView = new QTextBrowser();
    m_chatView->setOpenLinks(false);
    lay->addWidget(m_chatView, 1);

    m_inputEdit = new QTextEdit();
    m_inputEdit->setMaximumHeight(80);
    m_inputEdit->setPlaceholderText("输入提示词...");
    lay->addWidget(m_inputEdit);

    auto *bot = new QHBoxLayout();
    m_statusLabel = new QLabel("就绪");
    m_statusLabel->setStyleSheet("color:#656d76;font-size:12px;");
    bot->addWidget(m_statusLabel);
    bot->addStretch();
    m_sendBtn = new QPushButton("发送");
    connect(m_sendBtn, &QPushButton::clicked, this, &AiDialog::onSend);
    bot->addWidget(m_sendBtn);
    lay->addLayout(bot);

    m_net = new QNetworkAccessManager(this);
    connect(m_net, &QNetworkAccessManager::finished,
            this, &AiDialog::onReplyFinished);
}

AiDialog::~AiDialog()
{
    if (m_reply) { m_reply->abort(); m_reply->deleteLater(); }
}

void AiDialog::closeEvent(QCloseEvent *e)
{
    if (m_reply) { m_reply->abort(); m_reply->deleteLater(); m_reply = nullptr; }
    m_busy = false;
    e->accept();
}

void AiDialog::setCommitContext(const QString &ctx)
{
    if (!m_messages.isEmpty()) return;
    QString sys = QStringLiteral(
        "## 角色定位\n你是「墨迹」写作平台的 AI 助手。墨迹是一款面向大学生和教师的"
        "Git 版本管理写作工具，用户通过章节文件夹管理文档，用 Git 追踪每次修改历史。"
        "学生在main分支下进行写作，教师或学生都可以创建分支进行修改。\n\n"
        "## 核心能力\n"
        "1.分析commit历史：总结修改趋势、识别高频修改区域、发现写作瓶颈\n"
        "2.写作建议：根据commit message和文件变更给出改进意见\n"
        "3.分支协作解读：帮助理解教师反馈分支的修改意图\n"
        "4.章节结构建议：根据commit涉及的文件分布，建议合理的章节拆分\n"
        "5.详细整理出这篇文章经过谁手，做出了怎样的修改\n\n"
        "## 输出格式\n- 用中文回复\n- 分析类回复用分点列表，每条配具体理由\n"
        "- 涉及具体commit或文件时，用方括号标注编号\n"
        "- 每段分析到位但不冗杂\n\n"
        "## 注意事项\n- 仅基于提供的commit上下文作答，不编造信息，不出现AI幻觉\n"
        "- 如果用户问题超出写作辅助范围，委婉引导回正题\n"
        "- 如果commit历史为空，提示用户先进行提交");
    if (!ctx.isEmpty())
        sys += "\n\n## 当前 Commit 历史上下文\n" + ctx;

    QJsonObject s;
    s["role"] = "system"; s["content"] = sys;
    m_messages.append(s);
    addBubble("assistant", "你好！我是墨迹 AI 写作助手。我已读取当前的 commit 历史，"
              "可以帮你分析修改趋势、提供写作建议、解读反馈。请随时提问！");
}

void AiDialog::onSend()
{
    if (m_busy) return;
    QString key = m_apiKeyEdit->text().trimmed();
    if (key.isEmpty()) { m_statusLabel->setText("请先输入并保存 API Key"); return; }
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;

    QSettings().setValue("ai/model", m_modelCombo->currentData().toString());

    addBubble("user", text);
    m_inputEdit->clear();
    m_sendBtn->setEnabled(false);
    m_busy = true;
    m_statusLabel->setText("等待回复...");

    QJsonObject u;
    u["role"] = "user"; u["content"] = text;
    m_messages.append(u);

    QJsonObject body;
    body["model"] = m_modelCombo->currentData().toString();
    body["messages"] = m_messages;
    body["stream"] = false;

    QNetworkRequest req(QUrl("https://api.deepseek.com/v1/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
    m_reply = m_net->post(req, QJsonDocument(body).toJson());
}

void AiDialog::addBubble(const QString &role, const QString &text)
{
    bool isUser = (role == "user");
    QString body  = isUser
        ? text.toHtmlEscaped().replace('\n', "<br>")
        : simpleMarkdown(text);

    // 气泡样式：用户右对齐蓝底白字，AI 左对齐白底
    QString bubbleStyle = isUser
        ? "background:#1e40af;color:#ffffff;border-radius:12px 12px 4px 12px;"
        : "background:#ffffff;border:1px solid #e2e8f0;border-radius:12px 12px 12px 4px;";
    QString align = isUser ? "flex-end" : "flex-start";
    QString bubbleLabel = isUser ? "👤 你" : "🤖 墨迹 AI";

    QString bubble = QString(
        "<div style='display:flex;flex-direction:column;align-items:%1;margin:10px 12px;'>"
        "<span style='font-size:13px;color:#94a3b8;margin-bottom:3px;'>%2</span>"
        "<div style='%3 padding:10px 14px;max-width:80%%;font-size:15px;line-height:1.6;'>"
        "%4</div></div>"
    ).arg(align, bubbleLabel, bubbleStyle, body);

    m_chatHtml += bubble;
    renderHtml();
}

void AiDialog::renderHtml()
{
    m_chatView->setHtml(
        "<style>"
        "body{font-size:15px;line-height:1.7;color:#1e293b;background:#f8fafc;margin:0;padding:8px;}"
        "b{font-weight:bold;}"
        "code{background:#f1f5f9;padding:1px 6px;border-radius:4px;"
        "font-family:'Cascadia Code',Consolas,monospace;font-size:14px;}"
        "ul{margin:4px 0 8px;padding-left:20px;}"
        "li{margin:4px 0;}"
        "p{margin:4px 0 8px;}"
        "</style>" + m_chatHtml);
    m_chatView->verticalScrollBar()->setValue(
        m_chatView->verticalScrollBar()->maximum());
}

void AiDialog::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply != m_reply) return;
    m_reply = nullptr;
    m_sendBtn->setEnabled(true);
    m_busy = false;

    if (reply->error() != QNetworkReply::NoError) {
        QString err = (reply->error() == QNetworkReply::AuthenticationRequiredError)
            ? "❌ API Key 无效，请检查后重试。"
            : QString("❌ 请求失败: %1").arg(reply->errorString());
        m_statusLabel->setText("出错");
        addBubble("assistant", err);
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    QString content = obj["choices"].toArray()[0]
        .toObject()["message"].toObject()["content"].toString();

    if (content.isEmpty()) {
        m_statusLabel->setText("空回复");
        return;
    }

    QJsonObject a;
    a["role"] = "assistant"; a["content"] = content;
    m_messages.append(a);

    addBubble("assistant", content);
    m_statusLabel->setText("就绪");
}
