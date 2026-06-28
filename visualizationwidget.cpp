#include "visualizationwidget.h"
#include "databaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDate>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

VisualizationWidget::VisualizationWidget(int userId, QWidget *parent)
    : QWidget(parent), m_userId(userId)
{
    setupUI();
    showPieChart();
}

void VisualizationWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *controlLayout = new QHBoxLayout;

    controlLayout->addWidget(new QLabel(QStringLiteral("图表类型:")));
    m_chartType = new QComboBox;
    m_chartType->addItem(QStringLiteral("饼图（分类占比）"), "pie");
    m_chartType->addItem(QStringLiteral("折线图（日趋势）"), "line");
    controlLayout->addWidget(m_chartType);

    controlLayout->addWidget(new QLabel(QStringLiteral("收支类型:")));
    m_billType = new QComboBox;
    m_billType->addItem(QStringLiteral("支出"), QStringLiteral("支出"));
    m_billType->addItem(QStringLiteral("收入"), QStringLiteral("收入"));
    controlLayout->addWidget(m_billType);

    controlLayout->addWidget(new QLabel(QStringLiteral("开始:")));
    m_startDate = new QDateEdit(QDate::currentDate().addMonths(-1));
    m_startDate->setDisplayFormat("yyyy-MM-dd");
    m_startDate->setCalendarPopup(true);
    controlLayout->addWidget(m_startDate);

    controlLayout->addWidget(new QLabel(QStringLiteral("结束:")));
    m_endDate = new QDateEdit(QDate::currentDate());
    m_endDate->setDisplayFormat("yyyy-MM-dd");
    m_endDate->setCalendarPopup(true);
    controlLayout->addWidget(m_endDate);

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"));
    controlLayout->addWidget(m_refreshBtn);
    controlLayout->addStretch();

    mainLayout->addLayout(controlLayout);

    m_chartView = new QChartView;
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(400);
    mainLayout->addWidget(m_chartView);

    connect(m_refreshBtn, &QPushButton::clicked, this, &VisualizationWidget::onRefresh);
    connect(m_chartType, &QComboBox::currentTextChanged, this, &VisualizationWidget::onRefresh);
}

void VisualizationWidget::onRefresh()
{
    if (m_chartType->currentData().toString() == "pie")
        showPieChart();
    else
        showLineChart();
}

void VisualizationWidget::showPieChart()
{
    auto *chart = new QChart;
    chart->setTitle(QStringLiteral("%1分类占比").arg(m_billType->currentText()));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto *series = new QPieSeries;

    QString startDate = m_startDate->date().toString("yyyy-MM-dd");
    QString endDate = m_endDate->date().toString("yyyy-MM-dd");
    auto stats = databaseManager::instance().getCategoryStats(m_userId, m_billType->currentData().toString(),
                                                               startDate, endDate);

    QVector<QColor> colors = {
        QColor("#5B7FFF"), QColor("#00C897"), QColor("#FF6B6B"), QColor("#F59E0B"),
        QColor("#7C5CFC"), QColor("#38B2AC"), QColor("#ED64A6"), QColor("#4A5568"),
        QColor("#3182CE"), QColor("#DD6B20"), QColor("#48BB78"), QColor("#9F7AEA"),
        QColor("#E53E3E"), QColor("#2B6CB0"), QColor("#975A16")
    };

    int colorIdx = 0;
    double total = 0;
    for (const auto &s : stats) total += s.second;

    for (const auto &s : stats) {
        if (s.second <= 0) continue;
        double pct = total > 0 ? (s.second / total * 100.0) : 0;
        auto *slice = series->append(QStringLiteral("%1 (%2%)").arg(s.first).arg(pct, 0, 'f', 1), s.second);
        slice->setColor(colors[colorIdx % colors.size()]);
        ++colorIdx;
    }

    chart->addSeries(series);
    chart->legend()->setAlignment(Qt::AlignRight);

    auto *oldChart = m_chartView->chart();
    m_chartView->setChart(chart);
    delete oldChart;
}

void VisualizationWidget::showLineChart()
{
    auto *chart = new QChart;
    chart->setTitle(QStringLiteral("%1日趋势").arg(m_billType->currentText()));
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QString startDate = m_startDate->date().toString("yyyy-MM-dd");
    QString endDate = m_endDate->date().toString("yyyy-MM-dd");
    auto stats = databaseManager::instance().getDailyStats(m_userId, m_billType->currentData().toString(),
                                                            startDate, endDate);

    auto *series = new QLineSeries;
    series->setName(m_billType->currentText());

    QStringList categories;
    double maxVal = 0;
    for (int i = 0; i < stats.size(); ++i) {
        series->append(i, stats[i].second);
        categories << stats[i].first;
        if (stats[i].second > maxVal) maxVal = stats[i].second;
    }

    chart->addSeries(series);

    auto *axisX = new QBarCategoryAxis;
    axisX->append(categories);
    axisX->setLabelsAngle(-45);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis;
    axisY->setRange(0, maxVal * 1.1 + 1);
    axisY->setLabelFormat("%.0f");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    auto *oldChart = m_chartView->chart();
    m_chartView->setChart(chart);
    delete oldChart;
}
