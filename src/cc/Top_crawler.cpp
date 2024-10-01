#include "Top_crawler.h"
#include "Top_StockList.h"
#include "Top_widget.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QStringEncoder>
#include <QDate>

#include <ctime>
#include <iostream>
#include <sstream>
#include <regex>

#include <chrono>
#include <thread>

TWSEDatabase::TWSEDatabase() {
}

bool TWSEDatabase::fetch(const std::string& stockID)
{
    stockID_ = stockID;
    clearAll();

    QDateTime today = QDateTime::currentDateTime();
    QString fromDate = today.addDays(-60).toString("yyyyMM");
    QString toDate = today.toString("yyyyMM");

    QDateTime startDate_ = QDateTime::fromString(fromDate, "yyyyMM");
    QDateTime endDate_ = QDateTime::fromString(toDate, "yyyyMM");

    if (!startDate_.isValid() || !endDate_.isValid()) {
        qCritical() << "Invalid date input";
    }

    for (QDateTime date = startDate_; date <= endDate_; date = date.addMonths(1)) {
        QString dateSpot = date.toString("yyyyMM");
        // qDebug() << dateSpot;
        fetchAndParseData(dateSpot);
    }

    if (spots_.size() == 2) {
        std::string err = "ERROR: " + stockID + " fetching data failed.";
        log(QString::fromStdString(err));
        return false;
    }
    else {
        // post-process
        return fillMissingDate();
    }
}

std::string TWSEDatabase::getStockID()
{
    return stockID_;
}


int TWSEDatabase::getTotalDays()
{
    return totalDays_;
}

std::vector<qreal> TWSEDatabase::getHigh()
{
    return high_;
}

std::vector<qreal> TWSEDatabase::getLow()
{
    return low_;
}

std::vector<qreal> TWSEDatabase::getClose()
{
    return close_;
}

std::vector<qreal> TWSEDatabase::closeFilled()
{
    return closeFilled_;
}

std::vector<qreal> TWSEDatabase::getOpen()
{
    return open_;
}

std::vector<int> TWSEDatabase::getVolume()
{
    return volume_;
}

std::vector<int> TWSEDatabase::getTransactions()
{
    return transactions_;
}

std::vector<QDateTime> TWSEDatabase::getTimestamps()
{
    return timeStamps_;
}

std::vector<QDateTime> TWSEDatabase::getTimestampsFilled()
{
    return timeStampsFilled_;
}

std::vector<qreal> TWSEDatabase::getTimestampsFilledQreal()
{
    return timeStampFilledQreal_;
}

std::vector<Spot> TWSEDatabase::getSpots()
{
    return spots_;
}

std::pair<QDateTime, qreal> TWSEDatabase::GetLatestClosePrice() {
    
    return { timeStampsFilled_.back(), closeFilled_.back() };
}

template<int window_size>
std::vector<qreal> TWSEDatabase::getMA() {
    std::vector<qreal> res_;
    qreal sum_ = 0;
    for (size_t i = 0; i < closeFilled_.size(); ++i) {
        sum_ += closeFilled_[i];
        if (i < window_size - 1)
            res_.push_back(0);
        else if (i == window_size - 1)
            res_.push_back(sum_ / window_size);
        else {
            sum_ -= closeFilled_[i - window_size];
            res_.push_back(sum_ / window_size);
        }
    }
    return res_;
}

std::vector<qreal> TWSEDatabase::getMA5()
{
    return getMA<5>();
}

std::vector<qreal> TWSEDatabase::getMA10()
{
    return getMA<10>();
}

void TWSEDatabase::fetchAndParseData(const QString& dateSpot) {

    std::string dateSpot_ = dateSpot.toStdString();
    const std::string apiUrl = "http://www.twse.com.tw/exchangeReport/STOCK_DAY?response=json&date=" + dateSpot_ + DUMMY + "&stockNo=" + stockID_;
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string response;

        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
            std::wstring wstrdata = utf8_conv.from_bytes(response);
            std::string utf8data = WStringToUTF8(wstrdata);

            parseData(utf8data);
        }
    }
}

bool TWSEDatabase::fillMissingDate()
{
    timeStampsFilled_.clear();
    closeFilled_.clear();
    timeStampFilledQreal_.clear();
    
    // stock fetching error
    if (timeStamps_.empty() || close_.empty()) {
        QString msg = "ERROR: " + QString::fromStdString(stockID_) + " fetching from TWSE error.";
        log(msg);
        return false;
    }

    timeStampsFilled_.push_back(timeStamps_[0]);
    closeFilled_.push_back(close_[0]);
    timeStampFilledQreal_.push_back(timeStamps_[0].toMSecsSinceEpoch());

    /*std::vector<QString> tmp;
    for (auto& i : timeStamps_) {
        tmp.push_back(i.toString("yyyy-MM-dd"));
    }*/
    

    for (int idx = 1; idx < timeStamps_.size();) {
        QDateTime nextDay = timeStampsFilled_.back().addDays(1);
        QDateTime nextDate = timeStamps_[idx];

        auto day = nextDay.toString("yyyy-MM-dd");
        auto date = nextDate.toString("yyyy-MM-dd");

        if (nextDay == nextDate) {
            timeStampsFilled_.push_back(nextDay);
            closeFilled_.push_back(close_[idx]);
            timeStampFilledQreal_.push_back(nextDay.toMSecsSinceEpoch());
            idx++;
        }
        else {
            timeStampsFilled_.push_back(nextDay);
            closeFilled_.push_back(closeFilled_.back());
            timeStampFilledQreal_.push_back(nextDay.toMSecsSinceEpoch());
        }

        // (WA) 11/13 duplicated date from TWSE  
        if (nextDay > timeStamps_.back()) {
            QString msg = "ERROR: " + QString::fromStdString(stockID_) + " date duplicated.";
            log(msg);
            return false;
        }
        // tmp.push_back(nextDay.toString("yyyy-MM-dd"));
    }



    if (timeStampsFilled_.empty() || closeFilled_.empty()) {
        /*auto aa = currtDate.toString("yyyy-MM-dd");
        auto bb = endDate.toString("yyyy-MM-dd");
        std::vector<QString> cc;
        for (auto i : timeStamps_) {
            cc.push_back(i.toString("yyyy-MM-dd"));
        }*/
        log("WARNING: Fetching error, retry " + QString::number(retryNum++));
        fetch(stockID_);
    }

    return true;
}

double TWSEDatabase::clearStringComma(const std::string& inStr)
{
    /*std::string res = inStr;
    for (auto itr = res.begin(); itr != res.end();) {
        if (*itr == ',') {
            res.erase(itr);
        }
        else {
            ++itr;
        }
    }
    return std::stod(res);*/

    std::string res = inStr;
    res.erase(std::remove(res.begin(), res.end(), ','), res.end());
    try {
        // Attempt to convert the cleaned string to a double
        return std::stod(res);
    }
    catch (const std::invalid_argument& e) {
        // Handle invalid input (e.g., non-numeric string)
        std::cerr << "Invalid input: " << e.what() << std::endl;
        log(QString::fromStdString("WARNING: Dirty data in stock " + stockID_));
    }
    catch (const std::out_of_range& e) {
        // Handle out-of-range values (e.g., too large or too small)
        std::cerr << "Out of range: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        // Handle other exceptions
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }

    return 0.0;
}

std::string TWSEDatabase::WStringToUTF8(const std::wstring& wstr) {
    int utf8str_len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8str(utf8str_len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8str[0], utf8str_len, nullptr, nullptr);
    return utf8str;
}

void TWSEDatabase::log(const QString& message)
{
    Logger::getInstance().log(message);
}

std::string TWSEDatabase::yearPreprocess(const std::string& dateStr)
{
    std::string result;
    size_t pos = dateStr.find('/');

    if (pos != std::string::npos) {
        std::string yearStr = dateStr.substr(0, pos);
        int yearInt = std::stoi(yearStr) + 1911;
        result = std::to_string(yearInt);
        result += dateStr.substr(pos);
    }
    return result;
}

QDateTime TWSEDatabase::toQDateTime(const std::string& dateStr)
{
    std::string tmp = yearPreprocess(dateStr);
    std::tm timeInfo = {}; // Initialize the struct to all zeros
    std::istringstream dateStream(tmp);

    dateStream >> std::get_time(&timeInfo, "%Y/%m/%d");

    if (dateStream.fail()) {
        std::cerr << "Date parsing failed for: " << tmp << std::endl;
        return QDateTime(); // Return an error value
    }

    // Convert the parsed date to a Unix timestamp
    time_t timestamp = std::mktime(&timeInfo);

    // Convert the parsed date to a Unix timestamp
    return QDateTime::fromSecsSinceEpoch(timestamp);
}

size_t TWSEDatabase::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* output = static_cast<std::string*>(userp);
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}


void TWSEDatabase::parseData(const std::string& rawData) {
    
    json data_ = json::parse(rawData);
    if (data_.size() == 2) return; // no data

    // std::string stat_ = data_["stat"].get<std::string>();
    // std::string title_ = data_["title"].get<std::string>();
    std::string date_ = data_["date"].get<std::string>().substr(0, 6);
    totalDays_ += data_["total"].get<int>();


    // Parse data
    const json& raw = data_["data"];
    for (const auto& entry : raw) {
        std::vector<std::string> rowData;

        if (entry.size() == 9) {
            QDateTime timestmp = toQDateTime(entry[0].get<std::string>());
            std::string hh = entry[4].get<std::string>();
            std::string ll = entry[5].get<std::string>();
            std::string cc = entry[6].get<std::string>();
            std::string oo = entry[3].get<std::string>();

            if (hh == "--" || ll == "--" || cc == "--" || oo == "--") continue;

            qreal h = clearStringComma(hh);
            qreal l = clearStringComma(ll);
            qreal c = clearStringComma(cc);
            qreal o = clearStringComma(oo);

            int v = clearStringComma(entry[1].get<std::string>());
            int tr = clearStringComma(entry[8].get<std::string>());

            // filter bad results

            Spot spot(timestmp, o, h, l, c, v);

            high_.push_back(h);
            low_.push_back(l);
            close_.push_back(c);
            open_.push_back(o);
            volume_.push_back(v);
            transactions_.push_back(tr);
            timeStamps_.push_back(timestmp);
            spots_.push_back(spot);
            
        }
        else {
            qDebug() << "WARNING: json parsing error.";
        }
    }
        
    
}

void TWSEDatabase::clearAll()
{
    high_.clear();
    low_.clear();
    close_.clear();
    open_.clear();
    volume_.clear();
    transactions_.clear();
    timeStamps_.clear();
    spots_.clear();

    high_.shrink_to_fit();
    low_.shrink_to_fit();
    close_.shrink_to_fit();
    open_.shrink_to_fit();
    volume_.shrink_to_fit();
    transactions_.shrink_to_fit();
    timeStamps_.shrink_to_fit();
    spots_.shrink_to_fit();
}

TWSEList::TWSEList()
{
    setBlackList();

    CURL* curl;
    std::string htmlContent;
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url_);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &htmlContent);

        // Perform the download operation
        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            // Now htmlContent contains the downloaded webpage
            // Step 1: Extract all <tr>...</tr> patterns
            matchTR(htmlContent);
        }
        else {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up and close libcurl
        curl_easy_cleanup(curl);
    }
}

std::map<std::string, std::pair<std::string, std::string>> TWSEList::getStockList()
{
    return stockDict_;
}

void TWSEList::setBlackList()
{
    // black list of stock (dirty data)
    std::vector<std::string> blackVector = {};
    blackList_.insert(blackVector.begin(), blackVector.end());
}

size_t TWSEList::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output)
{
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

void TWSEList::matchTR(const std::string& rawContent)
{
    std::regex trPattern("<tr>.*?</tr>");
    std::smatch trMatches;
    std::regex_iterator<std::string::const_iterator> trIt(rawContent.begin(), rawContent.end(), trPattern);
    std::regex_iterator<std::string::const_iterator> trEnd;

    int flagForDone = -1;

    while (trIt != trEnd && flagForDone < 1) {
        std::string trContent = (*trIt)[0].str();

        // Step 2: Find all <td>...</td> patterns within each <tr>...</tr>
        matchTD(trContent, flagForDone);
        ++trIt;
    }
}

void TWSEList::matchTD(const std::string& trContent, int& flagForDone)
{
    std::regex tdPattern("<td[^>]*>.*?</td>");
    std::smatch tdMatches;
    std::regex_iterator<std::string::const_iterator> tdIt(trContent.begin(), trContent.end(), tdPattern);
    std::regex_iterator<std::string::const_iterator> tdEnd;

    std::vector<std::string> tdContents; // Store all <td> contents


    while (tdIt != tdEnd) {
        std::string tdContent = (*tdIt)[0].str();
        tdContents.push_back(tdContent);
        ++tdIt;
    }

    if (tdContents.size() == 1) flagForDone++;

    else if (tdContents.size() >= 5) {
        // Access and print the first and fifth <td> tags
        matchTDItem(tdContents);
    }
}

void TWSEList::matchTDItem(const std::vector<std::string>& tdContents)
{
    std::string stockID = tdContents[0];
    std::string stockCate = tdContents[4];

    std::regex itemPattern(">([^<]+)<");
    std::smatch itemMatches;

    std::string num, name, cate;
    if (std::regex_search(stockID, itemMatches, itemPattern)) {
        if (itemMatches.size() == 2) {
            std::string extractedItem = itemMatches[1];
            int splitIdx = extractedItem.find('¡@');

            num = extractedItem.substr(0, splitIdx - 1);
            if (num.size() > 4) { num = num.substr(0, 4); } // for stock 4148 issue (stocklist format error)
            name = extractedItem.substr(splitIdx + 1);
        }
    }

    if (std::regex_search(stockCate, itemMatches, itemPattern)) {
        if (itemMatches.size() == 2) {
            cate = itemMatches[1];
        }
    }

    if (num.empty() || name.empty() || cate.empty()) std::cerr << "row parsing error." << std::endl;
    else {
        if (!blackList_.count(num)) stockDict_[num] = { name, cate };
    }
}

void TWSEList::log(const QString& message)
{
    Logger::getInstance().log(message);
}
