#include "categorymanagerwidget.h"
#include "databaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QGridLayout>

static const char* EMOJI_SET[] = {
    "💰", "💵", "💴", "💶", "💷", "💸", "🪙", "💎", "🏦", "💳",
    "🍽️", "🍔", "🍕", "🍜", "☕", "🍺", "🍰", "🥗", "🍱", "🧋",
    "🚗", "🚌", "🚇", "✈️", "🚲", "🚄", "🚕", "⛽", "🅿️", "🚢",
    "🛒", "👗", "👟", "💄", "🎒", "👕", "👜", "⌚", "💍", "👓",
    "🏠", "🔑", "💡", "🪑", "🛏️", "🛁", "🏡", "🪴", "🖼️", "🕰️",
    "🎮", "🎬", "🎵", "📺", "🎪", "🎤", "🎧", "🎯", "🎨", "🎭",
    "🏥", "💊", "🩺", "🩹", "🏃", "🧘", "💪", "🦷", "👁️", "🧠",
    "📚", "📝", "🎓", "✏️", "📐", "💻", "🖨️", "📱", "📞", "⌨️",
    "🧴", "🧹", "🧺", "🪥", "🧻", "🪒", "🧼", "🪣", "🧽", "🫧",
    "🎁", "🎉", "🎊", "💼", "📊", "📋", "📈", "📌", "📦", "❤️"
};

QString CategoryManagerWidget::pickIcon()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("选择图标"));
    dlg.setFixedSize(520, 380);

    auto *layout = new QVBoxLayout(&dlg);
    auto *grid = new QGridLayout;
    grid->setSpacing(6);

    QString selected = m_currentIcon;
    bool picked = false;

    for (int i = 0; i < 100; ++i) {
        auto *btn = new QPushButton(QString::fromUtf8(EMOJI_SET[i]));
        btn->setFixedSize(44, 44);
        btn->setStyleSheet("QPushButton{font-size:22px; border:2px solid transparent; border-radius:6px; background:#F8F9FC;}"
                           "QPushButton:hover{border-color:#5B7FFF; background:#EEF1FF;}");
        int row = i / 10, col = i % 10;
        grid->addWidget(btn, row, col);

        QString icon = QString::fromUtf8(EMOJI_SET[i]);
        connect(btn, &QPushButton::clicked, [&dlg, &selected, &picked, icon]() {
            selected = icon;
            picked = true;
            dlg.accept();
        });
    }

    layout->addLayout(grid);

    auto *clearBtn = new QPushButton(QStringLiteral("清除图标"));
    clearBtn->setFlat(true);
    connect(clearBtn, &QPushButton::clicked, [&dlg, &selected, &picked]() {
        selected.clear();
        picked = true;
        dlg.accept();
    });
    layout->addWidget(clearBtn);

    layout->addStretch();

    dlg.exec();
    if (picked)
        m_currentIcon = selected;

    return m_currentIcon;
}

CategoryManagerWidget::CategoryManagerWidget(int userId, QWidget *parent)
    : QWidget(parent), m_userId(userId)
{
    setupUI();
    refresh();
}

void CategoryManagerWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *inputGroup = new QGroupBox(QStringLiteral("分类信息"));
    auto *inputLayout = new QHBoxLayout(inputGroup);

    inputLayout->addWidget(new QLabel(QStringLiteral("名称:")));
    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText(QStringLiteral("如: 购物"));
    inputLayout->addWidget(m_nameEdit);

    inputLayout->addWidget(new QLabel(QStringLiteral("类型:")));
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({QStringLiteral("支出"), QStringLiteral("收入")});
    inputLayout->addWidget(m_typeCombo);

    inputLayout->addWidget(new QLabel(QStringLiteral("图标:")));
    m_iconBtn = new QPushButton;
    m_iconBtn->setFixedSize(40, 40);
    m_iconBtn->setStyleSheet("QPushButton{font-size:20px; border:2px dashed #D1D5DB; border-radius:6px; background:#F8F9FC;}"
                             "QPushButton:hover{border-color:#5B7FFF;}");
    inputLayout->addWidget(m_iconBtn);

    m_addBtn = new QPushButton(QStringLiteral("添加"));
    m_editBtn = new QPushButton(QStringLiteral("修改"));
    m_deleteBtn = new QPushButton(QStringLiteral("删除"));
    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    inputLayout->addWidget(m_addBtn);
    inputLayout->addWidget(m_editBtn);
    inputLayout->addWidget(m_deleteBtn);

    mainLayout->addWidget(inputGroup);

    m_table = new QTableWidget;
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({QStringLiteral("ID"), QStringLiteral("图标"), QStringLiteral("名称"), QStringLiteral("类型")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setColumnHidden(0, true);
    m_table->setColumnWidth(1, 60);
    mainLayout->addWidget(m_table);

    connect(m_iconBtn, &QPushButton::clicked, this, &CategoryManagerWidget::onPickIcon);
    connect(m_addBtn, &QPushButton::clicked, this, &CategoryManagerWidget::onAdd);
    connect(m_editBtn, &QPushButton::clicked, this, &CategoryManagerWidget::onEdit);
    connect(m_deleteBtn, &QPushButton::clicked, this, &CategoryManagerWidget::onDelete);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &CategoryManagerWidget::onSelectionChanged);
}

void CategoryManagerWidget::refresh()
{
    auto categories = databaseManager::instance().getCategories(m_userId);
    m_table->setRowCount(0);
    for (const auto &c : categories) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(c.id)));
        auto *iconItem = new QTableWidgetItem(c.icon);
        iconItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 1, iconItem);
        m_table->setItem(row, 2, new QTableWidgetItem(c.name));
        auto *typeItem = new QTableWidgetItem(c.type);
        typeItem->setForeground(c.type == "收入" ? QColor("#00C897") : QColor("#FF6B6B"));
        m_table->setItem(row, 3, typeItem);
    }
}

void CategoryManagerWidget::onPickIcon()
{
    m_currentIcon = pickIcon();
    m_iconBtn->setText(m_currentIcon);
}

void CategoryManagerWidget::onAdd()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入分类名称"));
        return;
    }
    databaseManager::instance().addCategory(m_userId, name, m_typeCombo->currentText(), m_currentIcon);
    m_nameEdit->clear();
    m_currentIcon.clear();
    m_iconBtn->setText(QString());
    refresh();
}

void CategoryManagerWidget::onEdit()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int id = m_table->item(row, 0)->text().toInt();
    QString oldName = m_table->item(row, 2)->text();
    QString oldType = m_table->item(row, 3)->text();
    QString oldIcon = m_table->item(row, 1)->text();

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("修改分类"));
    dlg.setFixedSize(380, 200);
    auto *dlgLayout = new QVBoxLayout(&dlg);

    auto *form = new QFormLayout;
    form->setSpacing(10);

    auto *nameEdit = new QLineEdit(oldName);
    nameEdit->setMinimumHeight(32);
    form->addRow(QStringLiteral("名称:"), nameEdit);

    auto *typeCombo = new QComboBox;
    typeCombo->addItems({QStringLiteral("支出"), QStringLiteral("收入")});
    typeCombo->setCurrentText(oldType);
    form->addRow(QStringLiteral("类型:"), typeCombo);

    // Icon row with pick button
    auto *iconRow = new QHBoxLayout;
    auto *iconBtn = new QPushButton(oldIcon.isEmpty() ? QStringLiteral("选择") : oldIcon);
    iconBtn->setFixedSize(44, 44);
    iconBtn->setStyleSheet("QPushButton{font-size:20px; border:2px dashed #D1D5DB; border-radius:6px; background:#F8F9FC;}"
                           "QPushButton:hover{border-color:#5B7FFF;}");
    QString pickedIcon = oldIcon;
    connect(iconBtn, &QPushButton::clicked, [&dlg, iconBtn, &pickedIcon]() {
        QDialog pickDlg(&dlg);
        pickDlg.setWindowTitle(QStringLiteral("选择图标"));
        pickDlg.setFixedSize(520, 380);
        auto *pLayout = new QVBoxLayout(&pickDlg);
        auto *grid = new QGridLayout;
        grid->setSpacing(6);
        for (int i = 0; i < 100; ++i) {
            auto *ebtn = new QPushButton(QString::fromUtf8(EMOJI_SET[i]));
            ebtn->setFixedSize(44, 44);
            ebtn->setStyleSheet("QPushButton{font-size:22px; border:2px solid transparent; border-radius:6px; background:#F8F9FC;}"
                                "QPushButton:hover{border-color:#5B7FFF; background:#EEF1FF;}");
            int r = i / 10, c = i % 10;
            grid->addWidget(ebtn, r, c);
            QString emoji = QString::fromUtf8(EMOJI_SET[i]);
            connect(ebtn, &QPushButton::clicked, [&pickDlg, iconBtn, &pickedIcon, emoji]() {
                pickedIcon = emoji;
                iconBtn->setText(emoji);
                pickDlg.accept();
            });
        }
        pLayout->addLayout(grid);
        auto *clearB = new QPushButton(QStringLiteral("清除图标"));
        clearB->setFlat(true);
        connect(clearB, &QPushButton::clicked, [&pickDlg, iconBtn, &pickedIcon]() {
            pickedIcon.clear();
            iconBtn->setText(QStringLiteral("选择"));
            pickDlg.accept();
        });
        pLayout->addWidget(clearB);
        pickDlg.exec();
    });
    iconRow->addWidget(iconBtn);
    iconRow->addStretch();
    form->addRow(QStringLiteral("图标:"), iconRow);

    dlgLayout->addLayout(form);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btnBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    btnBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlgLayout->addWidget(btnBox);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString newName = nameEdit->text().trimmed();
    if (newName.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("分类名称不能为空"));
        return;
    }

    databaseManager::instance().updateCategory(id, newName, typeCombo->currentText(), pickedIcon);
    refresh();
}

void CategoryManagerWidget::onDelete()
{
    int row = m_table->currentRow();
    if (row < 0) return;
    int id = m_table->item(row, 0)->text().toInt();
    auto reply = QMessageBox::question(this, QStringLiteral("确认删除"),
                                        QStringLiteral("确定要删除此分类吗？"),
                                        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        databaseManager::instance().deleteCategory(id);
        refresh();
    }
}

void CategoryManagerWidget::onSelectionChanged()
{
    int row = m_table->currentRow();
    bool has = (row >= 0);
    m_editBtn->setEnabled(has);
    m_deleteBtn->setEnabled(has);
}
