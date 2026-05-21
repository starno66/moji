#include "commitdetailwidget.h"

CommitDetailWidget::CommitDetailWidget(QObject *parent)
    : QObject(parent)
{
}

void CommitDetailWidget::bind(QLabel *hashLabel,
                               QLabel *authorLabel,
                               QLabel *dateLabel,
                               QLabel *messageLabel,
                               QLabel *filesLabel)
{
    m_hashLabel = hashLabel;
    m_authorLabel = authorLabel;
    m_dateLabel = dateLabel;
    m_messageLabel = messageLabel;
    m_filesLabel = filesLabel;
}

void CommitDetailWidget::showCommit(const CommitInfo &info)
{
    if (!m_hashLabel) return;

    m_hashLabel->setText(info.hash);
    m_authorLabel->setText(info.author);
    m_dateLabel->setText(info.dateTime.toLocalTime().toString("yyyy-MM-dd hh:mm:ss"));
    m_messageLabel->setText(info.message);
    m_filesLabel->setText(info.files.join('\n'));
}

void CommitDetailWidget::clear()
{
    if (!m_hashLabel) return;

    m_hashLabel->clear();
    m_authorLabel->clear();
    m_dateLabel->clear();
    m_messageLabel->clear();
    m_filesLabel->clear();
}
