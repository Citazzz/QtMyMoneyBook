#include "mainwindow.h"
#include "billwidget.h"
#include "categorymanagerwidget.h"
#include "budgetwidget.h"
#include "searchfilterwidget.h"
#include "visualizationwidget.h"
#include "accountmanagerwidget.h"
#include "databaseManager.h"

#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(int userId, QWidget *parent)
    : QMainWindow(parent), m_userId(userId)
{
    setupUI();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    QString username = databaseManager::instance().currentUsername();
    setWindowTitle(QStringLiteral("个人记账 - %1").arg(username));
    resize(1000, 700);

    m_tabs = new QTabWidget;
    setCentralWidget(m_tabs);

    m_billWidget = new BillWidget(m_userId);
    m_tabs->addTab(m_billWidget, QStringLiteral("账单管理"));

    m_categoryWidget = new CategoryManagerWidget(m_userId);
    m_tabs->addTab(m_categoryWidget, QStringLiteral("分类管理"));

    m_budgetWidget = new BudgetWidget(m_userId);
    m_tabs->addTab(m_budgetWidget, QStringLiteral("预算管理"));

    m_accountWidget = new AccountManagerWidget(m_userId);
    m_tabs->addTab(m_accountWidget, QStringLiteral("账户管理"));

    m_searchWidget = new SearchFilterWidget(m_userId);
    m_tabs->addTab(m_searchWidget, QStringLiteral("搜索筛选"));

    m_vizWidget = new VisualizationWidget(m_userId);
    m_tabs->addTab(m_vizWidget, QStringLiteral("数据可视化"));

    connect(m_billWidget, &BillWidget::billsChanged, this, [this]() {
        m_budgetWidget->refresh();
    });

    connect(m_accountWidget, &AccountManagerWidget::accountsChanged, this, [this]() {
        m_billWidget->loadAccounts();
    });

    // 注销账户菜单
    auto *accountMenu = menuBar()->addMenu(QStringLiteral("账户"));
    auto *deleteAcctAction = accountMenu->addAction(QStringLiteral("注销账户"));
    deleteAcctAction->setToolTip(QStringLiteral("永久删除所有数据并退出"));
    connect(deleteAcctAction, &QAction::triggered, this, &MainWindow::onDeleteAccount);

    statusBar()->showMessage(QStringLiteral("就绪  |  欢迎 %1").arg(username));
}

void MainWindow::onDeleteAccount()
{
    auto reply = QMessageBox::warning(this,
        QStringLiteral("注销账户"),
        QStringLiteral("确定要注销账户吗？\n\n"
                       "此操作将永久删除您的所有数据，包括：\n"
                       "• 所有账单记录\n"
                       "• 所有账户及余额\n"
                       "• 所有自定义分类\n"
                       "• 所有预算设置\n\n"
                       "删除后可使用相同用户名重新注册。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    // 二次确认
    auto reply2 = QMessageBox::question(this,
        QStringLiteral("二次确认"),
        QStringLiteral("此操作不可撤销！确定继续吗？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply2 != QMessageBox::Yes)
        return;

    databaseManager::instance().deleteUser(m_userId);
    emit loggedOut();
    close();
    QApplication::exit(100);
}
