#pragma once

#include <string>
#include <sstream>
#include <QDateTime>
#include <QString>
#include <QUrl>

inline std::string util_itos(int &i) {
	std::stringstream s;
	s << i;
	return s.str();
}

inline QString timestamp2DateStr(const qreal &t) {
	return QDateTime::fromMSecsSinceEpoch(t).toString("yyyy-MM-dd");
}

inline QDateTime timestamp2Date(const qreal& t) {
	return QDateTime::fromMSecsSinceEpoch(t);
}

inline QString decodeUrlEncodedString(const QString& encodedString) {
	QByteArray byteArray  = QByteArray::fromPercentEncoding(encodedString.toUtf8());
	QString decodedString = QString::fromUtf8(byteArray);
	return decodedString;
}