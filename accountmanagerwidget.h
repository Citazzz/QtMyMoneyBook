#ifndef ACCOUNTMANAGERWIDGET_H
#define ACCOUNTMANAGERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>

class AccountManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AccountManagerWidget(int userId, QWidget *parent = nullptr);
    void refresh();

signals:
    void accountsChanged();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSelectionChanged();

private:
    void setupUI();

    int m_userId;
    QTableWidget *m_table;
    QLineEdit *m_nameEdit;
    QComboBox *m_typeCombo;
    QDoubleSpinBox *m_balanceSpin;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
};

#endif // ACCOUNTMANAGERWIDGET_H