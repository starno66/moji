#ifndef COMMITMODEL_H
#define COMMITMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "gitmanager.h"    // 需要 CommitInfo 结构体

/*
 * 和 ChapterListModel 类似，但每行显示的是 commit 信息。
 *
 * 不需要内置筛选功能。
 * 筛选由 MainWindow 负责：选不同章节时，用 GitManager::log(folder) 重新查询。
 */

class CommitModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit CommitModel(QObject *parent = nullptr);

    // ====== QAbstractListModel 接口 ======
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // ====== 数据管理 ======
    void setCommits(const QList<CommitInfo> &commits);
    CommitInfo commitAt(int row) const;
    void clear();

private:
    QList<CommitInfo> m_commits;
};

#endif // COMMITMODEL_H
