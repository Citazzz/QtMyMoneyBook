#include "billwidget.h"
#include "databaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDate>

BillWidget::BillWidget(int userId, QWidget *parent)
    : QWidget(parent), m_userId(userId)
{
    setupUI();
    refresh();
}

void BillWidget::loadAccounts()
{
    m_accountCombo->clear();
    auto accounts = databaseManager::instance().getAccounts(m_userId);
    for (const auto &a : accounts)
        m_accountCombo->addItem(a.name, a.id);
}

void BillWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    // === 汇总信息 ===
    auto *summaryLayout = new QHBoxLayout;
    m_incomeLabel = new QLabel(QStringLiteral("收入: ¥0.00"));
    m_incomeLabel->setStyleSheet("font-size:14px; color:#00C897; font-weight:bold;");
    m_expenseLabel = new QLabel(QStringLiteral("支出: ¥0.00"));
    m_expenseLabel->setStyleSheet("font-size:14px; color:#FF6B6B; font-weight:bold;");
    m_balanceLabel = new QLabel(QStringLiteral("结余: ¥0.00"));
    m_balanceLabel->setStyleSheet("font-size:14px; color:#5B7FFF; font-weight:bold;");
    summaryLayout->addWidget(m_incomeLabel);
    summaryLayout->addWidget(m_expenseLabel);
    summaryLayout->addWidget(m_balanceLabel);
    summaryLayout->addStretch();
    mainLayout->addLayout(summaryLayout);

    // === 输入区域 ===
    auto *inputGroup = new QGroupBox(QStringLiteral("记账"));
    auto *inputLayout = new QHBoxLayout(inputGroup);

    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({QStringLiteral("支出"), QStringLiteral("收入")});
    inputLayout->addWidget(new QLabel(QStringLiteral("类型:")));
    inputLayout->addWidget(m_typeCombo);

    m_dateEdit = new QDateEdit(QDate::currentDate());
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    inputLayout->addWidget(new QLabel(QStringLiteral("日期:")));
    inputLayout->addWidget(m_dateEdit);

    m_accountCombo = new QComboBox;
    m_accountCombo->setMinimumWidth(100);
    inputLayout->addWidget(new QLabel(QStringLiteral("账户:")));
    inputLayout->addWidget(m_accountCombo);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->setMinimumWidth(120);
    inputLayout->addWidget(new QLabel(QStringLiteral("分类:")));
    inputLayout->addWidget(m_categoryCombo);

    m_amountSpin = new QDoubleSpinBox;
    m_amountSpin->setRange(0.00, 99999999.99);
    m_amountSpin->setDecimals(2);
    m_amountSpin->setPrefix("¥");
    m_amountSpin->setValue(0.00);
    inputLayout->addWidget(new QLabel(QStringLiteral("金额:")));
    inputLayout->addWidget(m_amountSpin);

    m_remarkEdit = new QLineEdit;
    m_remarkEdit->setPlaceholderText(QStringLiteral("备注（可选）"));
    m_remarkEdit->setMaximumWidth(150);
    inputLayout->addWidget(new QLabel(QStringLiteral("备注:")));
    inputLayout->addWidget(m_remarkEdit);

    m_addBtn = new QPushButton(QStringLiteral("添加"));
    m_editBtn = new QPushButton(QStringLiteral("修改"));
    m_deleteBtn = new QPushButton(QStringLiteral("删除"));
    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    inputLayout->addWidget(m_addBtn);
    inputLayout->addWidget(m_editBtn);
    inputLayout->addWidget(m_deleteBtn);

    mainLayout->addWidget(inputGroup);

    // === 账单列表 ===
    m_table = new QTableWidget;
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("ID"), QStringLiteral("日期"), QStringLiteral("类型"),
        QStringLiteral("分类"), QStringLiteral("账户"), QStringLiteral("金额"), QStringLiteral("备注")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setColumnHidden(0, true);
    mainLayout->addWidget(m_table);

    // === 导出 ===
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    auto *exportBtn = new QPushButton(QStringLiteral("导出CSV"));
    bottomLayout->addWidget(exportBtn);
    mainLayout->addLayout(bottomLayout);

    // === 信号 ===
    connect(m_typeCombo, &QComboBox::currentTextChanged, this, [this](const QString &type) {
        loadCategories(type);
        m_amountSpin->setValue(0.00);
        m_remarkEdit->clear();
    });
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        m_amountSpin->setValue(0.00);
        m_remarkEdit->clear();
    });
    connect(m_addBtn, &QPushButton::clicked, this, &BillWidget::onAddBill);
    connect(m_editBtn, &QPushButton::clicked, this, &BillWidget::onEditBill);
    connect(m_deleteBtn, &QPushButton::clicked, this, &BillWidget::onDeleteBill);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &BillWidget::onBillSelected);
    connect(exportBtn, &QPushButton::clicked, this, &BillWidget::onExportCSV);

    loadAccounts();
    loadCategories(m_typeCombo->currentText());
}

void BillWidget::loadCategories(const QString &type)
{
    m_categoryCombo->clear();
    auto categories = databaseManager::instance().getCategoriesByType(m_userId, type);
    for (const auto &c : categories) {
        QString text = c.icon.isEmpty() ? c.name : (c.icon + " " + c.name);
        m_categoryCombo->addItem(text, c.id);
    }
}

void BillWidget::refresh()
{
    loadAccounts();

    QString currentType = m_typeCombo->currentText();
    loadCategories(currentType);

    auto bills = databaseManager::instance().getBills(m_userId);
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

        if (b.type == "收入")
            totalIncome += b.amount;
        else
            totalExpense += b.amount;
    }

    m_incomeLabel->setText(QStringLiteral("收入: ¥%1").arg(totalIncome, 0, 'f', 2));
    m_expenseLabel->setText(QStringLiteral("支出: ¥%1").arg(totalExpense, 0, 'f', 2));
    m_balanceLabel->setText(QStringLiteral("结余: ¥%1").arg(totalIncome - totalExpense, 0, 'f', 2));
}

void BillWidget::onAddBill()
{
    QString type = m_typeCombo->currentText();
    int categoryId = m_categoryCombo->currentData().toInt();
    int accountId = m_accountCombo->currentData().toInt();
    QString date = m_dateEdit->date().toString("yyyy-MM-dd");
    double amount = m_amountSpin->value();
    QString remark = m_remarkEdit->text().trimmed();

    if (accountId <= 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先创建账户！"));
        return;
    }
    if (categoryId <= 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先添加分类！"));
        return;
    }
    if (amount <= 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入有效金额"));
        return;
    }

    if (databaseManager::instance().addBill(m_userId, accountId, categoryId, date, type, amount, remark)) {
        refresh();
        emit billsChanged();
    } else {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("添加账单失败"));
    }
}

void BillWidget::onEditBill()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int billId = m_table->item(row, 0)->text().toInt();
    QString oldType   = m_table->item(row, 2)->text();
    QString oldCat    = m_table->item(row, 3)->text();
    QString oldAcct   = m_table->item(row, 4)->text();
    QString oldDate   = m_table->item(row, 1)->text();
    double oldAmount  = m_table->item(row, 5)->text().toDouble();
    QString oldRemark = m_table->item(row, 6)->text();

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("修改账单"));
    dlg.setFixedSize(400, 350);
    auto *dlgLayout = new QVBoxLayout(&dlg);

    auto *form = new QFormLayout;
    form->setSpacing(10);

    auto *typeCombo = new QComboBox;
    typeCombo->addItems({QStringLiteral("支出"), QStringLiteral("收入")});
    typeCombo->setCurrentText(oldType);
    form->addRow(QStringLiteral("类型:"), typeCombo);

    auto *dateEdit = new QDateEdit(QDate::fromString(oldDate, "yyyy-MM-dd"));
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    form->addRow(QStringLiteral("日期:"), dateEdit);

    auto *acctCombo = new QComboBox;
    auto accounts = databaseManager::instance().getAccounts(m_userId);
    for (const auto &a : accounts) {
        acctCombo->addItem(a.name, a.id);
        if (a.name == oldAcct) acctCombo->setCurrentIndex(acctCombo->count() - 1);
    }
    form->addRow(QStringLiteral("账户:"), acctCombo);

    auto *catCombo = new QComboBox;
    catCombo->setMinimumWidth(150);
    auto loadCats = [&](const QString &t) {
        catCombo->clear();
        auto cats = databaseManager::instance().getCategoriesByType(m_userId, t);
        for (const auto &c : cats) {
            QString text = c.icon.isEmpty() ? c.name : (c.icon + " " + c.name);
            catCombo->addItem(text, c.id);
            if (c.name == oldCat) catCombo->setCurrentIndex(catCombo->count() - 1);
        }
    };
    loadCats(oldType);
    QObject::connect(typeCombo, &QComboBox::currentTextChanged, &dlg, loadCats);
    form->addRow(QStringLiteral("分类:"), catCombo);

    auto *amountSpin = new QDoubleSpinBox;
    amountSpin->setRange(0.00, 99999999.99);
    amountSpin->setDecimals(2);
    amountSpin->setPrefix("¥");
    amountSpin->setValue(oldAmount);
    form->addRow(QStringLiteral("金额:"), amountSpin);

    auto *remarkEdit = new QLineEdit;
    remarkEdit->setText(oldRemark);
    remarkEdit->setPlaceholderText(QStringLiteral("备注（可选）"));
    form->addRow(QStringLiteral("备注:"), remarkEdit);

    dlgLayout->addLayout(form);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btnBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    btnBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlgLayout->addWidget(btnBox);

    if (dlg.exec() != QDialog::Accepted)
        return;

    if (amountSpin->value() <= 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入有效金额"));
        return;
    }

    if (databaseManager::instance().updateBill(billId,
            acctCombo->currentData().toInt(),
            catCombo->currentData().toInt(),
            dateEdit->date().toString("yyyy-MM-dd"),
            typeCombo->currentText(),
            amountSpin->value(),
            remarkEdit->text().trimmed())) {
        refresh();
        emit billsChanged();
    } else {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("修改账单失败"));
    }
}

void BillWidget::onDeleteBill()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int billId = m_table->item(row, 0)->text().toInt();
    auto reply = QMessageBox::question(this, QStringLiteral("确认删除"),
                                        QStringLiteral("确定要删除这条账单记录吗？"),
                                        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        if (databaseManager::instance().deleteBill(billId)) {
            refresh();
            emit billsChanged();
        }
    }
}

void BillWidget::onBillSelected()
{
    int row = m_table->currentRow();
    bool has = (row >= 0);
    m_editBtn->setEnabled(has);
    m_deleteBtn->setEnabled(has);
}

void BillWidget::onExportCSV()
{
    QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("导出CSV"),
                                                     QStringLiteral("账单_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
                                                     QStringLiteral("CSV文件 (*.csv)"));
    if (fileName.isEmpty()) return;

    if (databaseManager::instance().exportToCSV(m_userId, fileName))
        QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("账单已导出到:\n%1").arg(fileName));
    else
        QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("无法写入文件"));
}
