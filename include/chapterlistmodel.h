#ifndef CHAPTERLISTMODEL_H
#define CHAPTERLISTMODEL_H

#include <QAbstractListModel>
#include <QStringList>

/*
 * QAbstractListModel 是 Qt 模型/视图架构中的列表模型基类。
 *
 * 我们继承它，让 QListView 可以直接显示章节列表。
 * 只需要实现两个纯虚函数:
 *   - rowCount(): 告诉 view 有多少行
 *   - data():     告诉 view 每行显示什么内容
 */

class ChapterListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ChapterListModel(QObject *parent = nullptr);

    // ====== 必须实现的 QAbstractListModel 接口 ======
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // ====== 数据管理方法 ======
    void setChapters(const QStringList &chapters);  // 更新全部章节列表
    QStringList chapters() const;                    // 获取当前章节列表
    QString chapterAt(int row) const;                // 获取指定行的章节名

private:
    QStringList m_chapters;  // 内部存储：章节名称列表
};

#endif // CHAPTERLISTMODEL_H
