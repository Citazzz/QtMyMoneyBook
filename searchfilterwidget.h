#ifndef SEARCHFILTERWIDGET_H
#define SEARCHFILTERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>

class SearchFilterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchFilterWidget(int userId, QWidget *parent = nullptr);

private slots:
    void onSearch();

private:
    void setupUI();

    int m_userId;
    QLineEdit *m_keywordEdit;
    QDateEdit *m_startDate;
    QDateEdit *m_endDate;
    QComboBox *m_typeCombo;
    QComboBox *m_categoryCombo;
    QPushButton *m_searchBtn;
    QPushButton *m_resetBtn;
    QTableWidget *m_table;
};

#endif // SEARCHFILTERWIDGET_H