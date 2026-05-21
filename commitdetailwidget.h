#ifndef COMMITDETAILWIDGET_H
#define COMMITDETAILWIDGET_H

#include <QWidget>
#include <QLabel>
#include "gitmanager.h"

/*
 * 在 Designer 中，Commit 详情是用 QGroupBox + QFormLayout + 多个 QLabel 做的。
 * 这个类封装了对那些 QLabel 的赋值操作。
 *
 * 注意: 这个类不 new 任何 widget，而是通过 setupUi() 绑定 UI 文件中已创建好的控件。
 */

class CommitDetailWidget : public QObject
{
    Q_OBJECT
public:
    explicit CommitDetailWidget(QObject *parent = nullptr);

    // 绑定 UI 中的 label 指针
    void bind(QLabel *hashLabel,
              QLabel *authorLabel,
              QLabel *dateLabel,
              QLabel *messageLabel,
              QLabel *filesLabel);

    void showCommit(const CommitInfo &info);   // 显示 commit 信息
    void clear();                               // 清空

private:
    QLabel *m_hashLabel = nullptr;
    QLabel *m_authorLabel = nullptr;
    QLabel *m_dateLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QLabel *m_filesLabel = nullptr;
};

#endif // COMMITDETAILWIDGET_H
