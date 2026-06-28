#ifndef BUDGETWIDGET_H
#define BUDGETWIDGET_H

#include <QWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QDateEdit>

class BudgetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BudgetWidget(int userId, QWidget *parent = nullptr);
    void refresh();

private slots:
    void onSetBudget();

private:
    void setupUI();

    int m_userId;
    QDateEdit *m_monthEdit;
    QDoubleSpinBox *m_budgetSpin;
    QPushButton *m_setBtn;
    QLabel *m_budgetLabel;
    QLabel *m_spentLabel;
    QLabel *m_remainLabel;
    QProgressBar *m_progressBar;
};

#endif // BUDGETWIDGET_H