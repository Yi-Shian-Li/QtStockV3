#include "Top_strategy.h"
#include "Top_widget.h"
#include "Top_utils.h"
#include "Top_crawler.h"

#include <QString>
#include <QTextBrowser>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QMutex>

sqliteConnectionFactory::sqliteConnectionFactory(const QString& dbName, bool useWAL) 
    : dbName_(dbName)
{
}

sqliteConnectionFactory::~sqliteConnectionFactory()
{
    clearConnection();

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbName_);

    if (!db.open()) {
        qDebug() << db.lastError().text();
    }
    else {
        switchToDefaultMode(db);
    }
}


QSqlDatabase sqliteConnectionFactory::createConnection()
{
    QString connectname = QString::number((int)(QThread::currentThread()));

    if (!connMap_.contains(connectname)) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectname);
        db.setDatabaseName(dbName_);

        if (!db.open()) {
            qDebug() << db.lastError().text();
            assert(0);
        }

        switchToWALMode(db);

        connMap_.insert(connectname, db);
    }
    return connMap_.value(connectname);

}

void sqliteConnectionFactory::switchToWALMode(QSqlDatabase& db)
{
    QSqlQuery setting(db);
    setting.exec("PRAGMA journal_mode=WAL;");
    if (setting.next()) {
        QString mode = setting.value(0).toString();
        qDebug() << "Switched mode. Current SQLite mode is: " << mode;
    }

}

void sqliteConnectionFactory::switchToDefaultMode(QSqlDatabase& db)
{
    QSqlQuery setting(db);
    setting.exec("PRAGMA journal_mode=DELETE;");
    if (setting.next()) {
        QString mode = setting.value(0).toString();
        qDebug() << "Switched mode. Current SQLite mode is: " << mode;
    }
}

void sqliteConnectionFactory::clearConnection()
{
    for (auto itr : connMap_) {
        if (itr.isOpen()) itr.close();
    }
    connMap_.clear();
}

testRunable::testRunable(const std::string id, const QSharedPointer<sqliteConnectionFactory>& factory, const std::string& stockNumber, int& counter)
    : factory_(factory), id_(id), stockNumber_(QString::fromStdString(stockNumber)), counter_(counter)
{
}

void testRunable::run() {

    

    QSqlDatabase threadConnect = factory_->createConnection();
    //QSqlQuery orderQuery(threadConnect);
    QSqlQuery crossPointQuery(threadConnect);

    
    TWSEDatabase twseDatabase;
    if (twseDatabase.fetch(id_) == false) return;

    QString symbol_ = QString::fromStdString(twseDatabase.getStockID());
    std::vector<qreal> ma5_ = twseDatabase.getMA5();
    std::vector<qreal> ma10_ = twseDatabase.getMA10();
    std::vector<qreal> close_ = twseDatabase.closeFilled();
    std::vector<QDateTime> series_ = twseDatabase.getTimestampsFilled();
    auto latestDayPrice = twseDatabase.GetLatestClosePrice();
    
    CrossList CL;
    CL.addId(symbol_);
    CL.addatestDayPrice_(latestDayPrice);
    int cross = 0;
    int status = -1;

    for (int i = 9; i < close_.size(); ++i) {
        QVector<QString> queryBuffer;
        if (ma5_.at(i) >= ma10_.at(i) && status != 1) {
            if (status == 0) {
                cross++;
                CL.addSpot(close_.at(i), series_.at(i), i);
                queryBuffer.push_back(symbol_);
                queryBuffer.push_back(series_[i].toString("yyyy-MM-dd"));
                queryBuffer.push_back(QString::number(close_[i]));
            }
            status = 1;
        }
        else if (ma5_.at(i) < ma10_.at(i) && status != 0) {
            if (status == 1) {
                cross++;
                CL.addSpot(close_.at(i), series_.at(i), i);
                queryBuffer.push_back(symbol_);
                queryBuffer.push_back(series_[i].toString("yyyy-MM-dd"));
                queryBuffer.push_back(QString::number(close_[i]));
            }
            status = 0;
        }
        else {
            // qDebug() << "wrong.";
        }
        if (!queryBuffer.empty()) {
            crossPointQuery.prepare("INSERT INTO crossPoints (StockID, Date, Price) VALUES (?, ?, ?)");
            for (int i = 0; i < 3; ++i) {
                crossPointQuery.bindValue(i, queryBuffer[i]);
            }
            if (!crossPointQuery.exec()) {
                qWarning() << "Failed to insert data into crossPoints table";
                qWarning() << crossPointQuery.lastError().text();
            }
        }
    
    }

    counter_++;
    
    if (cross < 3) {
        QString msg = "[" + QString::number(counter_) + "/" + stockNumber_ + "] " + symbol_ + " is skipped";
        log(msg);
        return; //log(QString::number(cross));
    }
    
    QString msg = "[" + QString::number(counter_) + "/" + stockNumber_ + "] " + symbol_ + " finished, target: " + QString::number(cross);
    log(msg);

    createOrder(CL, close_, series_);

    /*CL.printList();

    std::vector<CrossSpot> crosslist_ = CL.getList();
    for (auto& i : crosslist_) {
        QString p = QString::number(i.price_);
        QString d = i.date_.toString("yyyy-MM-dd");
        QString msg = "price: " + p + " date: " + d;
        log(msg);
    }*/

    /*orderQuery.prepare("INSERT INTO selectedStocks (stockID) VALUES (?)");
    orderQuery.bindValue(0, symbol_);
    if (!orderQuery.exec()) {
        qWarning() << "Failed to insert data into selectedStocks table";
        qWarning() << orderQuery.lastError().text();
    }*/

    /**/



}


void testRunable::createOrder(CrossList CL, std::vector<qreal> close_, std::vector<QDateTime> series_) {

    QSqlDatabase threadConnect = factory_->createConnection();
    QSqlQuery stockOrderQuery_(threadConnect);

    QMutexLocker locker(&createOrder_);
    std::vector<CrossSpot> crossList = CL.getList();
    QString symbol_ = CL.getId();
    auto latestDayPrice = CL.getLatestDayPrice();

    for (int i = 2; i < CL.size(); ++i) {
        int firstCrossIdx = crossList.at(i - 2).idx_;
        int secondCrossIdx = crossList.at(i - 1).idx_;
        int thirdCrossIdx = crossList.at(i).idx_;
        int scanIdx = crossList.at(i).idx_ + 1;
        int scanEnd = series_.size();

        while (scanIdx < scanEnd) {
            try {
                if (close_.at(scanIdx) > close_.at(firstCrossIdx)) {
                    
                    stockOrderQuery_.prepare("INSERT INTO orderList (StockID, Date, Price, ROI, CrossDate1, CrossDate2, CrossDate3, CrossPrice1, CrossPrice2, CrossPrice3) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

                    stockOrderQuery_.bindValue(0, symbol_);
                    stockOrderQuery_.bindValue(1, series_.at(scanIdx).toString("yyyy-MM-dd"));
                    stockOrderQuery_.bindValue(2, close_.at(scanIdx));

                    auto roi_ = 100 * (latestDayPrice.second - close_.at(scanIdx)) / close_.at(scanIdx);
                    QVariant roundedValue = QVariant(roi_).toFloat(); // Convert the value to a float
                    roundedValue = QString::number(roundedValue.toFloat(), 'f', 2); // Round the value to two decimal places
                    stockOrderQuery_.bindValue(3, roundedValue);
                    stockOrderQuery_.bindValue(4, crossList.at(i - 2).date_.toString("yyyy-MM-dd"));
                    stockOrderQuery_.bindValue(5, crossList.at(i - 1).date_.toString("yyyy-MM-dd"));
                    stockOrderQuery_.bindValue(6, crossList.at(i).date_.toString("yyyy-MM-dd"));
                    stockOrderQuery_.bindValue(7, crossList.at(i - 2).price_);
                    stockOrderQuery_.bindValue(8, crossList.at(i - 1).price_);
                    stockOrderQuery_.bindValue(9, crossList.at(i).price_);

                    if (!stockOrderQuery_.exec()) {
                        qWarning() << "Failed to insert data into orderList table";
                        qWarning() << stockOrderQuery_.lastError().text();
                    }
                    break;
                }
            }
            catch (const std::out_of_range& e) {
                // Handle the out-of-range exception, e.g., by logging an error or taking appropriate action.
                qWarning() << "Caught an out_of_range exception: " << e.what();
                break; // Exit the loop or take other appropriate action
            }
            scanIdx++;
        }
    }
}



void testRunable::log(const QString& message)
{
    Logger& logger = Logger::getInstance();
    logger.log(message);
}


void testRunable::insertCrossPoint(const QString& symbol, const QDateTime& date, double price)
{
    QSqlDatabase threadConnect = factory_->createConnection();
    QVector<QString> queryBuffer;
    queryBuffer.push_back(symbol);
    queryBuffer.push_back(date.toString("yyyy-MM-dd"));
    queryBuffer.push_back(QString::number(price));

    if (!queryBuffer.empty()) {
        
        QSqlQuery crossPointQuery(threadConnect);
        QMutexLocker locker(&generalMutex_);
        crossPointQuery.prepare("INSERT INTO crossPoints (StockID, Date, Price) VALUES (?, ?, ?)");
        for (int i = 0; i < 3; ++i) {
            crossPointQuery.bindValue(i, queryBuffer[i]);
        }
        if (!crossPointQuery.exec()) {
            qWarning() << "Failed to insert data into crossPoints table";
            qWarning() << crossPointQuery.lastError().text();
        }
    }
}


CrossList::CrossList() {}

void CrossList::printList() {
    for (auto& i : vec_)
        qDebug() << "price: " << i.price_ << " date: " << i.date_.toString("yyyy-MM-dd");
}


int CrossList::size() {
    return vec_.size();
}

void CrossList::clear() {
    vec_.clear();
}

void CrossList::addId(const QString& id)
{
    id_ = id;
}

void CrossList::addatestDayPrice_(const std::pair<QDateTime, qreal>& latestDayPrice)
{
    latestDayPrice_ = latestDayPrice;
}

void CrossList::addSpot(const qreal& price, const QDateTime& date, const int& idx) {
    CrossSpot spot_;
    spot_.price_ = price;
    spot_.date_ = date;
    spot_.idx_ = idx;
    vec_.emplace_back(spot_);
}

std::vector<CrossSpot> CrossList::getList() {
    return vec_;
}

std::pair<QDateTime, qreal> CrossList::getLatestDayPrice()
{
    return latestDayPrice_;
}

QString CrossList::getId()
{
    return id_;
}

strategyCrossMA::strategyCrossMA()
{
    //logCollector_ = new workersLogCollector();
    //connect(this, &strategyCrossMA::logMessage, logCollector_, &workersLogCollector::handleLogMessage);

    TWSEList* twseList = new TWSEList();
    stockList_ = twseList->getStockList();

}

void strategyCrossMA::run()
{
    log("NOTICE: start analyzing by using strategy 1.");
    executeStockQueries();
    // QSqlDatabase::removeDatabase("initConnection"); // Remove the write DB connection
    emit finished();
}

void strategyCrossMA::log(const QString& message) {
    Logger::getInstance().log(message);
}


bool strategyCrossMA::initWriteDB(QSqlDatabase& db, const QString& dbName)
{
    db = QSqlDatabase::addDatabase("QSQLITE", "initConnection");
    db.setDatabaseName(dbName);

    if (!db.open()) {
        log("ERROR: Failed to open order database" + db.lastError().text());
        return false;
    }

    QSqlQuery initCrossPointQuery_(db);
    initCrossPointQuery_.exec("DROP TABLE crossPoints");
    initCrossPointQuery_.exec("CREATE TABLE IF NOT EXISTS crossPoints ("
        "StockID TEXT NOT NULL,"
        "Date TEXT NOT NULL,"
        "Price REAL NOT NULL);");

    QSqlQuery initOrderlistQuery_(db);
    initOrderlistQuery_.exec("DROP TABLE selectedStocks");
    initOrderlistQuery_.exec("CREATE TABLE IF NOT EXISTS selectedStocks ("
        "StockID TEXT NOT NULL);");


    QSqlQuery initStockOrderQuery_(db);
    initStockOrderQuery_.exec("DROP TABLE orderList");
    initStockOrderQuery_.exec("CREATE TABLE IF NOT EXISTS orderList ("
        "StockID TEXT NOT NULL,"
        "Date TEXT NOT NULL,"
        "Price REAL NOT NULL,"
        "ROI REAL NOT NULL,"
        "CrossDate1 TEXT NOT NULL,"
        "CrossDate2 TEXT NOT NULL,"
        "CrossDate3 TEXT NOT NULL,"
        "CrossPrice1 REAL NOT NULL,"
        "CrossPrice2 REAL NOT NULL,"
        "CrossPrice3 REAL NOT NULL);");

    return true;
}

void strategyCrossMA::executeStockQueries()
{
    if (!initWriteDB(mainConnect, "order.db")) {
        log("ERROR: order database initialization failed before exec strategy.");
        return;
    }
    else {
        log("NOTICE: order database initialization success before exec strategy.");
    }

    QSharedPointer<sqliteConnectionFactory> factory(new sqliteConnectionFactory("order.db"));
    QThreadPool threadPool;
    log("NOTICE: auto select ideal thread number.");

    auto start = std::chrono::steady_clock::now();
    std::string stockNumber = std::to_string(stockList_.size());
    int counter = 0;

    for (std::map<std::string, std::pair<std::string, std::string>>::iterator itr = stockList_.begin(); itr != stockList_.end();++itr) {
        std::string id = itr->first;
        testRunable* query = new testRunable(id, factory, stockNumber, counter);
        threadPool.start(query);
    }

    threadPool.waitForDone();

    auto end = std::chrono::steady_clock::now();
    log("NOTICE: strategy exec runtime " + QString::number(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) + " ms");
    
    // QSqlDatabase switchMode = factory->createConnection();

}

analyzeStart::analyzeStart(QObject* parent) : QObject(parent)
{
    crossMA_ = std::make_unique<strategyCrossMA>();
    preUpdateDatabaseLabel(mainConnect);
    // crossMA_ = std::make_unique<strategyCrossMA>();

    //logCollector_ = new workersLogCollector();
    //connect(crossMA_, &strategyCrossMA::logMessage, logCollector_, &workersLogCollector::handleLogMessage);
}

analyzeStart::~analyzeStart()
{
    qDebug() << "analyzeStart destructor called";
}

QString analyzeStart::getLastTime()
{
    return lastTime_;
}

void analyzeStart::handleCrossMAFinished()
{
    // Perform any actions you need after the heavy task has finished
    postUpdateDatabaseLabel(mainConnect);
    emit refreshTableRequested();
}


void analyzeStart::start()
{
    QThread* workerThread = new QThread;
    workerThread->start();

    
    crossMA_->moveToThread(workerThread);

    connect(crossMA_.get(), &strategyCrossMA::finished, workerThread, &QThread::quit);
    connect(crossMA_.get(), &strategyCrossMA::finished, this, &analyzeStart::handleCrossMAFinished);

    QMetaObject::invokeMethod(crossMA_.get(), "run", Qt::QueuedConnection);

    connect(qApp, &QApplication::aboutToQuit, workerThread, &QThread::quit);

}

void analyzeStart::preUpdateDatabaseLabel(QSqlDatabase& mainConnect)
{
    mainConnect = QSqlDatabase::addDatabase("QSQLITE", "preUpdate");
    mainConnect.setDatabaseName("order.db");

    if (!mainConnect.open()) {
        log("ERROR: Failed to check the database timestamp. " + mainConnect.lastError().text());
        return;
    }

    QSqlQuery timeQuery_(mainConnect);
    QString cmd = "CREATE TABLE IF NOT EXISTS updateTime ("
        "timeStamp TEXT NOT NULL PRIMARY KEY);";

    if (timeQuery_.exec(cmd)) {
        QString selectCmd = "SELECT timeStamp FROM updateTime LIMIT 1;";
        if (timeQuery_.exec(selectCmd) && timeQuery_.next()) {
            QString getTime = timeQuery_.value(0).toString();
            if (!getTime.isEmpty()) {
                emit updateTimeLabel(getTime);
                lastTime_ = getTime;
            }
            else {
                lastTime_ = "---";
                log("WARNING: no timestamp in the database. You may be using an older version of the database.");
            }
        }
        else {
            lastTime_ = "---";
            log("ERROR: Failed to retrieve timestamp from 'updateTime' table. " + timeQuery_.lastError().text());
        }

    }
    
}


void analyzeStart::postUpdateDatabaseLabel(QSqlDatabase& mainConnect)
{
    mainConnect = QSqlDatabase::addDatabase("QSQLITE", "postUpdate");
    mainConnect.setDatabaseName("order.db");

    if (!mainConnect.open()) {
        log("ERROR: Failed to check the database timestamp. " + mainConnect.lastError().text());
        return;
    }

    QSqlQuery timeQuery_(mainConnect);
    

    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd-hh:mm:ss");

    timeQuery_.exec("DROP TABLE updateTime");
    QString cmd = "CREATE TABLE IF NOT EXISTS updateTime ("
        "timeStamp TEXT NOT NULL PRIMARY KEY);";

    if (!timeQuery_.exec(cmd)) {
        log("ERROR: Failed to insert timestamp. " + timeQuery_.lastError().text());
    }
    else {
        timeQuery_.prepare("INSERT OR REPLACE INTO updateTime (timeStamp) VALUES (:currentTime)");
        timeQuery_.bindValue(":currentTime", currentTime);

        if (timeQuery_.exec()) {
            emit updateTimeLabel(currentTime);
        }
        else {
            log("ERROR: Failed to insert timestamp into 'updateTime' table. " + timeQuery_.lastError().text());
        }
    }
    
}

void analyzeStart::requestIndicatorFromDB(StockInfo& stockInfo)
{
    // QSqlDatabase readIndicator;
    if (QSqlDatabase::contains("readIndicator")) {
        mainConnect = QSqlDatabase::database("readIndicator");
    }
    else {
        mainConnect = QSqlDatabase::addDatabase("QSQLITE", "readIndicator");
    }
    mainConnect.setDatabaseName("order.db");

    if (mainConnect.open()) {
        QSqlQuery query(mainConnect);
        QString id_ = stockInfo.ID;

        QString sqlQuery = "SELECT StockID, Date, Price, crossDate1, crossDate2, crossDate3, crossPrice1, crossPrice2, crossPrice3 "
            "FROM orderList "
            "WHERE StockID=" + id_ + " "
            "ORDER BY Date ASC ";

        QVector<QPair<QString, qreal>> orderList;
        QVector<QPair<QString, qreal>> crossList;

        if (query.exec(sqlQuery)) {
            while (query.next()) {
                QString date = query.value("Date").toString();
                qreal price  = query.value("Price").toFloat();

                QString date1 = query.value("crossDate1").toString();
                QString date2 = query.value("crossDate2").toString();
                QString date3 = query.value("crossDate3").toString();
                qreal price1 = query.value("crossPrice1").toFloat();
                qreal price2 = query.value("crossPrice2").toFloat();
                qreal price3 = query.value("crossPrice3").toFloat();

                orderList.push_back(qMakePair(date, price));
                crossList.push_back(qMakePair(date1, price1));
                crossList.push_back(qMakePair(date2, price2));
                crossList.push_back(qMakePair(date3, price3));
            }
        }
        emit sendIndicatorToChart(orderList, crossList);
    }
}

void analyzeStart::log(const QString& message)
{
    Logger::getInstance().log(message);
}
