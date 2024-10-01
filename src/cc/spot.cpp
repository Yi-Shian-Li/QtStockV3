#include "spot.hpp"
#include "time_utils.hpp"

#include <iostream>
#include <sstream>


Spot::Spot(QDateTime date, qreal open, qreal high, qreal low, qreal close, int volume):
    date(date), open(open), high(high), low(low), close(close), volume(volume)
{
}

Spot::~Spot() {}

QDateTime Spot::getDate() const {
    return this->date;
}

QString Spot::getDateToString() {
    return (this->date).toString("yyyy-MM-dd");
}

qreal Spot::getOpen() {
    return this->open;
}

qreal Spot::getHigh() {
    return this->high;
}

qreal Spot::getLow() {
    return this->low;
}

qreal Spot::getClose() {
    return this->close;
}

int Spot::getVolume() {
    return this->volume;
}


void Spot::printSpot() {
    // std::cout << this->toString() << std::endl;
}
