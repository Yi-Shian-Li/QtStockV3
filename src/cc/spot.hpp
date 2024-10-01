#ifndef SPOT_HPP
#define SPOT_HPP

#include <QtGlobal>
#include <QDateTime>
#include <ctime>
#include <string>

/**
 * @brief Spot class
 */
class Spot {

public:

    /**
     * @brief Spot constructor
     * @param date Spot date
     * @param open Price at opening
     * @param high Highest price value
     * @param low Lowest price value
     * @param close Price at closing
     */
    Spot(QDateTime date, qreal open, qreal high, qreal low, qreal close, int volume);


    /**
     * @brief Quote destructor
     */
    ~Spot();

    /**
     * @brief Date getter
     * @return Spot date
     */
    QDateTime getDate() const;

    /**
     * @brief Date getter
     * @return Spot date
     */
    QString getDateToString();

    /**
     * @brief Open price getter
     * @return Price at opening
     */
    qreal getOpen();

    /**
     * @brief High price getter
     * @return Higher price value
     */
    qreal getHigh();

    /**
     * @brief Low price getter
     * @return Lower price value
     */
    qreal getLow();

    /**
     * @brief Close price getter
     * @return Price at closing
     */
    qreal getClose();

    int getVolume();
 

    /**
     * @brief Print the spots
     */
    void printSpot();

private:

    /**
     * @brief Spot date in epoch format
     */
    QDateTime date;

    /**
     * @brief Price at opening
     */
    qreal open;

    /**
     * @brief Highest price value at this date
     */
    qreal high;

    /**
     * @brief Lowest price value at this date
     */
    qreal low;

    /**
     * @brief Price at closing
     */
    qreal close;

    // (2023-03-10 WMC added)
    int volume;

};

#endif /* SPOT_HPP */
