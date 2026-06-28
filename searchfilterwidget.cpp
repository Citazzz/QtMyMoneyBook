#include "searchfilterwidget.h"
#include "databaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QDate>

SearchFilterWidget::SearchFilterWidget(int userId, QWidget *parent)
    : QWidget(parent), m_userId(userId)
{
    setupUI();
}

void SearchFilterWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    // 筛选条件
    auto *filterGroup = new QGroupBox(QStringLiteral("筛选条件"));
    auto *filterLayout = new QHBoxLayout(filterGroup);

    filterLayout->addWidget(new QLabel(QStringLiteral("关键字:")));
    m_keywordEdit = new QLineEdit;
    m_keywordEdit->setPlaceholderText(QStringLiteral("搜索备注..."));
    filterLayout->addWidget(m_keywordEdit);

    filterLayout->addWidget(new QLabel(QStringLiteral("类型:")));
    m_typeCombo = new QComboBox;
    m_typeCombo->addItem(QStringLiteral("全部"), "");
    m_typeCombo->addItem(QStringLiteral("收入"), "收入");
    m_typeCombo->addItem(QStringLiteral("支出"), "支出");
    filterLayout->addWidget(m_typeCombo);

    filterLayout->addWidget(new QLabel(QStringLiteral("分类:")));
    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem(QStringLiteral("全部分类"), -1);
    filterLayout->addWidget(m_categoryCombo);

    filterLayout->addWidget(new QLabel(QStringLiteral("开始:")));
    m_startDate = new QDateEdit(QDate::currentDate().addMonths(-1));
    m_startDate->setDisplayFormat("yyyy-MM-dd");
    m_startDate->setCalendarPopup(true);
    filterLayout->addWidget(m_startDate);

    filterLayout->addWidget(new QLabel(QStringLiteral("结束:")));
    m_endDate = new QDateEdit(QDate::currentDate());
    m_endDate->setDisplayFormat("yyyy-MM-dd");
    m_endDate->setCalendarPopup(true);
    filterLayout->addWidget(m_endDate);

    m_searchBtn = new QPushButton(QStringLiteral("搜索"));
    m_resetBtn = new QPushButton(QStringLiteral("重置"));
    filterLayout->addWidget(m_searchBtn);
    filterLayout->addWidget(m_resetBtn);

    mainLayout->addWidget(filterGroup);

    // 结果表格
    m_table = new QTableWidget;
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("ID"), QStringLiteral("日期"), QStringLiteral("类型"),
        QStringLiteral("分类"), QStringLiteral("账户"), QStringLiteral("金额"), QStringLiteral("备注")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setColumnHidden(0, true);
    mainLayout->addWidget(m_table);

    connect(m_searchBtn, &QPushButton::clicked, this, &SearchFilterWidget::onSearch);
    connect(m_resetBtn, &QPushButton::clicked, this, [this]() {
        m_keywordEdit->clear();
        m_typeCombo->setCurrentIndex(0);
        m_categoryCombo->setCurrentIndex(0);
        m_startDate->setDate(QDate::currentDate().addMonths(-1));
        m_endDate->setDate(QDate::currentDate());
        m_table->setRowCount(0);
    });

    // 加载分类列表
    auto categories = databaseManager::instance().getCategories(m_userId);
    for (const auto &c : categories) {
        QString text = c.icon.isEmpty() ? c.name : (c.icon + " " + c.name);
        m_categoryCombo->addItem(QStringLiteral("[%1] %2").arg(c.type, text), c.id);
    }
}

void SearchFilterWidget::onSearch()
{
    QString keyword = m_keywordEdit->text().trimmed();
    QString type = m_typeCombo->currentData().toString();
    int categoryId = m_categoryCombo->currentData().toInt();
    QString startDate = m_startDate->date().toString("yyyy-MM-dd");
    QString endDate = m_endDate->date().toString("yyyy-MM-dd");

    auto bills = databaseManager::instance().searchBills(m_userId, keyword, startDate, endDate, type, categoryId);

    m_table->setRowCount(0);
    double totalIncome = 0, totalExpense = 0;

    for (const auto &b : bills) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(b.id)));
        m_table->setItem(row, 1, new QTableWidgetItem(b.date));
        m_table->setItem(row, 2, new QTableWidgetItem(b.type));
        m_table->setItem(row, 3, new QTableWidgetItem(b.categoryName));
        m_table->setItem(row, 4, new QTableWidgetItem(b.accountName));
        m_table->setItem(row, 5, new QTableWidgetItem(QString::number(b.amount, 'f', 2)));
        m_table->setItem(row, 6, new QTableWidgetItem(b.remark));

        QColor color = (b.type == "收入") ? QColor("#00C897") : QColor("#FF6B6B");
        m_table->item(row, 5)->setForeground(color);

        if (b.type == "收入") totalIncome += b.amount;
        else totalExpense += b.amount;
    }

    // 底部汇总
    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem(""));
    m_table->setItem(row, 1, new QTableWidgetItem(""));
    auto *labelItem = new QTableWidgetItem(QStringLiteral("合计"));
    labelItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, 2, labelItem);
    m_table->setItem(row, 3, new QTableWidgetItem(QStringLiteral("收入: ¥%1  支出: ¥%2").arg(totalIncome, 0, 'f', 2).arg(totalExpense, 0, 'f', 2)));
    m_table->setItem(row, 4, new QTableWidgetItem(""));
    m_table->setItem(row, 5, new QTableWidgetItem(QString::number(totalIncome - totalExpense, 'f', 2)));
    m_table->setItem(row, 6, new QTableWidgetItem(""));

    // 高亮汇总行
    for (int c = 0; c < m_table->columnCount(); ++c) {
        if (m_table->item(row, c))
            m_table->item(row, c)->setBackground(QColor("#F0F2F5"));
    }
}
