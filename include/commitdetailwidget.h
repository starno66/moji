#ifndef COMMITDETAILWIDGET_H
#define COMMITDETAILWIDGET_H

#include <QObject>
#include <QTextBrowser>
#include "gitmanager.h"

class CommitDetailWidget : public QObject
{
    Q_OBJECT
public:
    explicit CommitDetailWidget(QObject *parent = nullptr);

    void bind(QTextBrowser *browser);
    void setCommitList(const QList<CommitInfo> &commits);
    void setWorkspacePath(const QString &path);
    void showCommit(const CommitInfo &info);
    void clear();

private slots:
    void onLinkClicked(const QUrl &url);

private:
    QString resolveMessage(const QString &message) const;

    QTextBrowser *m_browser = nullptr;
    QList<CommitInfo> m_allCommits;
    QString m_workspacePath;
};

#endif // COMMITDETAILWIDGET_H
