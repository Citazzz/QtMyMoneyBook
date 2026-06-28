#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>

class BillWidget;
class CategoryManagerWidget;
class BudgetWidget;
class SearchFilterWidget;
class VisualizationWidget;
class AccountManagerWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int userId, QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void loggedOut();

private slots:
    void onDeleteAccount();

private:
    void setupUI();

    int m_userId;
    QTabWidget *m_tabs;

    BillWidget *m_billWidget;
    CategoryManagerWidget *m_categoryWidget;
    BudgetWidget *m_budgetWidget;
    SearchFilterWidget *m_searchWidget;
    VisualizationWidget *m_vizWidget;
    AccountManagerWidget *m_accountWidget;
};

#endif // MAINWINDOW_H
