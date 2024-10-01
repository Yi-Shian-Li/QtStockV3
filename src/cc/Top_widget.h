#pragma once
#include <QtCharts>
#include <QTableWidget>
#include <QMouseEvent>
#include <QCandlestickSeries>
#include <QSqlDatabase>
#include "spot.hpp"
#include <memory>
#include "Top_crawler.h"
#include "Top_strategy.h"

class Logger;
class sqliteConnectionFactory;
class analyzeStart;
class WChartView;

/*!
* @brief Struct containing stock information.
*/
struct StockInfo {
	QString ID;    ///< Stock ID.
	QString Name;  ///< Stock name.
	QString Categ; ///< Stock category.
};



class Crosshairs {
public:
	Crosshairs();
	void setChart(QChart* chart);
	void updatePosition(QPointF position);

private:
	QGraphicsLineItem* m_xLine, * m_yLine;
	QGraphicsTextItem* m_yText;
	QChart* m_chart;
};

class RoundedWidget : public QWidget {
	Q_OBJECT
public:
	RoundedWidget(QWidget* parent = nullptr);
	void init();

public slots:
	void filterResult(const QString& msg);

private slots:
	void cellClicked(const QModelIndex& index);
	
signals:
	void stockSelected(StockInfo& stockInfo);

protected:
	void paintEvent(QPaintEvent* ev);

private:
	void configToolbar();
	void configBorderEffect();
	void configLayout();
	void configTable();
	void configModel();

	std::unique_ptr<QVBoxLayout> layout_;

	std::unique_ptr<QHBoxLayout> toolbar_;
	std::unique_ptr<QLabel> header_;
	std::unique_ptr<QLabel> latestUpdateTime_;

	std::unique_ptr<QSortFilterProxyModel> proxyModel_;
	std::unique_ptr<QStandardItemModel> model_;
	std::unique_ptr<QTableView> table_;
	int lastClicked_ = -1;
};

/*class WOrderTableWidget : public QTableWidget {
	Q_OBJECT
public:
	WOrderTableWidget(QTableWidget* tableWidget, const QString& dbName, const QString& sortOrder = "DESC");

public slots:
	void refreshTable();

signals:
	void stockSelected(StockInfo& stockInfo);

private slots:
	void cellClicked(int row, int column);

private:
	void connectToDatabase(const QString& dbName);
	void sqlToDefaultMode(QSqlQuery&);
	void log(const std::string& message);
	void setTableStyle();

	QTableWidget* tableWidget_;

	QString dbName_;
	int columnNum_ = 0;
	QString sortOrder_;
	int lastClicked_ = -1;
};*/


class RoundedOrderWidget : public QWidget {
	Q_OBJECT
public:
	RoundedOrderWidget(QWidget* parent = nullptr);

	void setLinkedChartView(WChartView* linkedChart);
	void init();

public slots:
	void refreshTable();
	void refreshTimeLabel(const QString& time);
	
private slots:
	void upTrendTableCellClicked(int row, int column);
	void downTrendTableCellClicked(int row, int column);
	
signals:
	void stockSelected(StockInfo& stockInfo);

protected:
	void paintEvent(QPaintEvent* ev);

private:
	void configToolbar();
	void configBorderEffect();
	void configLayout();

	void configUpTab();
	void configDownTab();

	void databaseToTable(QSqlDatabase readFromDB, QTableWidget* tableWidget, const QString& sortOrder);
	
	void sqlToDefaultMode(QSqlQuery&);
	void log(const std::string& message);
	// void setTableStyle();

	QString dbName_ = "order.db";
	int columnNum_ = 0;
	int upTableLastClicked_ = -1;
	int downTableLastClicked_ = -1;

	std::unique_ptr<QVBoxLayout> layout_;
	std::unique_ptr<QLabel> header_;
	std::unique_ptr<QHBoxLayout> toolbar_;

	std::unique_ptr<QTabWidget> mainTabwidget_;
	std::unique_ptr<QTableWidget> upTrendTable_;
	std::unique_ptr<QTableWidget> downTrendTable_;
	
	std::unique_ptr<QPushButton> strategyStart_;
	std::unique_ptr<QLabel> latestUpdateTime_;
	std::shared_ptr<analyzeStart> analyze_;
	WChartView* linkedChartView_ = nullptr;
	
};


class WChartControlPanel {
public:
	WChartControlPanel(QPushButton* rstBtn, QPushButton* handBtn, QPushButton* ma5Btn, QPushButton* ma10Btn, QPushButton* screenShot, QPushButton* indicatorBtn);
	QPushButton* getBtn_1();
	QPushButton* getBtn_2();
	QPushButton* getBtn_3();
	QPushButton* getBtn_4();
	QPushButton* getShotBtn();
	QPushButton* getIndicatorBtn();

private:
	std::unique_ptr<QPushButton> rstBtn_;
	std::unique_ptr<QPushButton> handBtn_;
	std::unique_ptr<QPushButton> ma5Btn_;
	std::unique_ptr<QPushButton> ma10Btn_;
	std::unique_ptr<QPushButton> screenShot_;
	std::unique_ptr<QPushButton> indicatorBtn_;
};

class WChartView : public QChartView {
	Q_OBJECT
public:
	WChartView(QWidget* parent);

	QDateTimeAxis* getXAxis();
	QValueAxis* getYAxis();
	QValueAxis* getYAxisVolume();
	QChart* getChart();

	void setController(WChartControlPanel* chartControl);
	void updateCandlestickSeries();

	template<int window_size>
	std::vector<double> GetMA();

	std::vector<double> GetMA5();
	std::vector<double> GetMA10();
	void updateCandles();
	void updateMA();

public slots:
	void updateChart(StockInfo& stockInfo);
	void colorSwitch();
	void plotMA5();
	void plotMA10();
	void screenShot();

	void updateIndicator(QVector<QPair<QString, qreal>>&, QVector<QPair<QString, qreal>>&);
	void plotIndicator();

protected:
	void mouseMoveEvent(QMouseEvent*);
	void enterEvent(QEnterEvent*);
	void leaveEvent(QEvent*);

private slots:
	void onCandlestickHovered(bool state, QCandlestickSet* set);
	// void onMA5Hovered(const QPointF& p, bool state);
private:
	static bool isNormal;
	// void fillMissingDate();
	void log(const std::string& message);
	void setChartData();
	void setChartAxis();
	void setSubChart();
	void setChartStyle();
	void setChartTtile();

	void initializeMA5Ptr();
	void initializeMA10Ptr();
	void initializeOrderScattersPtr();
	void initializeCrossScattersPtr();

	

	std::vector<Spot> spots_;
	// void showCursor();

	std::unique_ptr<TWSEDatabase> stockdb_;
	WChartControlPanel* chartControl_;

	std::unique_ptr<QChart> chart_;
	Crosshairs* m_crossHair_;

	// main plot
	std::unique_ptr<QCandlestickSeries> series_;
	std::unique_ptr<QDateTimeAxis> axisX_;
	std::unique_ptr<QValueAxis> axisY_;

	// sub plot
	std::unique_ptr<QAreaSeries> areaSeries_;
	std::unique_ptr<QLineSeries> lineSeries_;
	std::unique_ptr<QDateTimeAxis> axisX_Volume_;
	std::unique_ptr<QValueAxis> axisY_Volume_;

	qreal minValue_;
	qreal maxValue_;

	qreal minVolumeValue_;
	qreal maxVolumeValue_;

	QColor increasingColor, decreasingColor;

	// MOVING AVG.
	std::vector<qreal> ma5_;
	std::vector<qreal> ma10_;
	std::vector<qreal> timeStampFilledQreal_;

	std::unique_ptr<QLineSeries> ma5_p_;
	std::unique_ptr<QLineSeries> ma10_p_;

	// INDICATORS
	QVector<QPair<QString, qreal>> orderList_;
	QVector<QPair<QString, qreal>> crossList_;

	std::unique_ptr<QScatterSeries> orderScatters_;
	std::unique_ptr<QScatterSeries> crossScatters_;

	bool userChoosedStock_ = 0;
};


class Logger : public QObject {
	Q_OBJECT
public:
	static Logger& getInstance();
	void log(const QString& message);

signals:
	void logMessage(const QString& message);

private:
	Logger();

};