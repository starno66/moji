#include "commitmodel.h"
#include <QApplication>
#include <QPainter>
#include <QRegularExpression>
#include <QStyle>

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
        int number = index.row() + 1;
        QString dateStr = commit.dateTime.toLocalTime().toString("yyyy-MM-dd hh:mm");
        QString message = commit.message;

        // 将"回退至 <hash>"动态替换为"回退至 [当前序号]"
        static const QRegularExpression re(QStringLiteral("回退至 ([0-9a-f]{7,})"));
        QRegularExpressionMatch match = re.match(message);
        if (match.hasMatch()) {
            QString targetHash = match.captured(1);
            for (int i = 0; i < m_commits.size(); ++i) {
                if (m_commits[i].hash.startsWith(targetHash)) {
                    message.replace(targetHash, QString("[%1]").arg(i + 1));
                    break;
                }
            }
        }

        return QString("[%1] %2  %3").arg(number).arg(dateStr, message);
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

// ==================== CommitDelegate ====================

void CommitDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();

    // 准备样式选项，文本置空以便只绘制背景
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.text.clear();

    // 只绘制背景（选中态、hover、焦点框等）
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, nullptr);

    if (text.isEmpty()) return;

    // 分割 [N] 和剩余部分
    int bracketEnd = text.indexOf(']');
    if (bracketEnd == -1) {
        painter->save();
        painter->setPen(opt.palette.text().color());
        painter->drawText(opt.rect.adjusted(4, 0, -4, 0),
                          Qt::AlignLeft | Qt::AlignVCenter, text);
        painter->restore();
        return;
    }

    QString prefix = text.left(bracketEnd + 1);   // "[N]"
    QString suffix = text.mid(bracketEnd + 1);     // 剩余文字

    QFont boldFont = opt.font;
    boldFont.setBold(true);

    QRect textRect = opt.rect.adjusted(4, 0, -4, 0);
    int prefixWidth = QFontMetrics(boldFont).horizontalAdvance(prefix);

    painter->save();
    painter->setPen(opt.palette.text().color());

    // 加粗序号
    painter->setFont(boldFont);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, prefix);

    // 剩余部分正常字重
    textRect.setLeft(textRect.left() + prefixWidth);
    painter->setFont(opt.font);
    QString elided = opt.fontMetrics.elidedText(suffix, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elided);

    painter->restore();
}
