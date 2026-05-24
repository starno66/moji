#ifndef ENVIRONMENTSETUPDIALOG_H
#define ENVIRONMENTSETUPDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;
class QLabel;

class EnvironmentSetupDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EnvironmentSetupDialog(QWidget *parent = nullptr);

private slots:
    void onInstallGit();
    void onSaveConfig();

private:
    void updateState();

    QLabel *m_statusLabel;
    QPushButton *m_installBtn;
    QLineEdit *m_nameEdit;
    QLineEdit *m_emailEdit;
    QPushButton *m_saveBtn;
    bool m_gitInstalled = false;
};

#endif // ENVIRONMENTSETUPDIALOG_H
