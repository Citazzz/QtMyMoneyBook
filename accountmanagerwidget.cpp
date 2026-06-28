#include "accountmanagerwidget.h"
#include "databaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>

AccountManagerWidget::AccountManagerWidget(int userId, QWidget *parent)
    : QWidget(parent), m_userId(userId)
{
    setupUI();
    refresh();
}

void AccountManagerWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    // 输入区
    auto *inputGroup = new QGroupBox(QStringLiteral("账户信息"));
    auto *inputLayout = new QHBoxLayout(inputGroup);

    inputLayout->addWidget(new QLabel(QStringLiteral("名称:")));
    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText(QStringLiteral("如: 工商银行卡"));
    inputLayout->addWidget(m_nameEdit);

    inputLayout->addWidget(new QLabel(QStringLiteral("类型:")));
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({QStringLiteral("现金"), QStringLiteral("支付宝"), QStringLiteral("银行卡"), QStringLiteral("微信"), QStringLiteral("其他")});
    inputLayout->addWidget(m_typeCombo);

    inputLayout->addWidget(new QLabel(QStringLiteral("余额:")));
    m_balanceSpin = new QDoubleSpinBox;
    m_balanceSpin->setRange(0, 99999999.99);
    m_balanceSpin->setDecimals(2);
    m_balanceSpin->setPrefix("¥");
    inputLayout->addWidget(m_balanceSpin);

    m_addBtn = new QPushButton(QStringLiteral("添加"));
    m_editBtn = new QPushButton(QStringLiteral("修改"));
    m_deleteBtn = new QPushButton(QStringLiteral("删除"));
    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    inputLayout->addWidget(m_addBtn);
    inputLayout->addWidget(m_editBtn);
    inputLayout->addWidget(m_deleteBtn);

    mainLayout->addWidget(inputGroup);

    // 列表
    m_table = new QTableWidget;
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({QStringLiteral("ID"), QStringLiteral("名称"), QStringLiteral("类型"), QStringLiteral("余额")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setColumnHidden(0, true);
    mainLayout->addWidget(m_table);

    connect(m_addBtn, &QPushButton::clicked, this, &AccountManagerWidget::onAdd);
    connect(m_editBtn, &QPushButton::clicked, this, &AccountManagerWidget::onEdit);
    connect(m_deleteBtn, &QPushButton::clicked, this, &AccountManagerWidget::onDelete);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &AccountManagerWidget::onSelectionChanged);
}

void AccountManagerWidget::refresh()
{
    auto accounts = databaseManager::instance().getAccounts(m_userId);
    m_table->setRowCount(0);
    for (const auto &a : accounts) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(a.id)));
        m_table->setItem(row, 1, new QTableWidgetItem(a.name));
        m_table->setItem(row, 2, new QTableWidgetItem(a.type));
        auto *amtItem = new QTableWidgetItem(QString::number(a.balance, 'f', 2));
        amtItem->setForeground(a.balance >= 0 ? QColor("#00C897") : QColor("#FF6B6B"));
        m_table->setItem(row, 3, amtItem);
    }
}

void AccountManagerWidget::onAdd()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入账户名称"));
        return;
    }
    databaseManager::instance().addAccount(m_userId, name, m_typeCombo->currentText(), m_balanceSpin->value());
    refresh();
    emit accountsChanged();
}

void AccountManagerWidget::onEdit()
{
    int row = m_table->currentRow();
    if (row < 0) return;
    int id = m_table->item(row, 0)->text().toInt();
    databaseManager::instance().updateAccount(id, m_nameEdit->text().trimmed(),
                                               m_typeCombo->currentText(), m_balanceSpin->value());
    refresh();
    emit accountsChanged();
}

void AccountManagerWidget::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) return;
    int id = m_table->item(row, 0)->text().toInt();
    auto reply = QMessageBox::question(this, QStringLiteral("确认删除"),
                                        QStringLiteral("删除账户不会删除关联账单，确定继续吗？"),
                                        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        databaseManager::instance().deleteAccount(id);
        refresh();
        emit accountsChanged();
    }
}

void AccountManagerWidget::onSelectionChanged()
{
    int row = m_table->currentRow();
    bool has = (row >= 0);
    m_editBtn->setEnabled(has);
    m_deleteBtn->setEnabled(has);
    if (has) {
        m_nameEdit->setText(m_table->item(row, 1)->text());
        m_typeCombo->setCurrentText(m_table->item(row, 2)->text());
        m_balanceSpin->setValue(m_table->item(row, 3)->text().toDouble());
    }
}