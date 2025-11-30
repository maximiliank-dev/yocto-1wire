from PySide6.QtWidgets import QWidget, QToolTip
from PySide6.QtCore import QDateTime, Qt, QPointF
from PySide6.QtGui import QPainter, QCursor
from PySide6.QtWidgets import QVBoxLayout, QSizePolicy, QLabel
from PySide6.QtCharts import QChart, QChartView, QLineSeries, QValueAxis, QDateTimeAxis


class Graph2D(QWidget):
    """Graph Widget"""

    def __init__(self, data):
        super().__init__()
        self.init(data)

    def init(self, data):

        # Creating QChart and QChartView
        self.chart = QChart()
        self.chart.setAnimationOptions(QChart.AllAnimations)

        self.chart_view = QChartView(self.chart)
        self.chart_view.setRenderHint(QPainter.Antialiasing)

        # Layout
        self.main_layout = QVBoxLayout()
        size = QSizePolicy()
        size.setHorizontalPolicy(QSizePolicy.Policy.Maximum)

        label = QLabel("Temperature Plot")
        current_font = label.font()
        current_font.setBold(True)
        label.setFont(current_font)

        self.main_layout.addWidget(label)

        # Right Layout
        self.main_layout.addWidget(self.chart_view)

        self.setLayout(self.main_layout)

        self.setData(data)

    def setData(self, data: list):

        self.qdata = QLineSeries()
        self.qdata.setName("Temperature data")
        self.qdata.hovered.connect(self.on_point_hovered)

        paired_list = zip(data[0], data[1])
        for el in paired_list:
            x, y = el
            self.qdata.append(x.toMSecsSinceEpoch(), y)

        #before the axis is initalized
        self.chart.addSeries(self.qdata)

        #x axis
        self.axis_x = QDateTimeAxis()
        self.axis_x.setTitleText("Timestamp")
        self.axis_x.setFormat("hh:mm:ss")
        self.chart.addAxis(self.axis_x, Qt.AlignBottom)
        self.qdata.attachAxis(self.axis_x)
        # y axis
        self.axis_y = QValueAxis()
        self.axis_y.setLabelFormat("%.3f")
        self.axis_y.setTitleText("Y Data")
        self.chart.addAxis(self.axis_y, Qt.AlignLeft)
        self.qdata.attachAxis(self.axis_y)

    def data_update(self, data):
        """Append new data and rescale the Axix"""
        xs, ys = data[0], data[1]

        pts = [QPointF(x.toMSecsSinceEpoch(), y) for x, y in zip(xs, ys)]

        if len(pts) == 0:
            return

        for p in pts:
            self.qdata.append(p)

        all_pts = self.qdata.pointsVector()
        if not all_pts:
            raise Exception("Error self.qdata has no points")

        # xAxis rescale
        xmin_ms = int(all_pts[0].x())
        xmax_ms = int(all_pts[-1].x())
        self.axis_x.setRange(
            QDateTime.fromMSecsSinceEpoch(xmin_ms),
            QDateTime.fromMSecsSinceEpoch(xmax_ms),
        )

        # yAxis rescale
        ys_all = [p.y() for p in all_pts]
        ymin, ymax = min(ys_all) * 0.95, max(ys_all) * 1.05
        self.axis_y.setRange(ymin, ymax)

    def on_point_hovered(self, point, state):
        """Show coordinates when hovering a point"""
        if state:
            x_dt = QDateTime.fromMSecsSinceEpoch(int(
                point.x())).toString("HH:mm:ss")
            y_val = point.y()

            text = f"<b>Time:</b> {x_dt}<br><b>Temperature:</b> {y_val:.2f}"
            QToolTip.showText(QCursor.pos(), text)
        else:
            QToolTip.hideText()
