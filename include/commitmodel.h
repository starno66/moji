#ifndef COMMITMODEL_H
#define COMMITMODEL_H

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QList>
#include "gitmanager.h"

class CommitModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit CommitModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setCommits(const QList<CommitInfo> &commits);
    CommitInfo commitAt(int row) const;
    void clear();

private:
    QList<CommitInfo> m_commits;
};

// 自定义 delegate：将 [N] 序号部分加粗显示
class CommitDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

#endif // COMMITMODEL_H
