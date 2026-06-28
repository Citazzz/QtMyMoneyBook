#ifndef BILLWIDGET_H
#define BILLWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>

class BillWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BillWidget(int userId, QWidget *parent = nullptr);

    void refresh();
    void loadAccounts();

signals:
    void billsChanged();

private slots:
    void onAddBill();
    void onEditBill();
    void onDeleteBill();
    void onBillSelected();
    void onExportCSV();

private:
    void setupUI();
    void loadCategories(const QString &type);

    int m_userId;

    QTableWidget *m_table;
    QComboBox *m_typeCombo;
    QComboBox *m_categoryCombo;
    QComboBox *m_accountCombo;
    QDateEdit *m_dateEdit;
    QDoubleSpinBox *m_amountSpin;
    QLineEdit *m_remarkEdit;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QLabel *m_incomeLabel;
    QLabel *m_expenseLabel;
    QLabel *m_balanceLabel;
};

#endif // BILLWIDGET_H
