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

QString CommitDelegate::s_currentBranch = "main";
QStringList CommitDelegate::s_branchOnlyHashes;

void CommitDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    if (text.isEmpty()) return;

    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    // --- 卡片背景 ---
    bool selected = option.state & QStyle::State_Selected;
    QRect cardRect = option.rect.adjusted(2, 2, -2, -2);

    if (selected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#eef2ff"));
        painter->drawRoundedRect(cardRect, 8, 8);
    }

    // --- 文字颜色 ---
    QColor textColor = option.palette.text().color();
    if (s_currentBranch != "main" && !s_branchOnlyHashes.isEmpty()) {
        const CommitModel *model = static_cast<const CommitModel *>(index.model());
        CommitInfo ci = model->commitAt(index.row());
        QString hash7 = ci.hash.left(7);
        for (const QString &h : s_branchOnlyHashes) {
            if (h.startsWith(hash7) || hash7 == h.left(7)) {
                textColor = QColor("#cf222e");
                break;
            }
        }
    }
    if (selected && textColor == option.palette.text().color())
        textColor = QColor("#4338ca");

    // --- 解析 [序号] 前缀 ---
    int bracketEnd = text.indexOf(']');
    if (bracketEnd == -1) {
        painter->setPen(textColor);
        painter->drawText(cardRect.adjusted(4, 0, -4, 0),
                          Qt::AlignLeft | Qt::AlignVCenter, text);
        painter->restore();
        return;
    }

    QString prefix = text.left(bracketEnd + 1);
    QString suffix = text.mid(bracketEnd + 1);

    QFont boldFont = option.font;
    boldFont.setBold(true);

    QRect textRect = cardRect.adjusted(4, 0, -4, 0);
    QFontMetrics fmBold(boldFont);
    int prefixWidth = fmBold.horizontalAdvance(prefix);

    // 画加粗序号
    QRect prefixRect = textRect;
    prefixRect.setWidth(prefixWidth);
    painter->setFont(boldFont);
    painter->setPen(textColor);
    painter->drawText(prefixRect, Qt::AlignLeft | Qt::AlignVCenter, prefix);

    // 画剩余文字
    QRect suffixRect = textRect;
    suffixRect.setLeft(textRect.left() + prefixWidth);
    painter->setFont(option.font);
    QString elided = option.fontMetrics.elidedText(suffix, Qt::ElideRight, suffixRect.width());
    painter->drawText(suffixRect, Qt::AlignLeft | Qt::AlignVCenter, elided);

    painter->restore();
}
