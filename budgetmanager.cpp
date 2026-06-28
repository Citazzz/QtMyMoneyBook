#include "budgetwidget.h"
#include "databaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDate>

BudgetWidget::BudgetWidget(int userId, QWidget *parent)
    : QWidget(parent), m_userId(userId)
{
    setupUI();
    refresh();
}

void BudgetWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    // 设置预算
    auto *inputGroup = new QGroupBox(QStringLiteral("设置月度预算"));
    auto *inputLayout = new QHBoxLayout(inputGroup);

    inputLayout->addWidget(new QLabel(QStringLiteral("月份:")));
    m_monthEdit = new QDateEdit(QDate::currentDate());
    m_monthEdit->setDisplayFormat("yyyy-MM");
    m_monthEdit->setCalendarPopup(true);
    inputLayout->addWidget(m_monthEdit);

    inputLayout->addWidget(new QLabel(QStringLiteral("预算金额:")));
    m_budgetSpin = new QDoubleSpinBox;
    m_budgetSpin->setRange(0, 99999999.99);
    m_budgetSpin->setDecimals(2);
    m_budgetSpin->setPrefix("¥");
    m_budgetSpin->setValue(3000);
    inputLayout->addWidget(m_budgetSpin);

    m_setBtn = new QPushButton(QStringLiteral("设置预算"));
    inputLayout->addWidget(m_setBtn);

    mainLayout->addWidget(inputGroup);

    // 预算状态
    auto *statusGroup = new QGroupBox(QStringLiteral("本月预算状态"));
    auto *statusLayout = new QVBoxLayout(statusGroup);

    m_budgetLabel = new QLabel(QStringLiteral("月度预算: ¥0.00"));
    m_budgetLabel->setStyleSheet("font-size:14px; font-weight:bold;");
    m_spentLabel = new QLabel(QStringLiteral("已支出: ¥0.00"));
    m_spentLabel->setStyleSheet("font-size:14px;");
    m_remainLabel = new QLabel(QStringLiteral("剩余: ¥0.00"));
    m_remainLabel->setStyleSheet("font-size:14px;");

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%p%");
    m_progressBar->setMinimumHeight(25);

    statusLayout->addWidget(m_budgetLabel);
    statusLayout->addWidget(m_spentLabel);
    statusLayout->addWidget(m_remainLabel);
    statusLayout->addWidget(m_progressBar);

    mainLayout->addWidget(statusGroup);
    mainLayout->addStretch();

    connect(m_setBtn, &QPushButton::clicked, this, &BudgetWidget::onSetBudget);
}

void BudgetWidget::refresh()
{
    QString month = QDate::currentDate().toString("yyyy-MM");
    auto budget = databaseManager::instance().getBudget(m_userId, month);
    double spent = databaseManager::instance().getMonthExpense(m_userId, month);

    m_budgetLabel->setText(QStringLiteral("月度预算: ¥%1").arg(budget.amount, 0, 'f', 2));
    m_spentLabel->setText(QStringLiteral("已支出: ¥%1").arg(spent, 0, 'f', 2));
    m_remainLabel->setText(QStringLiteral("剩余: ¥%1").arg(budget.amount - spent, 0, 'f', 2));

    int percent = (budget.amount > 0) ? qMin(100, (int)(spent / budget.amount * 100)) : 0;
    m_progressBar->setValue(percent);

    if (percent >= 100) {
        m_progressBar->setStyleSheet("QProgressBar::chunk { background-color: #FF6B6B; }");
        m_remainLabel->setStyleSheet("font-size:14px; color:#FF6B6B; font-weight:bold;");
    } else if (percent >= 80) {
        m_progressBar->setStyleSheet("QProgressBar::chunk { background-color: #F59E0B; }");
        m_remainLabel->setStyleSheet("font-size:14px; color:#F59E0B; font-weight:bold;");
    } else {
        m_progressBar->setStyleSheet("QProgressBar::chunk { background-color: #00C897; }");
        m_remainLabel->setStyleSheet("font-size:14px; color:#00C897;");
    }
}

void BudgetWidget::onSetBudget()
{
    QString month = m_monthEdit->date().toString("yyyy-MM");
    double amount = m_budgetSpin->value();
    databaseManager::instance().setBudget(m_userId, month, amount);
    refresh();
}