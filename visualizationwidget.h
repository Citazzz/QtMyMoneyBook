#ifndef VISUALIZATIONWIDGET_H
#define VISUALIZATIONWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>

class QChartView;

class VisualizationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VisualizationWidget(int userId, QWidget *parent = nullptr);

private slots:
    void onRefresh();

private:
    void setupUI();
    void showPieChart();
    void showLineChart();

    int m_userId;
    QComboBox *m_chartType;
    QComboBox *m_billType;
    QDateEdit *m_startDate;
    QDateEdit *m_endDate;
    QPushButton *m_refreshBtn;
    QChartView *m_chartView;
};

#endif // VISUALIZATIONWIDGET_H
