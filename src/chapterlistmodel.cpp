#include "chapterlistmodel.h"
#include <QColor>

ChapterListModel::ChapterListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChapterListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_chapters.size();
}

QVariant ChapterListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_chapters.size())
        return {};

    QString name = m_chapters.at(index.row());

    if (role == Qt::DisplayRole) {
        if (m_dirtyChapters.contains(name))
            return name + "  ●";
        return name;
    }

    if (role == Qt::ForegroundRole) {
        if (m_dirtyChapters.contains(name))
            return QColor("#f59e0b");
        return {};
    }

    return {};
}

void ChapterListModel::setChapters(const QStringList &chapters)
{
    beginResetModel();
    m_chapters = chapters;
    endResetModel();
}

void ChapterListModel::setDirtyChapters(const QSet<QString> &dirty)
{
    if (m_dirtyChapters == dirty) return;  // 无变化则跳过
    m_dirtyChapters = dirty;
    if (!m_chapters.isEmpty())
        emit dataChanged(index(0), index(m_chapters.size() - 1),
                         {Qt::DisplayRole, Qt::ForegroundRole});
}

QStringList ChapterListModel::chapters() const
{
    return m_chapters;
}

QString ChapterListModel::chapterAt(int row) const
{
    if (row < 0 || row >= m_chapters.size())
        return {};
    return m_chapters.at(row);
}
