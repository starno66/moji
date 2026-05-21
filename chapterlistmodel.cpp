#include "chapterlistmodel.h"

ChapterListModel::ChapterListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChapterListModel::rowCount(const QModelIndex &parent) const
{
    // 列表模型通常只有顶层行，所以 parent 有效时返回 0
    if (parent.isValid())
        return 0;
    return m_chapters.size();
}

QVariant ChapterListModel::data(const QModelIndex &index, int role) const
{
    // index 无效或超出范围 → 返回空
    if (!index.isValid() || index.row() < 0 || index.row() >= m_chapters.size())
        return {};

    // 显示角色: 返回章节名称
    if (role == Qt::DisplayRole) {
        return m_chapters.at(index.row());
    }

    return {};
}

void ChapterListModel::setChapters(const QStringList &chapters)
{
    /*
     * beginResetModel() / endResetModel()
     * 告诉 view "整个模型要刷新了"，view 会重新读取所有数据
     * 如果不调用这两个函数，view 不知道数据变了
     */
    beginResetModel();
    m_chapters = chapters;
    endResetModel();
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
