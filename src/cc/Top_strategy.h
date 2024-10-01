#pragma once
#include "Top_crawler.h"
#include "Top_strategy.h"
#include "Top_widget.h"
#include <vector>
#include <memory>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <QThread>
#include <QRunnable>
#include <QDebug>
#include <QThreadPool>
#include <QMutex>
#include <QMutexLocker>

#include <QPushButton>
#include <QLabel>

class StockInfo;
class workersLogCollector;

struct CrossSpot {
	qreal price_ = 0;
	QDateTime date_;
	int idx_ = 0;
};

class CrossList {
public:
	CrossList();
	void printList();
	int size();
	void clear();
	void addSpot(const qreal& price, const QDateTime& date, const int& idx);
	void addId(const QString& id);
	void addatestDayPrice_(const std::pair<QDateTime, qreal>& latestDayPrice);

	std::vector<CrossSpot> getList();
	std::pair<QDateTime, qreal> getLatestDayPrice();
	QString getId();
private:
	std::vector<CrossSpot> vec_;
	std::pair<QDateTime, qreal> latestDayPrice_;
	QString id_;
};

/*class WStrategy {
public:
	WStrategy();
	void run();

private:
	bool initReadDB(QSqlDatabase& db, const QString& dbName);
	bool initWriteDB(QSqlDatabase& db, const QString& dbName);
	void executeStockQueries();
	void log(const QString& message);

	QSqlDatabase mainConnect;
	QSqlDatabase initTmp;
};*/



//class stockInfo {
//public:
//	stockInfo(const QString& symbol, const QString& name, const QString& url);
//	QString symbol_;
//	QString name_;
//	QString url_;
//};

class sqliteConnectionFactory {
public:
	sqliteConnectionFactory(const QString& dbName, bool useWAL = true);
	~sqliteConnectionFactory();

	QSqlDatabase createConnection();
	void clearConnection();
	void switchToWALMode(QSqlDatabase& db);
	void switchToDefaultMode(QSqlDatabase& db);
	
private:
	QString dbName_;
	QMap<QString, QSqlDatabase> connMap_;

	
};



class testRunable : public QObject, public QRunnable {
	Q_OBJECT
public:
	testRunable(const std::string id, const QSharedPointer<sqliteConnectionFactory>& factory, const std::string& stockNumber, int& counter);
	void run() override;

private:
	// int processCrossPoints(TWSEDatabase* twseDatabase, CrossList& CL);
	void insertCrossPoint(const QString& symbol, const QDateTime& date, double price);
	void createOrder(CrossList CL, std::vector<qreal> close_, std::vector<QDateTime> series_);
	void log(const QString& message);

	QMutex createOrder_;
	QMutex generalMutex_;
	std::string id_;
	QSharedPointer<sqliteConnectionFactory> factory_;

	QString stockNumber_;
	int& counter_;
};

class strategyCrossMA : public QObject {
	Q_OBJECT
public:
	strategyCrossMA();

public slots:
	void run();

signals:
	void finished();

private:
	bool initWriteDB(QSqlDatabase& db, const QString& dbName);
	void executeStockQueries();
	void log(const QString& message);

	std::map<std::string, std::pair<std::string, std::string>> stockList_;
	QSqlDatabase mainConnect;
};


class analyzeStart : public QObject {
	Q_OBJECT
public:
	analyzeStart(QObject* parent = nullptr);
	~analyzeStart();
	QString getLastTime();

public slots:
	void start();
	void handleCrossMAFinished();
	void requestIndicatorFromDB(StockInfo& stockInfo);

signals:
	void refreshTableRequested();
	void sendIndicatorToChart(QVector<QPair<QString, qreal>>&, QVector<QPair<QString, qreal>>&);
	void updateTimeLabel(const QString& time);
	

private:
	void preUpdateDatabaseLabel(QSqlDatabase &mainConnect);
	void postUpdateDatabaseLabel(QSqlDatabase &mainConnect);
	void log(const QString& message);
	QSqlDatabase mainConnect;

	std::unique_ptr<strategyCrossMA> crossMA_;
	QString lastTime_;
};

