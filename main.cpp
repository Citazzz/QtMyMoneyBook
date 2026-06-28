
#include "mainwindow.h"
#include "LoginDialog.h"
#include "databaseManager.h"

#include <QApplication>

static const char *APP_THEME = R"(
/* ===== 全局 ===== */
QWidget {
    background-color: #F0F2F5;
    color: #1A1D26;
    font-family: "Microsoft YaHei", "Segoe UI", "Helvetica Neue", sans-serif;
    font-size: 13px;
}

QMainWindow {
    background-color: #F0F2F5;
}

/* ===== QTabWidget ===== */
QTabWidget::pane {
    border: none;
    background-color: #FFFFFF;
    border-radius: 8px;
}

QTabBar::tab {
    background: #E8EAF0;
    color: #6B7280;
    border: none;
    padding: 10px 24px;
    margin-right: 4px;
    border-radius: 8px 8px 0 0;
    min-width: 90px;
    font-weight: 500;
}

QTabBar::tab:selected {
    background: #FFFFFF;
    color: #5B7FFF;
    font-weight: 600;
}

QTabBar::tab:hover:!selected {
    background: #EDEFF5;
    color: #5B7FFF;
}

/* ===== QGroupBox ===== */
QGroupBox {
    font-weight: 600;
    color: #1A1D26;
    border: 1px solid #E4E7EC;
    border-radius: 10px;
    margin-top: 14px;
    padding: 18px 14px 14px 14px;
    background-color: #FFFFFF;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 16px;
    padding: 0 10px;
    color: #5B7FFF;
    font-size: 13px;
}

/* ===== QPushButton ===== */
QPushButton {
    background-color: #5B7FFF;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    padding: 8px 18px;
    font-weight: 600;
    min-width: 64px;
}

QPushButton:hover {
    background-color: #4A6AE8;
}

QPushButton:pressed {
    background-color: #3D56D4;
}

QPushButton:disabled {
    background-color: #D1D5DB;
    color: #9CA3AF;
}

QPushButton[flat="true"] {
    background-color: transparent;
    color: #5B7FFF;
    border: none;
    font-weight: 500;
}

QPushButton[flat="true"]:hover {
    color: #4A6AE8;
}

/* ===== QLineEdit / QDateEdit / QDoubleSpinBox / QSpinBox / QComboBox ===== */
QLineEdit, QDateEdit, QDoubleSpinBox, QSpinBox, QComboBox {
    border: 1px solid #E4E7EC;
    border-radius: 6px;
    padding: 7px 10px;
    background-color: #FFFFFF;
    color: #1A1D26;
    selection-background-color: #5B7FFF;
    selection-color: #FFFFFF;
}

QLineEdit:focus, QDateEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus, QComboBox:focus {
    border-color: #5B7FFF;
    outline: none;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 26px;
    border-left: 1px solid #E4E7EC;
    border-top-right-radius: 6px;
    border-bottom-right-radius: 6px;
    background-color: #F8F9FC;
}

QComboBox QAbstractItemView {
    border: 1px solid #E4E7EC;
    border-radius: 4px;
    background-color: #FFFFFF;
    selection-background-color: #EEF1FF;
    selection-color: #1A1D26;
    padding: 4px;
}

/* ===== QTableWidget ===== */
QTableWidget {
    background-color: #FFFFFF;
    border: 1px solid #E4E7EC;
    border-radius: 8px;
    gridline-color: #F0F2F5;
    selection-background-color: #EEF1FF;
    selection-color: #1A1D26;
}

QHeaderView::section {
    background-color: #F8F9FC;
    color: #374151;
    padding: 10px 8px;
    border: none;
    border-bottom: 2px solid #E4E7EC;
    font-weight: 600;
    font-size: 12px;
}

/* ===== QProgressBar ===== */
QProgressBar {
    border: none;
    border-radius: 8px;
    background-color: #E8EAF0;
    text-align: center;
    color: #1A1D26;
    font-weight: 600;
    height: 24px;
    overflow: hidden;
}

QProgressBar::chunk {
    border-radius: 8px;
}

/* ===== QMenuBar ===== */
QMenuBar {
    background-color: #FFFFFF;
    border-bottom: 1px solid #E4E7EC;
    padding: 4px;
}

QMenuBar::item:selected {
    background-color: #EEF1FF;
    border-radius: 4px;
    color: #5B7FFF;
}

/* ===== QStatusBar ===== */
QStatusBar {
    background-color: #FFFFFF;
    color: #8E93A0;
    border-top: 1px solid #E4E7EC;
    font-size: 12px;
}

/* ===== QScrollBar ===== */
QScrollBar:vertical {
    border: none;
    background: transparent;
    width: 8px;
    border-radius: 4px;
    margin: 2px;
}

QScrollBar::handle:vertical {
    background: #C4C9D4;
    border-radius: 4px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background: #9CA3AF;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    border: none;
    background: transparent;
    height: 8px;
    border-radius: 4px;
    margin: 2px;
}

QScrollBar::handle:horizontal {
    background: #C4C9D4;
    border-radius: 4px;
    min-width: 30px;
}

QScrollBar::handle:horizontal:hover {
    background: #9CA3AF;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

/* ===== QDialog ===== */
QDialog {
    background-color: #FFFFFF;
}

/* ===== QToolTip ===== */
QToolTip {
    background-color: #1A1D26;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    padding: 6px 10px;
    font-size: 12px;
}
)";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet(QString::fromUtf8(APP_THEME));

    if (!databaseManager::instance().init()) {
        return -1;
    }

    int result = 0;
    while (true) {
        LoginDialog loginDialog;
        if (loginDialog.exec() != QDialog::Accepted) {
            break;
        }

        MainWindow w(loginDialog.userId());
        w.show();

        result = a.exec();
        if (result != 100) break;  // 100 = 注销账户，重启登录
    }

    databaseManager::instance().close();
    return result;
}
