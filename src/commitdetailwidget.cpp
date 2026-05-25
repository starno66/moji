#include "commitdetailwidget.h"
#include <QRegularExpression>

CommitDetailWidget::CommitDetailWidget(QObject *parent) : QObject(parent) {}

void CommitDetailWidget::bind(QTextBrowser *browser)
{
    m_browser = browser;
    if (m_browser) {
        m_browser->document()->setDefaultStyleSheet(
            "table { width: 100%; }"
            "td.label { text-align: right; vertical-align: top; "
            "           font-weight: bold; white-space: nowrap; "
            "           padding-right: 12px; width: 70px; }"
            "td.value { vertical-align: top; font-family: Consolas, monospace; "
            "           word-wrap: break-word; white-space: pre-wrap; }"
        );
    }
}

void CommitDetailWidget::setCommitList(const QList<CommitInfo> &commits)
{
    m_allCommits = commits;
}

QString CommitDetailWidget::resolveMessage(const QString &message) const
{
    static const QRegularExpression re(QStringLiteral("回退至 ([0-9a-f]{7,})"));
    QRegularExpressionMatch match = re.match(message);
    if (!match.hasMatch())
        return message;

    QString targetHash = match.captured(1);
    QString result = message;
    for (int i = 0; i < m_allCommits.size(); ++i) {
        if (m_allCommits[i].hash.startsWith(targetHash)) {
            result.replace(targetHash, QString("[%1]").arg(i + 1));
            break;
        }
    }
    return result;
}

void CommitDetailWidget::showCommit(const CommitInfo &info)
{
    if (!m_browser) return;

    QString dateStr = info.dateTime.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");

    // 过滤临时文件，并去掉第一级目录（章节文件夹名）
    QStringList visibleFiles;
    for (const QString &f : info.files) {
        QString name = f.section('/', -1);
        if (!name.startsWith("~$") && !name.startsWith(".") && !name.endsWith("~"))
            visibleFiles.append(f);
    }
    QString filesStr = visibleFiles.join("<br>");
    QString message = resolveMessage(info.message);

    auto esc = [](const QString &s) {
        return s.toHtmlEscaped();
    };

    QString html = QString(
        "<table>"
        "<tr><td class='label'>作者:</td>"
        "<td class='value'>%1</td></tr>"
        "<tr><td class='label'>时间:</td>"
        "<td class='value'>%2</td></tr>"
        "<tr><td class='label'>Message:</td>"
        "<td class='value'>%3</td></tr>"
        "<tr><td class='label'>涉及文件:</td>"
        "<td class='value'>%4</td></tr>"
        "</table>"
    ).arg(esc(info.author),
          esc(dateStr),
          esc(message),
          esc(filesStr));

    m_browser->setHtml(html);
}

void CommitDetailWidget::clear()
{
    if (m_browser)
        m_browser->clear();
}
