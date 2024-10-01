#include "QtStockV3.h"
#include "Top_widget.h"


QtStockV3::QtStockV3(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::QtStockV3Class())
{
    ui->setupUi(this);
    ui->centralWidget->setMouseTracking(true);
    ui->stackedWidget->setCurrentIndex(0);

    Logger& logger = Logger::getInstance();
    logger.log("LAUNCHING: QtStockAPIV3_2023.10.22");


    ui->iconOnlyMenu->setStyleSheet(
        "QWidget { "
        "background-color: white; "
        "width: 50px; "
        "} "
        "QPushButton, QLabel { "
        "height: 50px; "
        "border: none; "
        "border-radius: 5px; "
        "}"
        "QPushButton:hover { "
        "background-color: rgba(195, 199, 203, 0.5); "
        "}"
        "QPushButton:checked { "
        "background-color: rgba(195, 199, 203, 0.5); "
        "}"
    );
    /*ui->logoLabel->setStyleSheet(
        "QLabel { "
        "padding: 5px; "
        "}"
    );*/
    
    ui->fullMenu->setStyleSheet(
        "QWidget { "
        "background-color: white; "
        "} "
        "QPushButton { "
        "border: none; "
        "border-radius: 5px; "
        "text-align: left; "
        "padding: 0px 0px 0px 15px; "
        "color: black; "
        "}"
        "QPushButton:hover { "
        "background-color: rgba(195, 199, 203, 0.5); "
        "}"
        "QPushButton:checked { "
        "color: black; "
        "background-color: rgba(195, 199, 203, 0.5); "
        "}"
    );
    /*ui->logoLabel_2->setStyleSheet(
        "QLabel { "
        "padding: 5px; "
        "color: #fff; "
        "}"
    );*/
    
    /*ui->logoLabelText->setStyleSheet(
        "QLabel { "
        "padding-right: 10px; "
        "color: #fff; "
        "}"
    );*/
    ui->searchBox->setStyleSheet(
        "QLineEdit { "
        "border: none; "
        "padding: 5px 10px; "
        "margin-right: 5px; " // Add a margin to create space
        "}"
        "QLineEdit:focus { "
        "background-color: #70b9fe; "
        "}"
    );

    ui->fullMenu->setHidden(true);
    ui->splitter->setStretchFactor(0, 2);
    ui->splitter->setStretchFactor(1, 1);

    connect(ui->trendBtn_1, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(0);
        });
    connect(ui->rankBtn_1, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(1);
        });
    connect(ui->trendBtn_2, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(0);
        });
    connect(ui->rankBtn_2, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(1);
        });


    connect(&logger, &Logger::logMessage, this, &QtStockV3::handleLogMessage);

    //WStockDB* stockdb = new WStockDB();
    //stockdb->InitCrawler("2330.TW");
    //TWSEDatabase* twseData = new TWSEDatabase();
    
    // twseData->fetch("2330");


    WChartControlPanel* chartControl = new WChartControlPanel(ui->resetBtn, ui->handBtn, ui->ma5Btn, ui->ma10Btn, nullptr, nullptr);
    
    ui->trendChart->setController(chartControl);

    connect(chartControl->getBtn_1(), &QPushButton::clicked, ui->trendChart->getChart(), &QChart::zoomReset);
    connect(chartControl->getBtn_3(), &QPushButton::clicked, ui->trendChart, &WChartView::plotMA5);
    connect(chartControl->getBtn_4(), &QPushButton::clicked, ui->trendChart, &WChartView::plotMA10);
    connect(ui->trendTable, &RoundedWidget::stockSelected, ui->trendChart, &WChartView::updateChart);
    connect(ui->searchBox, &QLineEdit::textChanged, ui->trendTable, &RoundedWidget::filterResult);

    QAction* searchIcon = new QAction();

    QIcon icon_;
    icon_.addFile(QString::fromUtf8(":/QtStockV3/src/icon/search.png"), QSize(), QIcon::Normal, QIcon::Off);
    searchIcon->setIcon(icon_);
    ui->searchBox->addAction(searchIcon, QLineEdit::LeadingPosition);
    
    //------------------------------------------------------------------------------------------------

    WChartControlPanel* chartControl2 = new WChartControlPanel(ui->resetBtn_2, ui->handBtn_2, ui->ma5Btn_2, ui->ma10Btn_2, ui->shotBtn, ui->indicatorBtn);
    ui->trendChart_2->setController(chartControl2);


    //connect(ui->rankUpTable, &RoundedOrderWidget::stockSelected, ui->trendChart_2, &WChartView::updateChart);
    //connect(ui->rankDownTable, &RoundedOrderWidget::stockSelected, ui->trendChart_2, &WChartView::updateChart);
    
    connect(chartControl2->getBtn_1(), &QPushButton::clicked, ui->trendChart_2->getChart(), &QChart::zoomReset);
    connect(chartControl2->getBtn_3(), &QPushButton::clicked, ui->trendChart_2, &WChartView::plotMA5);
    connect(chartControl2->getBtn_4(), &QPushButton::clicked, ui->trendChart_2, &WChartView::plotMA10);
    connect(chartControl2->getShotBtn(), &QPushButton::clicked, ui->trendChart_2, &WChartView::screenShot);

    // analyzeBtn
    // analyzeStart* analyze = new analyzeStart(*ui->databaseTime);

    
    //connect(analyze, &analyzeStart::refreshTableRequested, ui->rankUpTable, &RoundedOrderWidget::refreshTable);
    //connect(analyze, &analyzeStart::refreshTableRequested, ui->rankDownTable, &RoundedOrderWidget::refreshTable);

    // Indicator plotting
    //connect(ui->rankUpTable, &RoundedOrderWidget::stockSelected, analyze, &analyzeStart::requestIndicatorFromDB);
    //connect(ui->rankDownTable, &RoundedOrderWidget::stockSelected, analyze, &analyzeStart::requestIndicatorFromDB);

    //ui->rankUpTable->setLinkedChartView(ui->trendChart_2);
    //ui->rankDownTable->setLinkedChartView(ui->trendChart_2);
    //connect(analyze, &analyzeStart::sendIndicatorToChart, ui->trendChart_2, &WChartView::updateIndicator);
    ui->orderWidget->setLinkedChartView(ui->trendChart_2);
    connect(chartControl2->getIndicatorBtn(), &QPushButton::clicked, ui->trendChart_2, &WChartView::plotIndicator);
    
}

QtStockV3::~QtStockV3()
{
    delete ui;
}

void QtStockV3::handleLogMessage(const QString& message) {
    ui->textBrowser->append(message);
}
