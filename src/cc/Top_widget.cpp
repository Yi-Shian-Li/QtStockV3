#include "Top_widget.h"
#include "Top_utils.h"
#include "Top_strategy.h"
#include "Top_StockList.h"
#include "QtStockV3.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <QDate>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QFile>
#include <QFileInfo>
#include <QStandardItem>

#define ShadowWidth 5

QColor globalBGColor(255, 255, 255);
QColor globalBaseColor(18, 19, 20);


Crosshairs::Crosshairs() {
}

void Crosshairs::setChart(QChart* chart)
{
	m_xLine = new QGraphicsLineItem(chart);
	m_yLine = new QGraphicsLineItem(chart);
	//m_xText(new QGraphicsTextItem(chart))
	m_yText = new QGraphicsTextItem(chart);
	m_chart = chart;

	m_xLine->setPen(QPen(Qt::gray, 2, Qt::DotLine));
	m_yLine->setPen(QPen(Qt::gray, 2, Qt::DotLine));
	//m_xText->setZValue(11);
	m_yText->setZValue(11);
	//m_xText->document()->setDocumentMargin(0);
	m_yText->document()->setDocumentMargin(0);
	//m_xText->setDefaultTextColor(Qt::blue);
	m_yText->setDefaultTextColor(Qt::white);
}

void Crosshairs::updatePosition(QPointF position)
{
	QLineF xLine(position.x(), m_chart->plotArea().top(),
		position.x(), m_chart->plotArea().bottom());
	QLineF yLine(m_chart->plotArea().left(), position.y(),
		m_chart->plotArea().right(), position.y());
	m_xLine->setLine(xLine);
	m_yLine->setLine(yLine);

	//QString xText = QString("%1").arg(m_chart->mapToValue(position).x());
	QString yText = QString::number(m_chart->mapToValue(position).y(), 'f', 1);
	//m_xText->setHtml(QString("<div style='background-color: #ff0000;'>") + xText + "</div>");
	m_yText->setHtml(QString("<div style='background-color: #606060;'>") + yText + "</div>");
	//m_xText->setPos(position.x() - m_xText->boundingRect().width() / 2.0, m_chart->plotArea().bottom());
	m_yText->setPos(m_chart->plotArea().left() - m_yText->boundingRect().width()-5, position.y() - m_yText->boundingRect().height() / 2.0);

	if (!m_chart->plotArea().contains(position))
	{
		m_xLine->hide();
		// m_xText->hide();
		m_yLine->hide();
		m_yText->hide();
	}
	else
	{
		m_xLine->show();
		// m_xText->show();
		m_yLine->show();
		m_yText->show();
	}
}



RoundedWidget::RoundedWidget(QWidget* parent) : QWidget(parent)
{
	layout_ = std::make_unique<QVBoxLayout>(this);
	toolbar_ = std::make_unique<QHBoxLayout>();
	header_ = std::make_unique<QLabel>(this);
	latestUpdateTime_ = std::make_unique<QLabel>(this);

	model_ = std::make_unique<QStandardItemModel>();
	proxyModel_ = std::make_unique<QSortFilterProxyModel>(this);
	table_ = std::make_unique<QTableView>(this);

	setAutoFillBackground(false);
	setContentsMargins(ShadowWidth, ShadowWidth, ShadowWidth, ShadowWidth);

	configToolbar();
	configBorderEffect();
	configLayout();
	configTable();
	init();

	connect(table_.get(), &QTableView::clicked, this, &RoundedWidget::cellClicked);
	
}

void RoundedWidget::init()
{
	configModel();
}

void RoundedWidget::filterResult(const QString& msg)
{
	proxyModel_->setFilterFixedString(msg);
}

void RoundedWidget::configToolbar()
{
	header_->setFont(QFont("Microsoft JhengHei", -1, QFont::Bold, false));
	header_->setText("TWSE Stock List");

	auto spacer = new QWidget(this);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	toolbar_->addWidget(header_.get());
	toolbar_->addWidget(spacer);


	//QIcon icon11;
	//icon11.addFile(QString::fromUtf8("src/icon/ma5.png"), QSize(), QIcon::Normal, QIcon::Off);
	//strategyStart_->setIcon(icon11);

}

void RoundedWidget::configBorderEffect()
{
	auto borderEffect = new QGraphicsDropShadowEffect;
	borderEffect->setBlurRadius(ShadowWidth);
	borderEffect->setColor(QColor(125, 125, 125, 150));
	borderEffect->setOffset(0, 0);
	this->setGraphicsEffect(borderEffect);
}

void RoundedWidget::configLayout()
{
	layout_->addLayout(toolbar_.get());

	auto separator = new QFrame;
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);
	layout_->addWidget(separator);
	this->setLayout(layout_.get());
}

void RoundedWidget::configTable()
{
	table_->setMouseTracking(true);
	table_->setSortingEnabled(true);
	table_->setSelectionMode(QAbstractItemView::SingleSelection);

	table_->setAlternatingRowColors(true);
	table_->setFocusPolicy(Qt::NoFocus);
	table_->setSelectionBehavior(QAbstractItemView::SelectRows);
	table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table_->setFrameStyle(QFrame::NoFrame);
	table_->horizontalHeader()->setStretchLastSection(true);
	table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table_->setShowGrid(false);
	table_->verticalHeader()->hide();
	table_->setStyleSheet(
		"QTableView::item:selected{background-color: #1B89A1}"
	);

	QPalette p = table_->palette();
	p.setColor(QPalette::Base, QColor(255, 255, 255));
	p.setColor(QPalette::AlternateBase, QColor(224, 224, 224));
	table_->setPalette(p);

	QString horizontalHeaderStyle = "QHeaderView::section { \
										  background-color: rgb(204, 229, 255); /* header background color */ \
                                          border-left: 0.5px solid #b1b1b5; \
                                          border-top: 1px solid #b1b1b5; \
                                          border-right: 0.5px solid #b1b1b5; \
                                          border-bottom: 1px solid #b1b1b5; \
                                      }";

	table_->horizontalHeader()->setStyleSheet(horizontalHeaderStyle);

	// table content font size
	// QString fontFile = "src/TaipeiSansTCBeta-Regular.ttf";
	// int fontId = QFontDatabase::addApplicationFont(fontFile);
	// QString fontName = QFontDatabase::applicationFontFamilies(fontId).at(0);

	QFont font("Microsoft JhengHei", -1, QFont::Bold, false);
	font.setPointSize(10);
	table_->setFont(font);

	layout_->addWidget(table_.get());
}

void RoundedWidget::configModel()
{
	TWSEList* stockListPtr = new TWSEList();
	auto stockList = stockListPtr->getStockList();

	// column settings
	QStandardItem* IDItem = new QStandardItem("ID");
	QStandardItem* NameItem = new QStandardItem("Name");
	QStandardItem* CateItem = new QStandardItem("Category");
	model_->setHorizontalHeaderItem(0, IDItem);
	model_->setHorizontalHeaderItem(1, NameItem);
	model_->setHorizontalHeaderItem(2, CateItem);
	model_->setRowCount(stockList.size());

	int rowCount = 0;
	for (auto listPtr : stockList) {
		QString id = QString::fromStdString(listPtr.first);
		QString name = QString::fromLocal8Bit(listPtr.second.first);
		QString cate = QString::fromLocal8Bit(listPtr.second.second);

		QStandardItem* item0 = new QStandardItem(id);
		QStandardItem* item1 = new QStandardItem(name);
		QStandardItem* item2 = new QStandardItem(cate);

		item0->setData(Qt::ForegroundRole);
		item0->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);

		item1->setData(Qt::ForegroundRole);
		item1->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);

		item2->setData(Qt::ForegroundRole);
		item2->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);

		model_->setItem(rowCount, 0, item0);
		model_->setItem(rowCount, 1, item1);
		model_->setItem(rowCount, 2, item2);

		rowCount++;
	}

	proxyModel_->setFilterCaseSensitivity(Qt::CaseInsensitive);
	proxyModel_->setSourceModel(model_.get());
	table_->setModel(proxyModel_.get());
}

void RoundedWidget::cellClicked(const QModelIndex& proxyIndex) {

	// Map the proxy index to the source index
	QModelIndex sourceIndex = proxyModel_->mapToSource(proxyIndex);

	int row = sourceIndex.row();
	int column = sourceIndex.column();

	if (lastClicked_ == row) return;
	QStandardItem* id    = model_->item(row, 0);
	QStandardItem* name  = model_->item(row, 1);
	QStandardItem* categ = model_->item(row, 2);
	lastClicked_ = row;

	StockInfo stockInfo{ id->text(), name->text(), categ->text() };

	emit stockSelected(stockInfo);

}

void RoundedWidget::paintEvent(QPaintEvent* ev) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	const QPalette& palette = this->palette();
	QColor backgroundColor = palette.color(QPalette::Window);
	QPainterPath path;
	path.addRoundedRect(contentsRect(), 10, 10);
	painter.fillPath(path, backgroundColor);
}


WChartView::WChartView(QWidget* parent): QChartView(parent)
{
	chart_  = std::make_unique<QChart>();
	setChart(chart_.get());
	chart_->setTitle("Choose a stock from the list.");

	series_ = std::make_unique<QCandlestickSeries>();

	axisX_ = std::make_unique<QDateTimeAxis>();
	axisY_ = std::make_unique<QValueAxis>();

	areaSeries_ = std::make_unique<QAreaSeries>();
	lineSeries_ = std::make_unique<QLineSeries>();

	axisX_Volume_ = std::make_unique<QDateTimeAxis>();
	axisY_Volume_ = std::make_unique<QValueAxis>();

	stockdb_ = std::make_unique<TWSEDatabase>();

	this->setParent(parent);

	m_crossHair_ = new Crosshairs();
	m_crossHair_->setChart(chart_.get());

	connect(series_.get(), &QCandlestickSeries::hovered, this, &WChartView::onCandlestickHovered);
	
}

QDateTimeAxis* WChartView::getXAxis()
{
	return axisX_.get();
}

QValueAxis* WChartView::getYAxis()
{
	return axisY_.get();
}

QValueAxis* WChartView::getYAxisVolume()
{
	return axisY_Volume_.get();
}

QChart* WChartView::getChart()
{
	return chart_.get();
}

void WChartView::updateChart(StockInfo& stockInfo) {

	stockdb_->fetch(stockInfo.ID.toStdString());
	userChoosedStock_ = 1;

	this->updateCandles();
	this->updateMA();

	auto axisX = this->getXAxis();
	auto axisY = this->getYAxis();
	auto axisY_Volume = this->getYAxisVolume();

	// add the X-axis to the chart
	chart_->addAxis(axisX, Qt::AlignBottom);
	chart_->addAxis(axisY, Qt::AlignLeft);
	chart_->addAxis(axisY_Volume, Qt::AlignRight);

	// reflash chart title
	QString title_ = stockInfo.Name + " (" + stockInfo.ID + ")";
	chart_->setTitle(title_);
}

void WChartView::colorSwitch()
{
	if (isNormal) {
		isNormal = false;
		increasingColor = QColor::fromRgb(180, 90, 90);
		decreasingColor = QColor::fromRgb(100, 180, 110);
	}
	else {
		isNormal = true;
		increasingColor = QColor::fromRgb(100, 180, 110);
		decreasingColor = QColor::fromRgb(180, 90, 90);
	}
	series_->setIncreasingColor(increasingColor);
	series_->setDecreasingColor(decreasingColor);

	QList<QCandlestickSet*> setList = series_->sets();
	for (int i = 0; i < series_->count(); ++i) {
		QPen pen = setList[i]->pen();
		pen.setStyle(Qt::SolidLine);
		if (setList[i]->close() >= setList[i]->open()) {
			pen.setColor(increasingColor);
		}
		else {
			pen.setColor(decreasingColor);
		}
		setList[i]->setPen(pen);
	}
}

void WChartView::plotMA5()
{
	if (ma5_p_) {
		chart_->removeSeries(ma5_p_.get());
		ma5_p_ = nullptr;
	}
	else {
		initializeMA5Ptr();

		if (ma5_.empty()) ma5_ = stockdb_->getMA5();
		if (timeStampFilledQreal_.empty()) timeStampFilledQreal_ = stockdb_->getTimestampsFilledQreal();

		for (int i = 4; i < ma5_.size(); ++i) {
			ma5_p_->append(timeStampFilledQreal_[i], ma5_[i]);
		}

		chart_->addSeries(ma5_p_.get());
		ma5_p_->attachAxis(axisX_.get());
		ma5_p_->attachAxis(axisY_.get());

	}
}

void WChartView::plotMA10()
{
	if (ma10_p_) {
		chart_->removeSeries(ma10_p_.get());
		ma10_p_ = nullptr;
	}
	else {
		initializeMA10Ptr();

		if (ma10_.empty()) ma10_ = stockdb_->getMA10();
		if (timeStampFilledQreal_.empty()) timeStampFilledQreal_ = stockdb_->getTimestampsFilledQreal();

		for (int i = 9; i < ma10_.size(); ++i) {
			ma10_p_->append(timeStampFilledQreal_[i], ma10_[i]);
		}

		chart_->addSeries(ma10_p_.get());
		ma10_p_->attachAxis(axisX_.get());
		ma10_p_->attachAxis(axisY_.get());
		
	}
}

void WChartView::screenShot()
{
	QDateTime today = QDateTime::currentDateTime();
	QString fname = today.toString("yyyy-MM-dd-hh-mm-ss");
	QString filePath = "./src/" + fname + ".png";

	QPixmap p = this->grab();
	p.save(filePath, "PNG");

	QString message = "WARNING: " + filePath + " is saved.";
	log(message.toStdString());
}

void WChartView::updateIndicator(QVector<QPair<QString, qreal>>& orderList, QVector<QPair<QString, qreal>>& crossList)
{
	orderList_ = orderList;
	crossList_ = crossList;

	if (orderScatters_) {
		chart_->removeSeries(orderScatters_.get());
		initializeOrderScattersPtr();

		if (orderList_.empty()) return;
		for (QPair<QString, qreal>& pts : orderList_) {
			auto xAxis = QDateTime::fromString(pts.first, "yyyy-MM-dd").toMSecsSinceEpoch();
			orderScatters_->append(xAxis, pts.second);
		}

		chart_->addSeries(orderScatters_.get());
		orderScatters_->attachAxis(axisX_.get());
		orderScatters_->attachAxis(axisY_.get());
	}

	if (crossScatters_) {
		chart_->removeSeries(crossScatters_.get());
		initializeCrossScattersPtr();

		if (crossList_.empty()) return;
		for (QPair<QString, qreal>& pts : crossList_) {
			auto xAxis = QDateTime::fromString(pts.first, "yyyy-MM-dd").toMSecsSinceEpoch();
			crossScatters_->append(xAxis, pts.second);
		}

		chart_->addSeries(crossScatters_.get());
		crossScatters_->attachAxis(axisX_.get());
		crossScatters_->attachAxis(axisY_.get());
	}
}

void WChartView::plotIndicator()
{
	if (orderScatters_) {
		chart_->removeSeries(orderScatters_.get());
		orderScatters_.reset();
	}
	else {
		initializeOrderScattersPtr();

		if (orderList_.empty()) return;
		for (QPair<QString, qreal>& pts : orderList_) {
			auto xAxis = QDateTime::fromString(pts.first, "yyyy-MM-dd").toMSecsSinceEpoch();
			orderScatters_->append(xAxis, pts.second);
		}
		
		chart_->addSeries(orderScatters_.get());
		orderScatters_->attachAxis(axisX_.get());
		orderScatters_->attachAxis(axisY_.get());
		
	}

	if (crossScatters_) {
		chart_->removeSeries(crossScatters_.get());
		crossScatters_.reset();
	}
	else {
		initializeCrossScattersPtr();

		if (crossList_.empty()) return;
		for (QPair<QString, qreal>& pts : crossList_) {
			auto xAxis = QDateTime::fromString(pts.first, "yyyy-MM-dd").toMSecsSinceEpoch();
			crossScatters_->append(xAxis, pts.second);
		}

		chart_->addSeries(crossScatters_.get());
		crossScatters_->attachAxis(axisX_.get());
		crossScatters_->attachAxis(axisY_.get());
	}
}

bool WChartView::isNormal = true;

void WChartView::setController(WChartControlPanel* chartControl)
{
	chartControl_ = chartControl;
}


void WChartView::log(const std::string& message)
{
	Logger::getInstance().log(QString::fromStdString(message));
}

void WChartView::setChartData()
{
	qreal candlestickWidth = 0.7;
	series_->attachAxis(axisX_.get());
	series_->attachAxis(axisY_.get());
	series_->setBodyWidth(candlestickWidth * 0.8); // Adjust for spacing
	series_->setMinimumColumnWidth(candlestickWidth * 0.2); // Adjust for spacing
	series_->setName("candles");
}

void WChartView::setChartAxis()
{
	axisX_->setFormat("yyyy-MM-dd");
	axisX_->setTickCount(10);
	axisX_->setLabelsAngle(-45);

	// hide the X-axis
	axisX_->setVisible(true);

	chart_->addAxis(axisY_.get(), Qt::AlignLeft);
	chart_->addAxis(axisX_.get(), Qt::AlignBottom);
	chart_->addAxis(axisY_Volume_.get(), Qt::AlignRight);

	chart_->addSeries(series_.get());

	chart_->legend()->setVisible(true); // turn off legend
}

void WChartView::setSubChart()
{
	lineSeries_->attachAxis(axisY_Volume_.get());
	areaSeries_->setUpperSeries(lineSeries_.get());
	areaSeries_->setName("Volume");
	
	chart_->addSeries(areaSeries_.get());
	areaSeries_->attachAxis(axisY_Volume_.get());
}


void WChartView::updateCandlestickSeries()
{
	// close_.clear();
	// date_.clear();
	series_->clear();
	lineSeries_->clear();
	

	minValue_ = std::numeric_limits<quint64>::max();
	maxValue_ = std::numeric_limits<quint64>::min();

	minVolumeValue_ = std::numeric_limits<quint64>::max();
	maxVolumeValue_ = std::numeric_limits<quint64>::min();

	if (isNormal) {
		increasingColor = QColor::fromRgb(180, 90, 90);
		decreasingColor = QColor::fromRgb(100, 180, 110);
	}
	else {
		increasingColor = QColor::fromRgb(100, 180, 110);
		decreasingColor = QColor::fromRgb(180, 90, 90);
	}

	
	spots_ = stockdb_->getSpots();

	for (Spot i : spots_) {
		qreal open = i.getOpen();
		qreal high = i.getHigh();
		qreal low = i.getLow();
		qreal close = i.getClose();
		qreal volume = i.getVolume()/10000;
		QDateTime dateTime = i.getDate();
		QCandlestickSet* candlestickSet = new QCandlestickSet(open, high, low, close, dateTime.toMSecsSinceEpoch());

		QPen pen = candlestickSet->pen();
		pen.setStyle(Qt::SolidLine);
		if (close >= open) {
			pen.setColor(increasingColor);
		}
		else
			pen.setColor(decreasingColor);

		candlestickSet->setPen(pen);
		series_->append(candlestickSet);

		minValue_ = qMin(minValue_, low);
		maxValue_ = qMax(maxValue_, high);

		// for subplot
		minVolumeValue_ = qMin(minVolumeValue_, volume);
		maxVolumeValue_ = qMax(maxVolumeValue_, volume);

		// Add data to the line series
		lineSeries_->append(dateTime.toMSecsSinceEpoch(), volume);

		// collect close price to calc MA
		// close_.push_back(close);
		// date_.push_back(dateTime.toMSecsSinceEpoch());
	}

	series_->setIncreasingColor(increasingColor);
	series_->setDecreasingColor(decreasingColor);

	// set the range of the Y-axis to show the data
	// adjust the scale of the Y-axis to a nice round number
	// label
	axisY_->setRange(minValue_, maxValue_);
	axisY_->applyNiceNumbers();
	axisY_->setTitleText("Price");

	axisY_Volume_->setRange(minVolumeValue_, maxVolumeValue_ * 3);
	axisY_Volume_->applyNiceNumbers();
	axisY_Volume_->setTitleText("Volume (W)");

	stockdb_->fillMissingDate();
}

void WChartView::setChartStyle()
{
	chart_->setTheme(QChart::ChartThemeLight);
	chart_->setAnimationOptions(QChart::SeriesAnimations);
	chart_->setBackgroundBrush(QBrush(QColor(255, 255, 255)));
	this->setStyleSheet("background-color: rgb(236, 236, 234);");

	QBrush backgroundColorBrush(globalBaseColor);
	// externalPtr_->setBackgroundBrush(backgroundColorBrush);

	QBrush brush(QColor(0, 128, 255, 75)); // red color with 50% transparency
	areaSeries_->setBrush(brush);
	areaSeries_->setPen(QPen(Qt::transparent));

}

void WChartView::setChartTtile()
{
	// QString fontFile = "src/TaipeiSansTCBeta-Regular.ttf";
	// int fontId = QFontDatabase::addApplicationFont(fontFile);
	// QString fontName = QFontDatabase::applicationFontFamilies(fontId).at(0);

	QFont font("Microsoft JhengHei", -1, QFont::Bold, false);
	font.setPointSize(20);
	chart_->setTitle("2330.TW");
	chart_->setTitleFont(font);
}

void WChartView::initializeMA5Ptr()
{
	ma5_p_ = std::make_unique<QLineSeries>();
	ma5_p_->setPen(QPen(QColor::fromRgb(235, 149, 17), 1, Qt::SolidLine));  // set line color and width to red and 2 pixels, respectively, and style to solid line
	ma5_p_->setPointsVisible(false);  // show data points on the line
	ma5_p_->setName("MA5");
}

void WChartView::initializeMA10Ptr()
{
	ma10_p_ = std::make_unique<QLineSeries>();
	ma10_p_->setPen(QPen(QColor::fromRgb(255, 0, 255), 1, Qt::SolidLine));  // set line color and width to red and 2 pixels, respectively, and style to solid line
	ma10_p_->setPointsVisible(false);  // show data points on the line
	ma10_p_->setName("MA10");
}

void WChartView::initializeOrderScattersPtr()
{
	orderScatters_ = std::make_unique<QScatterSeries>();
	orderScatters_->setName("Order Indicators");
	orderScatters_->setMarkerShape(QScatterSeries::MarkerShapeStar);
	orderScatters_->setMarkerSize(25);
	orderScatters_->setColor(QColor(255, 51, 153));
}

void WChartView::initializeCrossScattersPtr()
{
	crossScatters_ = std::make_unique<QScatterSeries>();
	crossScatters_->setName("Servant Indicators");
	crossScatters_->setMarkerShape(QScatterSeries::MarkerShapeCircle);
	crossScatters_->setMarkerSize(10);
	crossScatters_->setColor(QColor(255, 153, 51));
}

void WChartView::mouseMoveEvent(QMouseEvent* event)
{
	QChartView::mouseMoveEvent(event); // Call the base class implementation
	//qDebug() << event->pos();
	/*if (m_crossHair_)
	{
		m_crossHair_->updatePosition(event->pos());
	}*/
	if (userChoosedStock_)
		m_crossHair_->updatePosition(event->pos());
}

void WChartView::enterEvent(QEnterEvent* event)
{
	QChartView::enterEvent(event); // Call the base class implementation
	// qDebug() << "mouse move in.";
	// Logger::getInstance().log("WARNING: mouse move in.");
}

void WChartView::leaveEvent(QEvent* event)
{
	QChartView::leaveEvent(event); // Call the base class implementation
	// Logger::getInstance().log("WARNING: mouse move out.");
}


void WChartView::onCandlestickHovered(bool state, QCandlestickSet* set)
{
	QList<QCandlestickSet*> tmp_series_ = series_->sets();

	if (state) { // If mouse is hovering over a data point
		// Find the candlestick that the mouse is hovering over
		for (auto it = tmp_series_.begin(); it != tmp_series_.end(); ++it) {
			QCandlestickSet* candlestickSet = *it;
			if (candlestickSet->timestamp() == set->timestamp()) {
				QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(candlestickSet->timestamp());
				// Set the tooltip text to the x and y values of the hovered-over candlestick
				QString tooltipText = QString("Date: %1\nOpen: %2\nHigh: %3\nLow: %4\nClose: %5")
					.arg(dateTime.toString("yyyy-MM-dd"))
					.arg(candlestickSet->open())
					.arg(candlestickSet->high())
					.arg(candlestickSet->low())
					.arg(candlestickSet->close());
				chart_->setToolTip(tooltipText);
				return;
			}
		}
	}
	else {
		QToolTip::hideText();
	}
	// If mouse is not hovering over a data point, clear the tooltip text
	chart_->setToolTip("");
}

void WChartView::updateCandles()
{
	chart_->zoomReset();
	updateCandlestickSeries();

	setChartAxis();
	setChartData();
	setSubChart();
	setChartStyle();
	setChartTtile();
	setMouseTracking(true);

	setRubberBand(QChartView::RectangleRubberBand); // Enable horizontal panning
	setInteractive(true); // Enable chart interactions, including panning
	setRenderHint(QPainter::Antialiasing);
	setChart(chart_.get());
}

void WChartView::updateMA()
{
	ma5_.clear();
	ma10_.clear();
	timeStampFilledQreal_.clear();

	ma5_ = stockdb_->getMA5();
	ma10_ = stockdb_->getMA10();
	timeStampFilledQreal_ = stockdb_->getTimestampsFilledQreal();

	if (ma5_p_) {
		chart_->removeSeries(ma5_p_.get());
		ma5_p_->clear();
		for (int i = 4; i < ma5_.size(); ++i)
			ma5_p_->append(timeStampFilledQreal_[i], ma5_[i]);

		chart_->addSeries(ma5_p_.get());
		ma5_p_->attachAxis(axisX_.get());
		ma5_p_->attachAxis(axisY_.get());
	}

	if (ma10_p_) {
		chart_->removeSeries(ma10_p_.get());
		ma10_p_->clear();
		for (int i = 9; i < ma10_.size(); ++i)
			ma10_p_->append(timeStampFilledQreal_[i], ma10_[i]);

		chart_->addSeries(ma10_p_.get());
		ma10_p_->attachAxis(axisX_.get());
		ma10_p_->attachAxis(axisY_.get());
	}
}

WChartControlPanel::WChartControlPanel(QPushButton* rstBtn, QPushButton* handBtn, QPushButton* ma5Btn, QPushButton* ma10Btn, QPushButton* screenShot, QPushButton* indicatorBtn):
	rstBtn_(rstBtn), handBtn_(handBtn), ma5Btn_(ma5Btn), ma10Btn_(ma10Btn), screenShot_(screenShot), indicatorBtn_(indicatorBtn)
{
}

QPushButton* WChartControlPanel::getBtn_1()
{
	return rstBtn_.get();
}

QPushButton* WChartControlPanel::getBtn_2()
{
	return handBtn_.get();
}

QPushButton* WChartControlPanel::getBtn_3()
{
	return ma5Btn_.get();
}

QPushButton* WChartControlPanel::getBtn_4()
{
	return ma10Btn_.get();
}

QPushButton* WChartControlPanel::getShotBtn()
{
	return screenShot_.get();
}

QPushButton* WChartControlPanel::getIndicatorBtn()
{
	return indicatorBtn_.get();
}

Logger::Logger() {
}

Logger& Logger::getInstance()
{
	static Logger instance;
	return instance;
}

void Logger::log(const QString& message)
{
	emit logMessage(message);
}

/*WOrderTableWidget::WOrderTableWidget(QTableWidget* tableWidget, const QString& dbName, const QString& sortOrder):
	tableWidget_(tableWidget), dbName_(dbName), sortOrder_(sortOrder)
{
	QFileInfo f(dbName_);
	if (!f.exists() || !f.isFile()) {
		log("ERROR: No order database. Please run the strategy first.");
	}
	else {
		connectToDatabase(dbName_);
	}

	tableWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);
	tableWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
	tableWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tableWidget_->setSortingEnabled(true);
	tableWidget_->setMouseTracking(true);

	connect(tableWidget_, &QTableWidget::cellClicked, this, &WOrderTableWidget::cellClicked);
}

void WOrderTableWidget::refreshTable()
{
	log("NOTICE: auto refresh order tables.");
	tableWidget_->clearContents();
	tableWidget_->setRowCount(0);
	connectToDatabase(dbName_);
}

void WOrderTableWidget::cellClicked(int row, int column) {
	if (lastClicked_ == row) return;
	QTableWidgetItem* id = tableWidget_->item(row, 0);
	QTableWidgetItem* name = tableWidget_->item(row, 1);
	QTableWidgetItem* categ = tableWidget_->item(row, 2);
	lastClicked_ = row;

	//qDebug() << row;

	StockInfo stockInfo{ id->text(), name->text(), categ->text() };

	emit stockSelected(stockInfo);

}

void WOrderTableWidget::connectToDatabase(const QString& dbName)
{
	setTableStyle();

	QSqlDatabase readFromDB;
	if (QSqlDatabase::contains("readFromDB")) {
		readFromDB = QSqlDatabase::database("readFromDB");
	}
	else {
		readFromDB = QSqlDatabase::addDatabase("QSQLITE", "readFromDB");
	}
	readFromDB.setDatabaseName("order.db");

	if (readFromDB.open()) {
		QSqlQuery query(readFromDB);
		sqlToDefaultMode(query);

		TWSEList* twseList = new TWSEList();
		auto list_ = twseList->getStockList();

		QString sqlQuery = "SELECT stockID, date, MAX(ROI) AS ROI, price "
			"FROM orderList "
			"GROUP BY stockID "
			"ORDER BY ROI " + sortOrder_ + " "
			"LIMIT 30";

		if (query.exec(sqlQuery)) {

			int columnCount = query.record().count();
			tableWidget_->setColumnCount(columnCount);

			// Populate the header labels of the table widget with the column names
			for (int i = 0; i < columnCount; i++) {
				QTableWidgetItem* item = new QTableWidgetItem(query.record().fieldName(i));
				tableWidget_->setHorizontalHeaderItem(i, item);
			}
			tableWidget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
			tableWidget_->horizontalHeader()->setStretchLastSection(false);

			int rowCount = 0;
			while (query.next()) {
				int stockId = query.value("stockID").toInt();
				double roi = query.value("ROI").toDouble();
				// qDebug() << "Stock ID:" << stockId << "ROI:" << roi;

				tableWidget_->setRowCount(rowCount + 1);
				for (int i = 0; i < columnCount; i++) {
					auto itemvalue = query.value(i);
					QTableWidgetItem* item = new QTableWidgetItem(itemvalue.toString());
					switch (i) {
					case 2: // ROI column
						if (itemvalue.toDouble() > 0) {
							auto t = itemvalue.toDouble();
							item->setData(Qt::ForegroundRole, QColor(Qt::red));
						}
						else {
							item->setData(Qt::ForegroundRole, QColor(Qt::green));
						}
						break;
					case 3: // price
						item->setText(QString::number(itemvalue.toDouble(), 'f', 2));
						//item->setData(Qt::ForegroundRole, QColor(Qt::black));
						break;
					default:
						//item->setData(Qt::ForegroundRole, QColor(Qt::black));
						break;
					}
					item->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
					tableWidget_->setItem(rowCount, i, item);

				}
				rowCount++;
			}

			// add name column
			tableWidget_->insertColumn(1);
			tableWidget_->setHorizontalHeaderItem(1, new QTableWidgetItem("Name"));
			for (int row = 0; row < tableWidget_->rowCount(); row++) {
				// Get the item from the previous column in the same row
				QString stockID = tableWidget_->item(row, 0)->text(); // Assuming 0 is the index of the previous column
				QString stockName = QString::fromLocal8Bit(list_[stockID.toStdString()].first);
				QTableWidgetItem* newItem = new QTableWidgetItem(stockName);
				newItem->setData(Qt::ForegroundRole, QColor(Qt::black));
				newItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);


				tableWidget_->setItem(row, 1, newItem);
			}

			log("NOTICE: Retrieve orders from database success.");
		}

	}
	else {
		QString message = "ERROR: Failed to open the database. " + readFromDB.lastError().text();
		log(message.toStdString());
	}
	
	

}

void WOrderTableWidget::log(const std::string& message)
{
	Logger::getInstance().log(QString::fromStdString(message));
}

void WOrderTableWidget::sqlToDefaultMode(QSqlQuery& query)
{
	query.exec("PRAGMA journal_mode=DELETE;");
	if (query.next()) {
		QString mode = query.value(0).toString();
		QString msg = "Current SQLite mode is: Default";
		log(msg.toStdString());
	}
}

void WOrderTableWidget::setTableStyle()
{
	QString style = "QTableView { \
						gridline-color: rgb(255, 255, 255); \
						background: white; \
					  } \
					 QTableWidget::item { \
						background: white; \
					 } \
					 QTableView::item:selected { \
						background: rgba(192, 192, 192, 0.5); \
						color: rgb(0, 0, 0); \
						background-repeat: no - repeat; \
						background-position: center right; \
					  } \
					QScrollBar::handle:vertical {\
						background-color: rgb(50, 53, 49);\
						border-radius: 5px;\
						min-height: 20px;\
					}\
					QScrollBar::add-line:vertical,\
					QScrollBar::sub-line:vertical {\
						border: none;\
						background: none;\
					}\
					QScrollBar::add-page:vertical,\
					QScrollBar::sub-page:vertical {\
						background: none;\
					}";

	// style = style.arg(globalBGColor.red()).arg(globalBGColor.green()).arg(globalBGColor.blue()).arg(globalBaseColor.red()).arg(globalBaseColor.green()).arg(globalBaseColor.blue());
	tableWidget_->setStyleSheet(style);
	tableWidget_->verticalHeader()->setVisible(false);
	tableWidget_->setFocusPolicy(Qt::NoFocus);

	QString horizontalHeaderStyle = "QHeaderView::section { \
										  background-color: rgb(224, 224, 224);  \
										  color: black;  \
										  padding: 6px;  \
										  border: none;  \
										  font-weight: bold;  \
                                      }";

	tableWidget_->horizontalHeader()->setStyleSheet(horizontalHeaderStyle);

	// table content font size
	QString fontFile = "src/TaipeiSansTCBeta-Regular.ttf";
	int fontId = QFontDatabase::addApplicationFont(fontFile);
	QString fontName = QFontDatabase::applicationFontFamilies(fontId).at(0);

	QFont font(fontName);
	font.setPointSize(10);
	tableWidget_->setFont(font);
}*/


RoundedOrderWidget::RoundedOrderWidget(QWidget* parent) : QWidget(parent)
{
	layout_ = std::make_unique<QVBoxLayout>();
	toolbar_ = std::make_unique<QHBoxLayout>();
	header_ = std::make_unique<QLabel>();

	mainTabwidget_ = std::make_unique<QTabWidget>();
	upTrendTable_   = std::make_unique<QTableWidget>();
	downTrendTable_ = std::make_unique<QTableWidget>();

	// strategy related
	strategyStart_ = std::make_unique<QPushButton>();
	latestUpdateTime_ = std::make_unique<QLabel>();
	latestUpdateTime_->setFont(QFont(header_->font().family(), -1, QFont::Bold, false));

	
	analyze_ = std::make_shared<analyzeStart>();
	mainTabwidget_->setIconSize(QSize(27, 27));

	configToolbar();
	configBorderEffect();
	configUpTab();
	configDownTab();
	configLayout();

	init();
	
	// refresh table
	connect(analyze_.get(), &analyzeStart::refreshTableRequested, this, &RoundedOrderWidget::refreshTable);
	// refresh time label
	connect(analyze_.get(), &analyzeStart::updateTimeLabel, this, &RoundedOrderWidget::refreshTimeLabel);
	
	connect(strategyStart_.get(), &QPushButton::clicked, analyze_.get(), &analyzeStart::start);
	connect(upTrendTable_.get(), &QTableWidget::cellClicked, this, &RoundedOrderWidget::upTrendTableCellClicked);
	connect(downTrendTable_.get(), &QTableWidget::cellClicked, this, &RoundedOrderWidget::downTrendTableCellClicked);
	connect(this, &RoundedOrderWidget::stockSelected, analyze_.get(), &analyzeStart::requestIndicatorFromDB);
	
}


void RoundedOrderWidget::setLinkedChartView(WChartView* linkedChart)
{
	linkedChartView_ = linkedChart;
	
	//refresh chart
	connect(this, &RoundedOrderWidget::stockSelected, linkedChartView_, &WChartView::updateChart);
	//refresh Indicator
	connect(this, &RoundedOrderWidget::stockSelected, analyze_.get(), &analyzeStart::requestIndicatorFromDB);
	connect(analyze_.get(), &analyzeStart::sendIndicatorToChart, linkedChartView_, &WChartView::updateIndicator);
}

void RoundedOrderWidget::refreshTable()
{
	log("NOTICE: auto refresh order tables.");
	upTrendTable_->clearContents();
	upTrendTable_->setRowCount(0);

	downTrendTable_->clearContents();
	downTrendTable_->setRowCount(0);

	init();

}

void RoundedOrderWidget::upTrendTableCellClicked(int row, int column) {
	if (upTableLastClicked_ == row) return;
	QTableWidgetItem* id    = upTrendTable_->item(row, 0);
	QTableWidgetItem* name  = upTrendTable_->item(row, 1);
	QTableWidgetItem* categ = upTrendTable_->item(row, 2);
	upTableLastClicked_ = row;

	StockInfo stockInfo{ id->text(), name->text(), categ->text() };

	emit stockSelected(stockInfo);
}

void RoundedOrderWidget::downTrendTableCellClicked(int row, int column)
{
	if (downTableLastClicked_ == row) return;
	QTableWidgetItem* id    = downTrendTable_->item(row, 0);
	QTableWidgetItem* name  = downTrendTable_->item(row, 1);
	QTableWidgetItem* categ = downTrendTable_->item(row, 2);
	downTableLastClicked_ = row;

	StockInfo stockInfo{ id->text(), name->text(), categ->text() };

	emit stockSelected(stockInfo);
}

void RoundedOrderWidget::paintEvent(QPaintEvent* ev)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	const QPalette& palette = this->palette();
	QColor backgroundColor = palette.color(QPalette::Window);
	QPainterPath path;
	path.addRoundedRect(contentsRect(), 10, 10);
	painter.fillPath(path, backgroundColor);
}

void RoundedOrderWidget::configToolbar()
{
	header_->setFont(QFont(header_->font().family(), -1, QFont::Bold, false));
	header_->setText("Stock Order");

	auto spacer = new QWidget(this);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	toolbar_->addWidget(header_.get());
	toolbar_->addWidget(spacer);

	latestUpdateTime_->setText(analyze_->getLastTime());
	toolbar_->addWidget(new QLabel("Last Updated: ", this));
	toolbar_->addWidget(latestUpdateTime_.get());
	
	strategyStart_->setMinimumSize(QSize(27, 27));
	strategyStart_->setMaximumSize(QSize(27, 27));
	strategyStart_->setIconSize(QSize(27, 27));
	strategyStart_->setStyleSheet(
		"QPushButton {"
		"   background-color: transparent;"
		"   border: none;"
		"   image: url(':/QtStockV3/src/icon/icon_collect-07.png');"
		"}"
		"QPushButton:hover {"
		"   background-color: #e5e5e5; /* Change to your desired background color */"
		"   border: 1px solid #999; /* Change to your desired border color */"
		"   image: url(':/QtStockV3/src/icon/icon_collect-06.png');"
		"}"
	);

	toolbar_->addWidget(strategyStart_.get());
}

void RoundedOrderWidget::configBorderEffect()
{
	auto borderEffect = new QGraphicsDropShadowEffect;
	borderEffect->setBlurRadius(ShadowWidth);
	borderEffect->setColor(QColor(125, 125, 125, 150));
	borderEffect->setOffset(0, 0);
	this->setGraphicsEffect(borderEffect);
	this->setAutoFillBackground(false);
	this->setContentsMargins(ShadowWidth, ShadowWidth, ShadowWidth, ShadowWidth);
}

void RoundedOrderWidget::configLayout()
{
	layout_->addLayout(toolbar_.get());

	auto separator = new QFrame;
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);

	layout_->addWidget(separator);
	layout_->addWidget(mainTabwidget_.get());
	
	this->setLayout(layout_.get());
}

void RoundedOrderWidget::configUpTab()
{
	QWidget* upTab_ = new QWidget();
	QGridLayout* gridLayout1_ = new QGridLayout(upTab_);
	gridLayout1_->setSpacing(0);
	gridLayout1_->setContentsMargins(11, 11, 11, 11);
	gridLayout1_->setObjectName("gridLayout_1");
	gridLayout1_->setSizeConstraint(QLayout::SetDefaultConstraint);
	gridLayout1_->setContentsMargins(0, 0, 0, 0);

	upTrendTable_->setParent(upTab_);
	upTrendTable_->setMouseTracking(true);
	upTrendTable_->setSortingEnabled(true);
	upTrendTable_->setSelectionMode(QAbstractItemView::SingleSelection);
	
	upTrendTable_->setAlternatingRowColors(true);
	upTrendTable_->setFocusPolicy(Qt::NoFocus);
	upTrendTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
	upTrendTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	upTrendTable_->setFrameStyle(QFrame::NoFrame);
	upTrendTable_->horizontalHeader()->setStretchLastSection(true);
	upTrendTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	upTrendTable_->setShowGrid(false);
	upTrendTable_->verticalHeader()->hide();
	upTrendTable_->setStyleSheet(
		"QTableWidget::item:selected{background-color: #1B89A1}"
	);

	QPalette p = upTrendTable_->palette();
	p.setColor(QPalette::Base, QColor(255, 255, 255));
	p.setColor(QPalette::AlternateBase, QColor(224, 224, 224));
	upTrendTable_->setPalette(p);

	QString horizontalHeaderStyle = "QHeaderView::section { \
										  background-color: rgb(204, 229, 255); /* header background color */ \
                                          border-left: 0.5px solid #b1b1b5; \
                                          border-top: 1px solid #b1b1b5; \
                                          border-right: 0.5px solid #b1b1b5; \
                                          border-bottom: 1px solid #b1b1b5; \
                                      }";

	upTrendTable_->horizontalHeader()->setStyleSheet(horizontalHeaderStyle);

	// table content font size
	// QString fontFile = "src/TaipeiSansTCBeta-Regular.ttf";
	// int fontId = QFontDatabase::addApplicationFont(fontFile);
	// QString fontName = QFontDatabase::applicationFontFamilies(fontId).at(0);

	QFont font("Microsoft JhengHei", -1, QFont::Bold, false);
	font.setPointSize(10);
	upTrendTable_->setFont(font);

	QIcon icon_;
	icon_.addFile(QString::fromUtf8(":/QtStockV3/src/icon/icon_collect-12.png"), QSize(), QIcon::Normal, QIcon::Off);
	mainTabwidget_->addTab(upTab_, icon_, QString());
	mainTabwidget_->setTabText(mainTabwidget_->indexOf(upTab_), QCoreApplication::translate("QtStockV3Class", "Rise List", nullptr));
	
	gridLayout1_->addWidget(upTrendTable_.get(), 0, 0, 1, 1);
}

void RoundedOrderWidget::configDownTab()
{
	QWidget* downTab_ = new QWidget();
	QGridLayout* gridLayout2_ = new QGridLayout(downTab_);
	gridLayout2_->setSpacing(0);
	gridLayout2_->setContentsMargins(11, 11, 11, 11);
	gridLayout2_->setObjectName("gridLayout_2");
	gridLayout2_->setSizeConstraint(QLayout::SetDefaultConstraint);
	gridLayout2_->setContentsMargins(0, 0, 0, 0);

	downTrendTable_->setParent(downTab_);
	downTrendTable_->setMouseTracking(true);
	downTrendTable_->setSortingEnabled(true);
	downTrendTable_->setSelectionMode(QAbstractItemView::SingleSelection);

	downTrendTable_->setAlternatingRowColors(true);
	downTrendTable_->setFocusPolicy(Qt::NoFocus);
	downTrendTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
	downTrendTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	downTrendTable_->setFrameStyle(QFrame::NoFrame);
	downTrendTable_->horizontalHeader()->setStretchLastSection(true);
	downTrendTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	downTrendTable_->setShowGrid(false);
	downTrendTable_->verticalHeader()->hide();
	downTrendTable_->setStyleSheet(
		"QTableWidget::item:selected{background-color: #1B89A1}"
	);

	QPalette p = downTrendTable_->palette();
	p.setColor(QPalette::Base, QColor(255, 255, 255));
	p.setColor(QPalette::AlternateBase, QColor(224, 224, 224));
	downTrendTable_->setPalette(p);

	QString horizontalHeaderStyle = "QHeaderView::section { \
										  background-color: rgb(204, 229, 255); /* header background color */ \
                                          border-left: 0.5px solid #b1b1b5; \
                                          border-top: 1px solid #b1b1b5; \
                                          border-right: 0.5px solid #b1b1b5; \
                                          border-bottom: 1px solid #b1b1b5; \
                                      }";

	downTrendTable_->horizontalHeader()->setStyleSheet(horizontalHeaderStyle);
	
	// table content font size
	// QString fontFile = "src/TaipeiSansTCBeta-Regular.ttf";
	// int fontId = QFontDatabase::addApplicationFont(fontFile);
	// QString fontName = QFontDatabase::applicationFontFamilies(fontId).at(0);

	QFont font("Microsoft JhengHei", -1, QFont::Bold, false);
	font.setPointSize(10);
	downTrendTable_->setFont(font);

	QIcon icon_;
	icon_.addFile(QString::fromUtf8(":/QtStockV3/src/icon/icon_collect-11.png"), QSize(), QIcon::Normal, QIcon::Off);
	mainTabwidget_->addTab(downTab_, icon_, QString());
	mainTabwidget_->setTabText(mainTabwidget_->indexOf(downTab_), QCoreApplication::translate("QtStockV3Class", "Fall List", nullptr));
	
	gridLayout2_->addWidget(downTrendTable_.get(), 0, 0, 1, 1);
}

void RoundedOrderWidget::databaseToTable(QSqlDatabase readFromDB, QTableWidget* tableWidget, const QString& sortOrder)
{
	if (readFromDB.open()) {
		QSqlQuery query(readFromDB);
		sqlToDefaultMode(query);

		TWSEList* twseList = new TWSEList();
		auto list_ = twseList->getStockList();

		QString sqlQuery = "SELECT StockID, Date, MAX(ROI) AS ROI, Price "
			"FROM orderList "
			"GROUP BY StockID "
			"ORDER BY ROI " + sortOrder + " "
			"LIMIT 30";

		if (query.exec(sqlQuery)) {

			int columnCount = query.record().count();
			tableWidget->setColumnCount(columnCount);

			// Populate the header labels of the table widget with the column names
			for (int i = 0; i < columnCount; i++) {
				QTableWidgetItem* item = new QTableWidgetItem(query.record().fieldName(i));
				tableWidget->setHorizontalHeaderItem(i, item);
			}
			tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
			tableWidget->horizontalHeader()->setStretchLastSection(false);

			int rowCount = 0;
			while (query.next()) {
				int stockId = query.value("StockID").toInt();
				double roi = query.value("ROI").toDouble();
				// qDebug() << "Stock ID:" << stockId << "ROI:" << roi;

				tableWidget->setRowCount(rowCount + 1);
				for (int i = 0; i < columnCount; i++) {
					auto itemvalue = query.value(i);
					QTableWidgetItem* item = new QTableWidgetItem(itemvalue.toString());
					switch (i) {
					case 2: // ROI column
						if (itemvalue.toDouble() > 0) {
							auto t = itemvalue.toDouble();
							item->setData(Qt::ForegroundRole, QColor(Qt::red));
						}
						else {
							item->setData(Qt::ForegroundRole, QColor(Qt::green));
						}
						break;
					case 3: // price
						item->setText(QString::number(itemvalue.toDouble(), 'f', 2));
						//item->setData(Qt::ForegroundRole, QColor(Qt::black));
						break;
					default:
						//item->setData(Qt::ForegroundRole, QColor(Qt::black));
						break;
					}
					item->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
					tableWidget->setItem(rowCount, i, item);

				}
				rowCount++;
			}

			// add name column
			tableWidget->insertColumn(1);
			tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("Name"));
			for (int row = 0; row < tableWidget->rowCount(); row++) {
				// Get the item from the previous column in the same row
				QString stockID = tableWidget->item(row, 0)->text(); // Assuming 0 is the index of the previous column
				QString stockName = QString::fromLocal8Bit(list_[stockID.toStdString()].first);
				QTableWidgetItem* newItem = new QTableWidgetItem(stockName);
				newItem->setData(Qt::ForegroundRole, QColor(Qt::black));
				newItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);


				tableWidget->setItem(row, 1, newItem);
			}

			log("NOTICE: Retrieve orders from database success.");
		}

	}
	else {
		QString message = "ERROR: Failed to open the database. " + readFromDB.lastError().text();
		log(message.toStdString());
	}
}

void RoundedOrderWidget::init()
{
	QFileInfo f(dbName_);
	if (!f.exists() || !f.isFile()) {
		log("ERROR: No order database. Please run the strategy first.");
		return;
	}

	QSqlDatabase readFromDB;
	if (QSqlDatabase::contains("readFromDB")) {
		readFromDB = QSqlDatabase::database("readFromDB");
	}
	else {
		readFromDB = QSqlDatabase::addDatabase("QSQLITE", "readFromDB");
	}
	readFromDB.setDatabaseName(dbName_);

	databaseToTable(readFromDB, upTrendTable_.get(), "DESC");
	databaseToTable(readFromDB, downTrendTable_.get(), "ASC");
	
}

void RoundedOrderWidget::refreshTimeLabel(const QString& time)
{
	latestUpdateTime_->setText(time);
}

void RoundedOrderWidget::sqlToDefaultMode(QSqlQuery& query)
{
	query.exec("PRAGMA journal_mode=DELETE;");
	if (query.next()) {
		QString mode = query.value(0).toString();
		QString msg = "Current SQLite mode is: Default";
		log(msg.toStdString());
	}
}

void RoundedOrderWidget::log(const std::string& message)
{
	Logger::getInstance().log(QString::fromStdString(message));
}
