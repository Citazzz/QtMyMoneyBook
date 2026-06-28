#ifndef CATEGORYMANAGERWIDGET_H
#define CATEGORYMANAGERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

class CategoryManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CategoryManagerWidget(int userId, QWidget *parent = nullptr);
    void refresh();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSelectionChanged();
    void onPickIcon();

private:
    void setupUI();
    QString pickIcon();

    int m_userId;
    QString m_currentIcon;
    QTableWidget *m_table;
    QLineEdit *m_nameEdit;
    QComboBox *m_typeCombo;
    QPushButton *m_iconBtn;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
};

#endif // CATEGORYMANAGERWIDGET_H