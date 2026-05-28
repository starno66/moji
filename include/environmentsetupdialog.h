#ifndef ENVIRONMENTSETUPDIALOG_H
#define ENVIRONMENTSETUPDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;
class QLabel;
class QProgressBar;
class QProcess;
class QTimer;

class EnvironmentSetupDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EnvironmentSetupDialog(QWidget *parent = nullptr);

protected:
    void reject() override;

private slots:
    void onInstallGit();
    void onInstallProgress();
    void onInstallFinished(int exitCode);
    void onProgressTimer();
    void onSaveConfig();

private:
    void updateState();

    QLabel *m_statusLabel;
    QPushButton *m_installBtn;
    QLineEdit *m_nameEdit;
    QLineEdit *m_emailEdit;
    QPushButton *m_saveBtn;
    QProgressBar *m_progressBar;
    QProcess *m_installProcess = nullptr;
    QTimer *m_progressTimer;
    int m_progressDot = 0;
    QString m_outputBuffer;
    bool m_gitInstalled = false;
    bool m_installing = false;
};

#endif // ENVIRONMENTSETUPDIALOG_H
