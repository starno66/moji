#ifndef AIDIALOG_H
#define AIDIALOG_H

#include <QDialog>
#include <QJsonArray>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextBrowser;
class QTextEdit;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;

class AiDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AiDialog(QWidget *parent = nullptr);
    ~AiDialog() override;

    void setCommitContext(const QString &context);

protected:
    void closeEvent(QCloseEvent *e) override;

private slots:
    void onSend();
    void onReadyRead();
    void onReplyFinished(QNetworkReply *reply);

private:
    void addBubble(const QString &role, const QString &text);
    void renderHtml();
    static QString formatContent(const QString &role, const QString &text);

    QLineEdit  *m_apiKeyEdit;
    QComboBox  *m_modelCombo;
    QTextBrowser *m_chatView;
    QTextEdit  *m_inputEdit;
    QPushButton *m_sendBtn;
    QLabel     *m_statusLabel;

    QNetworkAccessManager *m_net;
    QNetworkReply *m_reply = nullptr;
    QJsonArray m_messages;
    QString m_chatHtml;
    QString m_streamBuffer;
    QString m_streamContent;
    bool m_busy = false;
    bool m_streaming = false;
};

#endif
