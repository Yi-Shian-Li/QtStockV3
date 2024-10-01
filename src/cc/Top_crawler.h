#pragma once
#include "spot.hpp"
#include <vector>
#include <unordered_set>
#include <QString>
#include <QDateTime>

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <codecvt>

using json = nlohmann::json;

class TWSEDatabase {
public:
    TWSEDatabase();
    bool fetch(const std::string& stockID);

    std::string getStockID();
    int getTotalDays();
    std::vector<qreal> getHigh();
    std::vector<qreal> getLow();
    std::vector<qreal> getClose();
    std::vector<qreal> closeFilled();
    std::vector<qreal> getOpen();
    std::vector<int> getVolume();
    std::vector<int> getTransactions();
    std::vector<QDateTime> getTimestamps();
    std::vector<QDateTime> getTimestampsFilled();
    std::vector<qreal> getTimestampsFilledQreal();
    std::vector<Spot> getSpots();
    std::pair<QDateTime, qreal> GetLatestClosePrice();

    bool fillMissingDate();
    template<int window_size>
    std::vector<qreal> getMA();
    std::vector<qreal> getMA5();
    std::vector<qreal> getMA10();

private:
    std::string stockID_;
    std::vector<Spot> spots_;

    std::vector<qreal> high_;
    std::vector<qreal> low_;
    std::vector<qreal> close_;
    std::vector<qreal> open_;
    std::vector<int> volume_;
    std::vector<int> transactions_;
    std::vector<QDateTime> timeStamps_;

    std::vector<qreal> closeFilled_;
    std::vector<QDateTime> timeStampsFilled_;
    std::vector<qreal> timeStampFilledQreal_;

    int totalDays_ = 0;
    const std::string DUMMY = "011";
    int retryNum = 1;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    void log(const QString& message);
    std::string WStringToUTF8(const std::wstring& wstr);
    std::string yearPreprocess(const std::string&);
    QDateTime toQDateTime(const std::string&);

    void parseData(const std::string& rawData);
    void clearAll();
    void fetchAndParseData(const QString& dateSpot);
    double clearStringComma(const std::string&);
};

class TWSEList {
public:
    TWSEList();
    std::map<std::string, std::pair<std::string, std::string>> getStockList();

private:
    void setBlackList();
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);
    void matchTR(const std::string& rawContent);
    void matchTD(const std::string& trContent, int& flagForDone);
    void matchTDItem(const std::vector<std::string>& tdContents);
    void log(const QString& message);

    const char* url_ = "https://isin.twse.com.tw/isin/C_public.jsp?strMode=2";
    std::map<std::string, std::pair<std::string, std::string>> stockDict_;
    std::unordered_set<std::string> blackList_;
};