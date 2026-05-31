#include "commitdetailwidget.h"
#include "memodialog.h"
#include <QDesktopServices>
#include <QDir>
#include <QMenu>
#include <QRegularExpression>
#include <QUrl>

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
            "a.file-link { color: #0969da; text-decoration: none; }"
            "a.file-link:hover { text-decoration: underline; }"
        );
        connect(m_browser, &QTextBrowser::anchorClicked,
                this, &CommitDetailWidget::onLinkClicked);
        m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_browser, &QTextBrowser::customContextMenuRequested,
                this, &CommitDetailWidget::onFileContextMenu);
    }
}

void CommitDetailWidget::setCommitList(const QList<CommitInfo> &commits)
{
    m_allCommits = commits;
}

void CommitDetailWidget::setWorkspacePath(const QString &path)
{
    m_workspacePath = path;
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

    // 过滤临时文件，并渲染为可点击的链接
    QStringList visibleFiles;
    for (const QString &f : info.files) {
        QString name = f.section('/', -1);
        if (name.startsWith("~$") || name.startsWith(".") || name.endsWith("~"))
            continue;

        // 去掉第一级目录（章节文件夹）
        QString displayPath = f.section('/', 1);
        if (displayPath.isEmpty()) displayPath = f;

        // 构造可点击的文件链接（用 QUrl 正确编码路径）
        QString absPath = m_workspacePath + "/" + f;
        // 统一分隔符
        absPath.replace('\\', '/');
        QUrl fileUrl = QUrl::fromLocalFile(absPath);
        visibleFiles.append(
            QString("<a class='file-link' href='%1'>%2</a>")
                .arg(fileUrl.toString(), displayPath.toHtmlEscaped()));
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
          filesStr);

    m_browser->setHtml(html);
}

void CommitDetailWidget::clear()
{
    if (m_browser)
        m_browser->clear();
}

void CommitDetailWidget::onLinkClicked(const QUrl &url)
{
    if (url.isLocalFile())
        QDesktopServices::openUrl(url);
}

void CommitDetailWidget::onFileContextMenu(const QPoint &pos)
{
    if (!m_browser) return;

    QString anchor = m_browser->anchorAt(pos);
    if (anchor.isEmpty()) return;

    QUrl url(anchor);
    if (!url.isLocalFile()) return;

    QString filePath = url.toLocalFile();
    // 计算相对于工作区的路径
    QString relPath = filePath;
    if (filePath.startsWith(m_workspacePath))
        relPath = filePath.mid(m_workspacePath.length() + 1);

    QMenu menu(m_browser);
    QAction *openAction = menu.addAction("打开文件");
    QAction *memoAction = menu.addAction("查看随记");

    QAction *chosen = menu.exec(m_browser->viewport()->mapToGlobal(pos));
    if (chosen == openAction) {
        QDesktopServices::openUrl(url);
    } else if (chosen == memoAction) {
        auto *dlg = new MemoDialog(m_workspacePath, nullptr);
        dlg->selectFile(QDir::fromNativeSeparators(relPath));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setWindowModality(Qt::ApplicationModal);
        dlg->show();
    }
}
