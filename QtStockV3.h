#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtStockV3.h"

QT_BEGIN_NAMESPACE
namespace Ui { class QtStockV3Class; };
QT_END_NAMESPACE

class QtStockV3 : public QMainWindow
{
    Q_OBJECT

public:
    QtStockV3(QWidget *parent = nullptr);
    ~QtStockV3();

public slots:
    void handleLogMessage(const QString& message);

private:
    Ui::QtStockV3Class *ui;
    std::unique_ptr<TWSEDatabase> twseData;

};
