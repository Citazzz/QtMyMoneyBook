#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

    int userId() const { return m_userId; }

private slots:
    void onLogin();
    void onRegister();
    void switchToLogin();
    void switchToRegister();

private:
    void setupLoginPage();
    void setupRegisterPage();

    QStackedWidget *m_stack;

    // Login page
    QLineEdit *m_loginUser;
    QLineEdit *m_loginPass;
    QLabel *m_loginMsg;

    // Register page
    QLineEdit *m_regUser;
    QLineEdit *m_regPass;
    QLineEdit *m_regPassConfirm;
    QLabel *m_regMsg;

    int m_userId = -1;
};

#endif // LOGINDIALOG_H