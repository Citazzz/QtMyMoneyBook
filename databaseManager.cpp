#include "databaseManager.h"
#include <QFile>
#include <QTextStream>
#include <QDir>

databaseManager& databaseManager::instance()
{
    static databaseManager inst;
    return inst;
}

QString databaseManager::hashPassword(const QString &password)
{
    QByteArray salted = ("MyMoneyBook_Salt_" + password).toUtf8();
    return QString(QCryptographicHash::hash(salted, QCryptographicHash::Sha256).toHex());
}

bool databaseManager::init()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("mymoney_data.db");

    if (!m_db.open()) {
        qDebug() << "Error: connection with database failed" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;

    query.exec("PRAGMA foreign_keys = ON");

    query.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE NOT NULL, "
        "password_hash TEXT NOT NULL)"
    );

    query.exec(
        "CREATE TABLE IF NOT EXISTS accounts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "name TEXT NOT NULL, "
        "type TEXT NOT NULL, "
        "balance REAL DEFAULT 0.0, "
        "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE)"
    );

    query.exec(
        "CREATE TABLE IF NOT EXISTS categories ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "name TEXT NOT NULL, "
        "type TEXT NOT NULL, "
        "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE)"
    );

    query.exec(
        "CREATE TABLE IF NOT EXISTS bills ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "account_id INTEGER, "
        "category_id INTEGER, "
        "date TEXT NOT NULL, "
        "type TEXT NOT NULL, "
        "amount REAL NOT NULL, "
        "remark TEXT, "
        "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE, "
        "FOREIGN KEY(account_id) REFERENCES accounts(id) ON DELETE SET NULL, "
        "FOREIGN KEY(category_id) REFERENCES categories(id) ON DELETE SET NULL)"
    );

    query.exec(
        "CREATE TABLE IF NOT EXISTS budgets ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "month TEXT NOT NULL, "
        "amount REAL NOT NULL, "
        "UNIQUE(user_id, month), "
        "FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE)"
    );

    // 迁移: categories 表增加 icon 字段
    query.exec("ALTER TABLE categories ADD COLUMN icon TEXT DEFAULT ''");

    // 为已存在的空图标分类设置默认图标
    struct { const char *name; const char *icon; } iconDefaults[] = {
        {"工资", "💰"}, {"奖金", "🎁"}, {"兼职", "💼"}, {"投资", "📈"}, {"其他收入", "💵"},
        {"餐饮", "🍽️"}, {"交通", "🚗"}, {"购物", "🛒"}, {"住房", "🏠"}, {"娱乐", "🎮"},
        {"医疗", "🏥"}, {"教育", "📚"}, {"通讯", "📱"}, {"日用", "🧴"}, {"其他支出", "📦"}
    };
    for (const auto &d : iconDefaults) {
        QSqlQuery uq;
        uq.prepare("UPDATE categories SET icon = ? WHERE name = ? AND (icon IS NULL OR icon = '')");
        uq.addBindValue(QString::fromUtf8(d.icon));
        uq.addBindValue(QString::fromUtf8(d.name));
        uq.exec();
    }

    return true;
}

void databaseManager::close()
{
    m_db.close();
    m_currentUserId = -1;
}

// ==================== 用户管理 ====================

int databaseManager::registerUser(const QString &user, const QString &password)
{
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password_hash) VALUES (?, ?)");
    query.addBindValue(user);
    query.addBindValue(hashPassword(password));

    if (!query.exec()) {
        qDebug() << "Register failed:" << query.lastError().text();
        return -1;
    }

    int userId = query.lastInsertId().toInt();
    m_currentUserId = userId;
    ensureDefaultCategories(userId);
    getOrCreateDefaultAccount(userId);
    return userId;
}

int databaseManager::loginUser(const QString &user, const QString &password)
{
    QSqlQuery query;
    query.prepare("SELECT id FROM users WHERE username = ? AND password_hash = ?");
    query.addBindValue(user);
    query.addBindValue(hashPassword(password));

    if (query.exec() && query.next()) {
        m_currentUserId = query.value(0).toInt();
        return m_currentUserId;
    }
    return -1;
}

bool databaseManager::deleteUser(int userId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM users WHERE id = ?");
    query.addBindValue(userId);
    if (!query.exec()) {
        qDebug() << "deleteUser failed:" << query.lastError().text();
        return false;
    }
    m_currentUserId = -1;
    return true;
}

QString databaseManager::currentUsername() const
{
    QSqlQuery query;
    query.prepare("SELECT username FROM users WHERE id = ?");
    query.addBindValue(m_currentUserId);
    if (query.exec() && query.next())
        return query.value(0).toString();
    return {};
}

void databaseManager::ensureDefaultCategories(int userId)
{
    struct DefaultCat { QString name; QString type; QString icon; };
    QVector<DefaultCat> defaults = {
        {"工资",   "收入", "💰"}, {"奖金",   "收入", "🎁"}, {"兼职",   "收入", "💼"}, {"投资", "收入", "📈"}, {"其他收入", "收入", "💵"},
        {"餐饮",   "支出", "🍽️"}, {"交通",   "支出", "🚗"}, {"购物",   "支出", "🛒"}, {"住房", "支出", "🏠"}, {"娱乐",     "支出", "🎮"},
        {"医疗",   "支出", "🏥"}, {"教育",   "支出", "📚"}, {"通讯",   "支出", "📱"}, {"日用", "支出", "🧴"}, {"其他支出", "支出", "📦"}
    };

    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO categories (user_id, name, type, icon) VALUES (?, ?, ?, ?)");
    for (const auto &d : defaults) {
        query.addBindValue(userId);
        query.addBindValue(d.name);
        query.addBindValue(d.type);
        query.addBindValue(d.icon);
        query.exec();
    }

    // 为已存在的旧分类补充默认图标
    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE categories SET icon = ? WHERE user_id = ? AND name = ? AND (icon IS NULL OR icon = '')");
    for (const auto &d : defaults) {
        updateQuery.addBindValue(d.icon);
        updateQuery.addBindValue(userId);
        updateQuery.addBindValue(d.name);
        updateQuery.exec();
    }
}

// ==================== 账户管理 ====================

int databaseManager::getOrCreateDefaultAccount(int userId)
{
    QSqlQuery query;
    query.prepare("SELECT id FROM accounts WHERE user_id = ? LIMIT 1");
    query.addBindValue(userId);
    if (query.exec() && query.next())
        return query.value(0).toInt();

    // 不存在则自动创建
    QSqlQuery insert;
    insert.prepare("INSERT INTO accounts (user_id, name, type, balance) VALUES (?, ?, ?, ?)");
    insert.addBindValue(userId);
    insert.addBindValue(QStringLiteral("默认账户"));
    insert.addBindValue(QStringLiteral("通用"));
    insert.addBindValue(0.0);
    if (insert.exec())
        return insert.lastInsertId().toInt();

    qDebug() << "getOrCreateDefaultAccount failed:" << insert.lastError().text();
    return -1;
}

QVector<AccountInfo> databaseManager::getAccounts(int userId)
{
    QVector<AccountInfo> result;
    QSqlQuery query;
    query.prepare("SELECT id, name, type, balance FROM accounts WHERE user_id = ? ORDER BY id");
    query.addBindValue(userId);
    if (query.exec()) {
        while (query.next()) {
            AccountInfo a;
            a.id = query.value(0).toInt();
            a.name = query.value(1).toString();
            a.type = query.value(2).toString();
            a.balance = query.value(3).toDouble();
            result.append(a);
        }
    }
    return result;
}

bool databaseManager::addAccount(int userId, const QString &name, const QString &type, double balance)
{
    QSqlQuery query;
    query.prepare("INSERT INTO accounts (user_id, name, type, balance) VALUES (?, ?, ?, ?)");
    query.addBindValue(userId);
    query.addBindValue(name);
    query.addBindValue(type);
    query.addBindValue(balance);
    if (!query.exec()) {
        qDebug() << "addAccount failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool databaseManager::updateAccount(int id, const QString &name, const QString &type, double balance)
{
    QSqlQuery query;
    query.prepare("UPDATE accounts SET name = ?, type = ?, balance = ? WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(type);
    query.addBindValue(balance);
    query.addBindValue(id);
    if (!query.exec()) {
        qDebug() << "updateAccount failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool databaseManager::deleteAccount(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM accounts WHERE id = ?");
    query.addBindValue(id);
    if (!query.exec()) {
        qDebug() << "deleteAccount failed:" << query.lastError().text();
        return false;
    }
    return true;
}

// ==================== 分类管理 ====================

bool databaseManager::addCategory(int userId, const QString &name, const QString &type, const QString &icon)
{
    QSqlQuery query;
    query.prepare("INSERT INTO categories (user_id, name, type, icon) VALUES (?, ?, ?, ?)");
    query.addBindValue(userId);
    query.addBindValue(name);
    query.addBindValue(type);
    query.addBindValue(icon);
    return query.exec();
}

bool databaseManager::updateCategory(int id, const QString &name, const QString &type, const QString &icon)
{
    QSqlQuery query;
    query.prepare("UPDATE categories SET name = ?, type = ?, icon = ? WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(type);
    query.addBindValue(icon);
    query.addBindValue(id);
    return query.exec();
}

bool databaseManager::deleteCategory(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM categories WHERE id = ?");
    query.addBindValue(id);
    return query.exec();
}

QVector<CategoryInfo> databaseManager::getCategories(int userId)
{
    QVector<CategoryInfo> result;
    QSqlQuery query;
    query.prepare("SELECT id, name, type, COALESCE(icon, '') FROM categories WHERE user_id = ? ORDER BY type, id");
    query.addBindValue(userId);
    if (query.exec()) {
        while (query.next()) {
            CategoryInfo c;
            c.id = query.value(0).toInt();
            c.name = query.value(1).toString();
            c.type = query.value(2).toString();
            c.icon = query.value(3).toString();
            result.append(c);
        }
    }
    return result;
}

QVector<CategoryInfo> databaseManager::getCategoriesByType(int userId, const QString &type)
{
    QVector<CategoryInfo> result;
    QSqlQuery query;
    query.prepare("SELECT id, name, type, COALESCE(icon, '') FROM categories WHERE user_id = ? AND type = ? ORDER BY id");
    query.addBindValue(userId);
    query.addBindValue(type);
    if (query.exec()) {
        while (query.next()) {
            CategoryInfo c;
            c.id = query.value(0).toInt();
            c.name = query.value(1).toString();
            c.type = query.value(2).toString();
            c.icon = query.value(3).toString();
            result.append(c);
        }
    }
    return result;
}

// ==================== 账单管理 ====================

bool databaseManager::addBill(int userId, int accountId, int categoryId,
                               const QString &date, const QString &type,
                               double amount, const QString &remark)
{
    QSqlQuery query;
    query.prepare("INSERT INTO bills (user_id, account_id, category_id, date, type, amount, remark) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(userId);
    query.addBindValue(accountId);
    query.addBindValue(categoryId);
    query.addBindValue(date);
    query.addBindValue(type);
    query.addBindValue(amount);
    query.addBindValue(remark);

    if (!query.exec()) {
        qDebug() << "addBill failed:" << query.lastError().text();
        return false;
    }

    // 更新账户余额
    double delta = (type == "收入") ? amount : -amount;
    QSqlQuery updateAcc;
    updateAcc.prepare("UPDATE accounts SET balance = balance + ? WHERE id = ?");
    updateAcc.addBindValue(delta);
    updateAcc.addBindValue(accountId);
    updateAcc.exec();

    return true;
}

bool databaseManager::updateBill(int id, int accountId, int categoryId,
                                  const QString &date, const QString &type,
                                  double amount, const QString &remark)
{
    // 先恢复旧账单对账户余额的影响
    QSqlQuery oldQuery;
    oldQuery.prepare("SELECT account_id, type, amount FROM bills WHERE id = ?");
    oldQuery.addBindValue(id);
    if (oldQuery.exec() && oldQuery.next()) {
        int oldAccountId = oldQuery.value(0).toInt();
        QString oldType = oldQuery.value(1).toString();
        double oldAmount = oldQuery.value(2).toDouble();
        double oldDelta = (oldType == "收入") ? -oldAmount : oldAmount;

        QSqlQuery revert;
        revert.prepare("UPDATE accounts SET balance = balance + ? WHERE id = ?");
        revert.addBindValue(oldDelta);
        revert.addBindValue(oldAccountId);
        revert.exec();
    }

    QSqlQuery query;
    query.prepare("UPDATE bills SET account_id = ?, category_id = ?, date = ?, type = ?, amount = ?, remark = ? "
                  "WHERE id = ?");
    query.addBindValue(accountId);
    query.addBindValue(categoryId);
    query.addBindValue(date);
    query.addBindValue(type);
    query.addBindValue(amount);
    query.addBindValue(remark);
    query.addBindValue(id);

    if (!query.exec()) {
        qDebug() << "updateBill failed:" << query.lastError().text();
        return false;
    }

    // 应用新账单对账户余额的影响
    double newDelta = (type == "收入") ? amount : -amount;
    QSqlQuery updateAcc;
    updateAcc.prepare("UPDATE accounts SET balance = balance + ? WHERE id = ?");
    updateAcc.addBindValue(newDelta);
    updateAcc.addBindValue(accountId);
    updateAcc.exec();

    return true;
}

bool databaseManager::deleteBill(int id)
{
    // 恢复账户余额
    QSqlQuery oldQuery;
    oldQuery.prepare("SELECT account_id, type, amount FROM bills WHERE id = ?");
    oldQuery.addBindValue(id);
    if (oldQuery.exec() && oldQuery.next()) {
        int accountId = oldQuery.value(0).toInt();
        QString type = oldQuery.value(1).toString();
        double amount = oldQuery.value(2).toDouble();
        double delta = (type == "收入") ? -amount : amount;

        QSqlQuery revert;
        revert.prepare("UPDATE accounts SET balance = balance + ? WHERE id = ?");
        revert.addBindValue(delta);
        revert.addBindValue(accountId);
        revert.exec();
    }

    QSqlQuery query;
    query.prepare("DELETE FROM bills WHERE id = ?");
    query.addBindValue(id);
    return query.exec();
}

QVector<BillInfo> databaseManager::getBills(int userId)
{
    QVector<BillInfo> result;
    QSqlQuery query;
    query.prepare(
        "SELECT b.id, b.account_id, b.category_id, b.date, b.type, b.amount, b.remark, "
        "a.name, c.name "
        "FROM bills b "
        "LEFT JOIN accounts a ON b.account_id = a.id "
        "LEFT JOIN categories c ON b.category_id = c.id "
        "WHERE b.user_id = ? "
        "ORDER BY b.date DESC, b.id DESC"
    );
    query.addBindValue(userId);
    if (query.exec()) {
        while (query.next()) {
            BillInfo b;
            b.id = query.value(0).toInt();
            b.accountId = query.value(1).toInt();
            b.categoryId = query.value(2).toInt();
            b.date = query.value(3).toString();
            b.type = query.value(4).toString();
            b.amount = query.value(5).toDouble();
            b.remark = query.value(6).toString();
            b.accountName = query.value(7).toString();
            b.categoryName = query.value(8).toString();
            result.append(b);
        }
    }
    return result;
}

QVector<BillInfo> databaseManager::getBillsByDateRange(int userId, const QString &startDate, const QString &endDate)
{
    QVector<BillInfo> result;
    QSqlQuery query;
    query.prepare(
        "SELECT b.id, b.account_id, b.category_id, b.date, b.type, b.amount, b.remark, "
        "a.name, c.name "
        "FROM bills b "
        "LEFT JOIN accounts a ON b.account_id = a.id "
        "LEFT JOIN categories c ON b.category_id = c.id "
        "WHERE b.user_id = ? AND b.date >= ? AND b.date <= ? "
        "ORDER BY b.date DESC, b.id DESC"
    );
    query.addBindValue(userId);
    query.addBindValue(startDate);
    query.addBindValue(endDate);
    if (query.exec()) {
        while (query.next()) {
            BillInfo b;
            b.id = query.value(0).toInt();
            b.accountId = query.value(1).toInt();
            b.categoryId = query.value(2).toInt();
            b.date = query.value(3).toString();
            b.type = query.value(4).toString();
            b.amount = query.value(5).toDouble();
            b.remark = query.value(6).toString();
            b.accountName = query.value(7).toString();
            b.categoryName = query.value(8).toString();
            result.append(b);
        }
    }
    return result;
}

QVector<BillInfo> databaseManager::getBillsByType(int userId, const QString &type)
{
    QVector<BillInfo> result;
    QSqlQuery query;
    query.prepare(
        "SELECT b.id, b.account_id, b.category_id, b.date, b.type, b.amount, b.remark, "
        "a.name, c.name "
        "FROM bills b "
        "LEFT JOIN accounts a ON b.account_id = a.id "
        "LEFT JOIN categories c ON b.category_id = c.id "
        "WHERE b.user_id = ? AND b.type = ? "
        "ORDER BY b.date DESC, b.id DESC"
    );
    query.addBindValue(userId);
    query.addBindValue(type);
    if (query.exec()) {
        while (query.next()) {
            BillInfo b;
            b.id = query.value(0).toInt();
            b.accountId = query.value(1).toInt();
            b.categoryId = query.value(2).toInt();
            b.date = query.value(3).toString();
            b.type = query.value(4).toString();
            b.amount = query.value(5).toDouble();
            b.remark = query.value(6).toString();
            b.accountName = query.value(7).toString();
            b.categoryName = query.value(8).toString();
            result.append(b);
        }
    }
    return result;
}

QVector<BillInfo> databaseManager::searchBills(int userId, const QString &keyword,
                                                const QString &startDate, const QString &endDate,
                                                const QString &type, int categoryId)
{
    QVector<BillInfo> result;
    QString sql =
        "SELECT b.id, b.account_id, b.category_id, b.date, b.type, b.amount, b.remark, "
        "a.name, c.name "
        "FROM bills b "
        "LEFT JOIN accounts a ON b.account_id = a.id "
        "LEFT JOIN categories c ON b.category_id = c.id "
        "WHERE b.user_id = ?";
    QVariantList params;
    params << userId;

    if (!keyword.isEmpty()) {
        sql += " AND b.remark LIKE ?";
        params << ("%" + keyword + "%");
    }
    if (!startDate.isEmpty()) {
        sql += " AND b.date >= ?";
        params << startDate;
    }
    if (!endDate.isEmpty()) {
        sql += " AND b.date <= ?";
        params << endDate;
    }
    if (!type.isEmpty()) {
        sql += " AND b.type = ?";
        params << type;
    }
    if (categoryId > 0) {
        sql += " AND b.category_id = ?";
        params << categoryId;
    }

    sql += " ORDER BY b.date DESC, b.id DESC";

    QSqlQuery query;
    query.prepare(sql);
    for (const auto &p : params)
        query.addBindValue(p);

    if (query.exec()) {
        while (query.next()) {
            BillInfo b;
            b.id = query.value(0).toInt();
            b.accountId = query.value(1).toInt();
            b.categoryId = query.value(2).toInt();
            b.date = query.value(3).toString();
            b.type = query.value(4).toString();
            b.amount = query.value(5).toDouble();
            b.remark = query.value(6).toString();
            b.accountName = query.value(7).toString();
            b.categoryName = query.value(8).toString();
            result.append(b);
        }
    }
    return result;
}

// ==================== 预算管理 ====================

bool databaseManager::setBudget(int userId, const QString &month, double amount)
{
    QSqlQuery query;
    query.prepare("INSERT INTO budgets (user_id, month, amount) VALUES (?, ?, ?) "
                  "ON CONFLICT(user_id, month) DO UPDATE SET amount = ?");
    query.addBindValue(userId);
    query.addBindValue(month);
    query.addBindValue(amount);
    query.addBindValue(amount);
    return query.exec();
}

BudgetInfo databaseManager::getBudget(int userId, const QString &month)
{
    BudgetInfo b;
    QSqlQuery query;
    query.prepare("SELECT id, month, amount FROM budgets WHERE user_id = ? AND month = ?");
    query.addBindValue(userId);
    query.addBindValue(month);
    if (query.exec() && query.next()) {
        b.id = query.value(0).toInt();
        b.month = query.value(1).toString();
        b.amount = query.value(2).toDouble();
    }
    return b;
}

double databaseManager::getMonthExpense(int userId, const QString &month)
{
    QSqlQuery query;
    query.prepare("SELECT COALESCE(SUM(amount), 0) FROM bills "
                  "WHERE user_id = ? AND type = '支出' AND date LIKE ?");
    query.addBindValue(userId);
    query.addBindValue(month + "%");
    if (query.exec() && query.next())
        return query.value(0).toDouble();
    return 0.0;
}

double databaseManager::getMonthIncome(int userId, const QString &month)
{
    QSqlQuery query;
    query.prepare("SELECT COALESCE(SUM(amount), 0) FROM bills "
                  "WHERE user_id = ? AND type = '收入' AND date LIKE ?");
    query.addBindValue(userId);
    query.addBindValue(month + "%");
    if (query.exec() && query.next())
        return query.value(0).toDouble();
    return 0.0;
}

// ==================== 统计 ====================

QVector<QPair<QString, double>> databaseManager::getCategoryStats(int userId, const QString &type,
                                                                   const QString &startDate, const QString &endDate)
{
    QVector<QPair<QString, double>> result;
    QSqlQuery query;
    query.prepare(
        "SELECT c.name, COALESCE(SUM(b.amount), 0) "
        "FROM bills b "
        "JOIN categories c ON b.category_id = c.id "
        "WHERE b.user_id = ? AND b.type = ? AND b.date >= ? AND b.date <= ? "
        "GROUP BY c.name ORDER BY SUM(b.amount) DESC"
    );
    query.addBindValue(userId);
    query.addBindValue(type);
    query.addBindValue(startDate);
    query.addBindValue(endDate);
    if (query.exec()) {
        while (query.next())
            result.append({query.value(0).toString(), query.value(1).toDouble()});
    }
    return result;
}

QVector<QPair<QString, double>> databaseManager::getDailyStats(int userId, const QString &type,
                                                                const QString &startDate, const QString &endDate)
{
    QVector<QPair<QString, double>> result;
    QSqlQuery query;
    query.prepare(
        "SELECT date, COALESCE(SUM(amount), 0) "
        "FROM bills "
        "WHERE user_id = ? AND type = ? AND date >= ? AND date <= ? "
        "GROUP BY date ORDER BY date"
    );
    query.addBindValue(userId);
    query.addBindValue(type);
    query.addBindValue(startDate);
    query.addBindValue(endDate);
    if (query.exec()) {
        while (query.next())
            result.append({query.value(0).toString(), query.value(1).toDouble()});
    }
    return result;
}

// ==================== 导出 CSV ====================

bool databaseManager::exportToCSV(int userId, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "日期,类型,账户,分类,金额,备注\n";

    auto bills = getBills(userId);
    for (const auto &b : bills) {
        out << b.date << ","
            << b.type << ","
            << b.accountName << ","
            << b.categoryName << ","
            << b.amount << ","
            << "\"" << b.remark << "\"\n";
    }

    file.close();
    return true;
}