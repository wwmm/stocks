#include "compare.hpp"
#include <QSqlError>
#include <QSqlQuery>
#include "chart_funcs.hpp"
#include "effects.hpp"
#include "math.hpp"

Compare::Compare(const QSqlDatabase& database, QWidget* parent)
    : db(database), chart(new QChart()), callout(new Callout(chart)) {
  setupUi(this);

  callout->hide();

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  frame_return->setGraphicsEffect(card_shadow());
  frame_accumulated_return->setGraphicsEffect(card_shadow());
  frame_time_window->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // chart settings

  chart->setTheme(QChart::ChartThemeLight);
  chart->setAcceptHoverEvents(true);
  chart->legend()->setAlignment(Qt::AlignRight);
  chart->legend()->setShowToolTips(true);

  chart_view->setChart(chart);
  chart_view->setRenderHint(QPainter::Antialiasing);
  chart_view->setRubberBand(QChartView::RectangleRubberBand);

  // signals

  connect(button_reset_zoom, &QPushButton::clicked, this, [&]() { chart->zoomReset(); });

  connect(radio_return_perc, &QRadioButton::toggled, this, &Compare::on_chart_selection);
  connect(radio_return_volatility, &QRadioButton::toggled, this, &Compare::on_chart_selection);
  connect(radio_accumulated_return_perc, &QRadioButton::toggled, this, &Compare::on_chart_selection);
  connect(radio_accumulated_return_second_derivative, &QRadioButton::toggled, this, &Compare::on_chart_selection);

  connect(spinbox_days, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) { process_tables(); });
}

void Compare::make_chart_return() {
  chart->setTitle("Return");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    QVector<int> dates;
    QVector<double> values;

    for (int n = 0; n < table->model->rowCount() && n < spinbox_days->value(); n++) {
      auto qdt = QDateTime::fromString(table->model->record(n).value("date").toString(), "dd/MM/yyyy");

      dates.append(qdt.toSecsSinceEpoch());
      values.append(table->model->record(n).value("return_perc").toDouble());
    }

    if (dates.size() < 2) {
      continue;
    }

    const auto s = add_series_to_chart(chart, dates, values, table->name);

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void Compare::make_chart_return_volatility() {
  chart->setTitle("Standard Deviation");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    QVector<int> dates;
    QVector<double> values;

    for (int n = 0; n < table->model->rowCount() && n < spinbox_days->value(); n++) {
      auto qdt = QDateTime::fromString(table->model->record(n).value("date").toString(), "dd/MM/yyyy");

      dates.append(qdt.toSecsSinceEpoch());
      values.append(table->model->record(n).value("return_perc").toDouble());
    }

    if (dates.size() < 2) {
      continue;
    }

    std::reverse(dates.begin(), dates.end());
    std::reverse(values.begin(), values.end());

    const auto s = add_series_to_chart(chart, dates, standard_deviation(values), table->name);

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void Compare::make_chart_accumulated_return() {
  chart->setTitle("Accumulated Return");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    QVector<int> dates;
    QVector<double> vreturn;
    QVector<double> accumulated_return;

    for (int n = 0; n < table->model->rowCount() && dates.size() < spinbox_days->value(); n++) {
      const auto qdt = QDateTime::fromString(table->model->record(n).value("date").toString(), "dd/MM/yyyy");

      dates.append(qdt.toSecsSinceEpoch());
      vreturn.append(table->model->record(n).value("return_perc").toDouble());
    }

    if (dates.size() < 2) {  // We need at least 2 points to show a line chart
      continue;
    }

    std::reverse(vreturn.begin(), vreturn.end());

    for (auto& value : vreturn) {
      value = value * 0.01 + 1.0;
    }

    accumulated_return.resize(vreturn.size());

    std::partial_sum(vreturn.begin(), vreturn.end(), accumulated_return.begin(), std::multiplies<>());

    for (auto& value : accumulated_return) {
      value = (value - 1.0) * 100;
    }

    std::reverse(accumulated_return.begin(), accumulated_return.end());

    const auto s = add_series_to_chart(chart, dates, accumulated_return, table->name.toUpper());

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void Compare::make_chart_accumulated_return_second_derivative() {
  chart->setTitle("Accumulated Return Second Derivative");

  add_axes_to_chart(chart, "");

  for (auto& table : tables) {
    QVector<int> dates;
    QVector<double> vreturn;
    QVector<double> accumulated_return;

    for (int n = 0; n < table->model->rowCount() && dates.size() < spinbox_days->value(); n++) {
      const auto qdt = QDateTime::fromString(table->model->record(n).value("date").toString(), "dd/MM/yyyy");

      dates.append(qdt.toSecsSinceEpoch());
      vreturn.append(table->model->record(n).value("return_perc").toDouble());
    }

    if (dates.size() < 3) {  // We need at least 3 points to calculate the second derivative
      continue;
    }

    std::reverse(dates.begin(), dates.end());
    std::reverse(vreturn.begin(), vreturn.end());

    for (auto& value : vreturn) {
      value = value * 0.01 + 1.0;
    }

    accumulated_return.resize(vreturn.size());

    std::partial_sum(vreturn.begin(), vreturn.end(), accumulated_return.begin(), std::multiplies<>());

    for (auto& value : accumulated_return) {
      value = (value - 1.0) * 100;
    }

    const auto s = add_series_to_chart(chart, dates, second_derivative(accumulated_return), table->name);

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void Compare::process(const QVector<Table const*>& tables) {
  this->tables = tables;

  process_tables();
}

void Compare::process_tables() {
  clear_chart(chart);

  if (radio_return_perc->isChecked()) {
    make_chart_return();
  } else if (radio_return_volatility->isChecked()) {
    make_chart_return_volatility();
  } else if (radio_accumulated_return_perc->isChecked()) {
    make_chart_accumulated_return();
  } else if (radio_accumulated_return_second_derivative->isChecked()) {
    make_chart_accumulated_return_second_derivative();
  }
}

void Compare::on_chart_selection(const bool& state) {
  if (!state) {
    return;
  }

  process_tables();
}

void Compare::on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name) {
  if (state) {
    const auto qdt = QDateTime::fromMSecsSinceEpoch(point.x());

    if (radio_return_perc->isChecked() || radio_accumulated_return_perc->isChecked()) {
      c->setText(QString("Fund: %1\nDate: %2\nReturn: %3%")
                     .arg(name, qdt.toString("MM/yyyy"), QString::number(point.y(), 'f', 2)));
    } else if (radio_return_volatility->isChecked()) {
      c->setText(QString("Fund: %1\nDate: %2\nValue: %3%")
                     .arg(name, qdt.toString("MM/yyyy"), QString::number(point.y(), 'f', 2)));
    } else if (radio_accumulated_return_second_derivative->isChecked()) {
      c->setText(QString("Fund: %1\nDate: %2\nValue: %3")
                     .arg(name, qdt.toString("MM/yyyy"), QString::number(point.y(), 'f', 2)));
    }

    c->setAnchor(point);

    c->setZValue(11);

    c->updateGeometry();

    c->show();
  } else {
    c->hide();
  }
}
