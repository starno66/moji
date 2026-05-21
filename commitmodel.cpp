#include "commitmodel.h"

CommitModel::CommitModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CommitModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_commits.size();
}

QVariant CommitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_commits.size())
        return {};

    const auto &commit = m_commits.at(index.row());

    if (role == Qt::DisplayRole) {
        // 显示格式: [abc1234] 2025-01-15 10:30  ch01: 修改引言
        QString shortHash = commit.hash.left(7);  // 取前 7 位
        QString dateStr = commit.dateTime.toLocalTime().toString("yyyy-MM-dd hh:mm");
        return QString("[%1] %2  %3").arg(shortHash, dateStr, commit.message);
    }

    return {};
}

void CommitModel::setCommits(const QList<CommitInfo> &commits)
{
    beginResetModel();
    m_commits = commits;
    endResetModel();
}

CommitInfo CommitModel::commitAt(int row) const
{
    if (row < 0 || row >= m_commits.size())
        return {};
    return m_commits.at(row);
}

void CommitModel::clear()
{
    beginResetModel();
    m_commits.clear();
    endResetModel();
}
