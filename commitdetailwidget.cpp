#include "commitdetailwidget.h"

CommitDetailWidget::CommitDetailWidget(QObject *parent)
    : QObject(parent)
{
}

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

void CommitDetailWidget::showCommit(const CommitInfo &info)
{
    if (!m_browser) return;

    QString dateStr = info.dateTime.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
    QString filesStr = info.files.join("<br>");

    // 对 HTML 特殊字符转义
    auto esc = [](const QString &s) {
        return s.toHtmlEscaped();
    };

    QString html = QString(
        "<table>"
        "<tr><td class='label'>Hash:</td>"
        "<td class='value'>%1</td></tr>"
        "<tr><td class='label'>作者:</td>"
        "<td class='value'>%2</td></tr>"
        "<tr><td class='label'>时间:</td>"
        "<td class='value'>%3</td></tr>"
        "<tr><td class='label'>Message:</td>"
        "<td class='value'>%4</td></tr>"
        "<tr><td class='label'>涉及文件:</td>"
        "<td class='value'>%5</td></tr>"
        "</table>"
    ).arg(esc(info.hash),
          esc(info.author),
          esc(dateStr),
          esc(info.message),
          esc(filesStr));

    m_browser->setHtml(html);
}

void CommitDetailWidget::clear()
{
    if (m_browser)
        m_browser->clear();
}
