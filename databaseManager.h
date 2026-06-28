#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QCryptographicHash>

struct AccountInfo {
    int id = -1;
    QString name;
    QString type;   // 现金/支付宝/银行卡/其他
    double balance = 0.0;
};

struct CategoryInfo {
    int id = -1;
    QString name;
    QString type;   // 收入/支出
    QString icon;
};

struct BillInfo {
    int id = -1;
    int accountId = -1;
    int categoryId = -1;
    QString date;
    QString type;       // 收入/支出
    double amount = 0.0;
    QString remark;
    QString accountName;
    QString categoryName;
};

struct BudgetInfo {
    int id = -1;
    QString month;      // YYYY-MM
    double amount = 0.0;
};

class databaseManager
{
public:
    static databaseManager& instance();

    bool init();
    void close();

    // 用户管理
    int registerUser(const QString &user, const QString &password);
    int loginUser(const QString &user, const QString &password);
    bool deleteUser(int userId);
    int currentUserId() const { return m_currentUserId; }
    QString currentUsername() const;

    // 账户管理
    int getOrCreateDefaultAccount(int userId);
    QVector<AccountInfo> getAccounts(int userId);
    bool addAccount(int userId, const QString &name, const QString &type, double balance);
    bool updateAccount(int id, const QString &name, const QString &type, double balance);
    bool deleteAccount(int id);

    // 分类管理
    bool addCategory(int userId, const QString &name, const QString &type, const QString &icon = "");
    bool updateCategory(int id, const QString &name, const QString &type, const QString &icon = "");
    bool deleteCategory(int id);
    QVector<CategoryInfo> getCategories(int userId);
    QVector<CategoryInfo> getCategoriesByType(int userId, const QString &type);

    // 账单管理
    bool addBill(int userId, int accountId, int categoryId,
                 const QString &date, const QString &type,
                 double amount, const QString &remark);
    bool updateBill(int id, int accountId, int categoryId,
                    const QString &date, const QString &type,
                    double amount, const QString &remark);
    bool deleteBill(int id);
    QVector<BillInfo> getBills(int userId);
    QVector<BillInfo> getBillsByDateRange(int userId, const QString &startDate, const QString &endDate);
    QVector<BillInfo> getBillsByType(int userId, const QString &type);
    QVector<BillInfo> searchBills(int userId, const QString &keyword,
                                  const QString &startDate, const QString &endDate,
                                  const QString &type, int categoryId);

    // 预算管理
    bool setBudget(int userId, const QString &month, double amount);
    BudgetInfo getBudget(int userId, const QString &month);
    double getMonthExpense(int userId, const QString &month);
    double getMonthIncome(int userId, const QString &month);

    // 统计
    QVector<QPair<QString, double>> getCategoryStats(int userId, const QString &type,
                                                      const QString &startDate, const QString &endDate);
    QVector<QPair<QString, double>> getDailyStats(int userId, const QString &type,
                                                   const QString &startDate, const QString &endDate);

    // 导出CSV
    bool exportToCSV(int userId, const QString &filePath);

private:
    databaseManager() = default;
    QSqlDatabase m_db;
    int m_currentUserId = -1;

    QString hashPassword(const QString &password);
    void ensureDefaultCategories(int userId);
};

#endif // DATABASEMANAGER_H
