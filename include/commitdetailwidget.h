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
    void showCommit(const CommitInfo &info);
    void clear();

private:
    QTextBrowser *m_browser = nullptr;
};

#endif // COMMITDETAILWIDGET_H
