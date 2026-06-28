#include "LoginDialog.h"
#include "databaseManager.h"
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

static void styleInput(QLineEdit *edit)
{
    edit->setMinimumHeight(36);
    edit->setStyleSheet("QLineEdit{font-size:14px; padding:6px 10px;}");
}

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("个人记账 - 登录"));
    setFixedSize(420, 400);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("个人记账系统"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #5B7FFF; margin-top: 8px;");
    mainLayout->addWidget(title);

    auto *subtitle = new QLabel(QStringLiteral("简洁高效的个人财务管理工具"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("color: #8E93A0; font-size: 12px; margin-bottom: 4px;");
    mainLayout->addWidget(subtitle);

    m_stack = new QStackedWidget;
    mainLayout->addWidget(m_stack);

    setupLoginPage();
    setupRegisterPage();

    m_stack->setCurrentIndex(0);
}

void LoginDialog::setupLoginPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    auto *group = new QGroupBox(QStringLiteral("账号登录"));
    auto *form = new QFormLayout(group);
    form->setSpacing(12);
    form->setContentsMargins(16, 20, 16, 16);

    m_loginUser = new QLineEdit;
    m_loginUser->setPlaceholderText(QStringLiteral("请输入用户名"));
    styleInput(m_loginUser);
    form->addRow(QStringLiteral("用户名:"), m_loginUser);

    m_loginPass = new QLineEdit;
    m_loginPass->setPlaceholderText(QStringLiteral("请输入密码"));
    m_loginPass->setEchoMode(QLineEdit::Password);
    styleInput(m_loginPass);
    form->addRow(QStringLiteral("密  码:"), m_loginPass);

    layout->addWidget(group);

    m_loginMsg = new QLabel;
    m_loginMsg->setStyleSheet("color: #FF6B6B; font-size: 12px; min-height: 18px;");
    m_loginMsg->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_loginMsg);

    auto *btnLogin = new QPushButton(QStringLiteral("登  录"));
    btnLogin->setMinimumHeight(40);
    btnLogin->setStyleSheet("QPushButton{font-size:15px; font-weight:600;}");
    layout->addWidget(btnLogin);

    auto *btnToReg = new QPushButton(QStringLiteral("没有账号？点击注册"));
    btnToReg->setFlat(true);
    layout->addWidget(btnToReg);

    connect(btnLogin, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(btnToReg, &QPushButton::clicked, this, &LoginDialog::switchToRegister);
    connect(m_loginPass, &QLineEdit::returnPressed, this, &LoginDialog::onLogin);

    m_stack->addWidget(page);
}

void LoginDialog::setupRegisterPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    auto *group = new QGroupBox(QStringLiteral("注册新账号"));
    auto *form = new QFormLayout(group);
    form->setSpacing(12);
    form->setContentsMargins(16, 20, 16, 16);

    m_regUser = new QLineEdit;
    m_regUser->setPlaceholderText(QStringLiteral("请输入用户名"));
    styleInput(m_regUser);
    form->addRow(QStringLiteral("用户名:"), m_regUser);

    m_regPass = new QLineEdit;
    m_regPass->setPlaceholderText(QStringLiteral("请输入密码"));
    m_regPass->setEchoMode(QLineEdit::Password);
    styleInput(m_regPass);
    form->addRow(QStringLiteral("密  码:"), m_regPass);

    m_regPassConfirm = new QLineEdit;
    m_regPassConfirm->setPlaceholderText(QStringLiteral("请再次输入密码"));
    m_regPassConfirm->setEchoMode(QLineEdit::Password);
    styleInput(m_regPassConfirm);
    form->addRow(QStringLiteral("确认密码:"), m_regPassConfirm);

    layout->addWidget(group);

    m_regMsg = new QLabel;
    m_regMsg->setStyleSheet("color: #FF6B6B; font-size: 12px; min-height: 18px;");
    m_regMsg->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_regMsg);

    auto *btnReg = new QPushButton(QStringLiteral("注  册"));
    btnReg->setMinimumHeight(40);
    btnReg->setStyleSheet("QPushButton{font-size:15px; font-weight:600;}");
    layout->addWidget(btnReg);

    auto *btnToLogin = new QPushButton(QStringLiteral("已有账号？返回登录"));
    btnToLogin->setFlat(true);
    layout->addWidget(btnToLogin);

    connect(btnReg, &QPushButton::clicked, this, &LoginDialog::onRegister);
    connect(btnToLogin, &QPushButton::clicked, this, &LoginDialog::switchToLogin);
    connect(m_regPassConfirm, &QLineEdit::returnPressed, this, &LoginDialog::onRegister);

    m_stack->addWidget(page);
}

void LoginDialog::onLogin()
{
    QString user = m_loginUser->text().trimmed();
    QString pass = m_loginPass->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_loginMsg->setText(QStringLiteral("用户名和密码不能为空"));
        return;
    }

    int uid = databaseManager::instance().loginUser(user, pass);
    if (uid > 0) {
        m_userId = uid;
        accept();
    } else {
        m_loginMsg->setText(QStringLiteral("用户名或密码错误"));
    }
}

void LoginDialog::onRegister()
{
    QString user = m_regUser->text().trimmed();
    QString pass = m_regPass->text();
    QString confirm = m_regPassConfirm->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_regMsg->setText(QStringLiteral("用户名和密码不能为空"));
        return;
    }
    if (user.length() < 3) {
        m_regMsg->setText(QStringLiteral("用户名至少需要3个字符"));
        return;
    }
    if (pass.length() < 4) {
        m_regMsg->setText(QStringLiteral("密码至少需要4个字符"));
        return;
    }
    if (pass != confirm) {
        m_regMsg->setText(QStringLiteral("两次输入的密码不一致"));
        return;
    }

    int uid = databaseManager::instance().registerUser(user, pass);
    if (uid > 0) {
        m_userId = uid;
        accept();
    } else {
        m_regMsg->setText(QStringLiteral("注册失败，用户名可能已存在"));
    }
}

void LoginDialog::switchToLogin()
{
    m_stack->setCurrentIndex(0);
    m_loginMsg->clear();
}

void LoginDialog::switchToRegister()
{
    m_stack->setCurrentIndex(1);
    m_regMsg->clear();
}
