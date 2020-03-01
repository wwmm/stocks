#include "table.hpp"
#include "chart_funcs.hpp"
#include "effects.hpp"

Table::Table(QWidget* parent)
    : QWidget(parent),
      model(nullptr),
      chart1(new QChart()),
      chart2(new QChart()),
      callout1(new Callout(chart1)),
      callout2(new Callout(chart2)) {
  setupUi(this);

  callout1->hide();
  callout2->hide();

  table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  table_view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  table_view->installEventFilter(this);

  // shadow effects

  button_add_row->setGraphicsEffect(button_shadow());
  chart_cfg_frame->setGraphicsEffect(card_shadow());
  frame_chart->setGraphicsEffect(card_shadow());
  frame_tableview->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // signals

  connect(button_add_row, &QPushButton::clicked, this, &Table::on_add_row);
  connect(button_reset_zoom, &QPushButton::clicked, this, &Table::reset_zoom);
  connect(radio_chart1, &QRadioButton::toggled, this, &Table::on_chart_selection);
  connect(radio_chart2, &QRadioButton::toggled, this, &Table::on_chart_selection);

  connect(spinbox_days, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
    clear_chart(chart2);
    make_chart2();
  });

  // chart 1 settings

  chart1->setTheme(QChart::ChartThemeLight);
  chart1->setAcceptHoverEvents(true);

  chart_view1->setChart(chart1);
  chart_view1->setRenderHint(QPainter::Antialiasing);
  chart_view1->setRubberBand(QChartView::RectangleRubberBand);

  // chart 2 settings

  chart2->setTheme(QChart::ChartThemeLight);
  chart2->setAcceptHoverEvents(true);

  chart_view2->setChart(chart2);
  chart_view2->setRenderHint(QPainter::Antialiasing);
  chart_view2->setRubberBand(QChartView::RectangleRubberBand);

  // select the default chart

  if (radio_chart1->isChecked()) {
    stackedwidget->setCurrentIndex(0);

    label_months->hide();
    spinbox_days->hide();
  } else if (radio_chart2->isChecked()) {
    stackedwidget->setCurrentIndex(1);

    label_months->show();
    spinbox_days->show();
  }
}

void Table::set_database(const QSqlDatabase& database) {
  db = database;

  model = new Model(db);
}

void Table::set_chart1_title(const QString& title) {
  chart1->setTitle(title);
}

void Table::set_chart2_title(const QString& title) {
  chart2->setTitle(title);
}

void Table::init_model() {
  model->setTable(name);
  model->setEditStrategy(QSqlTableModel::OnManualSubmit);
  model->setSort(1, Qt::DescendingOrder);

  auto currency = QLocale().currencySymbol();

  model->setHeaderData(1, Qt::Horizontal, "Date");
  model->setHeaderData(2, Qt::Horizontal, "Value\n" + currency);
  model->setHeaderData(3, Qt::Horizontal, "Return\n%");
  model->setHeaderData(4, Qt::Horizontal, "Accumulated Return\n%");

  model->select();

  table_view->setModel(model);
  table_view->setColumnHidden(0, true);
}

auto Table::eventFilter(QObject* object, QEvent* event) -> bool {
  if (event->type() == QEvent::KeyPress) {
    auto* keyEvent = dynamic_cast<QKeyEvent*>(event);

    if (keyEvent->key() == Qt::Key_Delete) {
      remove_selected_rows();

      return true;
    }

    if (keyEvent->matches(QKeySequence::Copy)) {
      auto s_model = table_view->selectionModel();

      if (s_model->hasSelection()) {
        auto selection_range = s_model->selection().constFirst();

        QString table_str;

        auto clipboard = QGuiApplication::clipboard();

        for (int i = selection_range.top(); i <= selection_range.bottom(); i++) {
          QString row_value;

          for (int j = selection_range.left(); j <= selection_range.right(); j++) {
            if (j < selection_range.right()) {
              if (j > 1) {
                row_value += locale.toString(s_model->model()->index(i, j).data().toDouble()) + "\t";
              } else {
                row_value += s_model->model()->index(i, j).data().toString() + "\t";
              }
            } else {
              if (j > 1) {
                row_value += locale.toString(s_model->model()->index(i, j).data().toDouble());
              } else {
                row_value += s_model->model()->index(i, j).data().toString();
              }
            }
          }

          table_str += row_value + "\n";
        }

        clipboard->setText(table_str);
      }

      return true;
    }

    if (keyEvent->matches(QKeySequence::Paste)) {
      auto s_model = table_view->selectionModel();

      if (s_model->hasSelection()) {
        auto clipboard = QGuiApplication::clipboard();

        auto table_str = clipboard->text();
        auto table_rows = table_str.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);

        auto selection_range = s_model->selection().constFirst();

        auto first_row = selection_range.top();
        auto first_col = selection_range.left();

        for (int i = 0; i < table_rows.size(); i++) {
          auto model_i = first_row + i;

          if (model_i < model->rowCount()) {
            auto row_cols = table_rows[i].split("\t");

            for (int j = 0; j < row_cols.size(); j++) {
              auto model_j = first_col + j;

              if (model_j < model->columnCount()) {
                model->setData(model->index(model_i, model_j), row_cols[j], Qt::EditRole);
              }
            }
          }
        }
      }

      return true;
    }

    return QObject::eventFilter(object, event);
  }

  return QObject::eventFilter(object, event);
}

void Table::remove_selected_rows() {
  auto s_model = table_view->selectionModel();

  if (s_model->hasSelection()) {
    auto index_list = s_model->selectedIndexes();

    QSet<int> row_set;
    QSet<int> column_set;

    for (auto& index : index_list) {
      row_set.insert(index.row());
      column_set.insert(index.column());
    }

    for (int idx = 1; idx < model->columnCount(); idx++) {
      if (!column_set.contains(idx)) {
        return;
      }
    }

    QList<int> row_list = row_set.values();

    if (!row_list.empty()) {
      std::sort(row_list.begin(), row_list.end());
      std::reverse(row_list.begin(), row_list.end());

      for (auto& index : row_list) {
        model->removeRow(index);
      }
    }
  }
}

void Table::clear_charts() {
  chart1->removeAllSeries();
  chart2->removeAllSeries();

  for (auto& axis : chart1->axes()) {
    chart1->removeAxis(axis);
  }

  for (auto& axis : chart2->axes()) {
    chart2->removeAxis(axis);
  }
}

void Table::on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name) {
  if (state) {
    auto qdt = QDateTime();

    qdt.setMSecsSinceEpoch(point.x());

    c->setText(QString("Curve: %1\nDate: %2\nReturn: %3")
                   .arg(name, qdt.toString("dd/MM/yyyy"), QString::number(point.y(), 'f', 2)));

    c->setAnchor(point);

    c->setZValue(11);

    c->updateGeometry();

    c->show();
  } else {
    c->hide();
  }
}

void Table::reset_zoom() {
  if (radio_chart1->isChecked()) {
    chart1->zoomReset();
  }

  if (radio_chart2->isChecked()) {
    chart2->zoomReset();
  }
}

void Table::on_chart_selection(const bool& state) {
  if (state) {
    if (radio_chart1->isChecked()) {
      stackedwidget->setCurrentIndex(0);

      label_months->hide();
      spinbox_days->hide();
    } else if (radio_chart2->isChecked()) {
      stackedwidget->setCurrentIndex(1);

      label_months->show();
      spinbox_days->show();
    }
  }
}

void Table::on_add_row() {
  auto rec = model->record();

  rec.setGenerated("id", false);

  rec.setValue("date", int(QDateTime::currentSecsSinceEpoch()));

  rec.setGenerated("value", true);
  rec.setGenerated("accumulated", true);
  rec.setGenerated("return_perc", true);
  rec.setGenerated("accumulated_return_perc", true);

  rec.setValue("value", 0.0);
  rec.setValue("return_perc", 0.0);
  rec.setValue("accumulated_return_perc", 0.0);

  if (!model->insertRecord(0, rec)) {
    qDebug() << "failed to add row to table " + name;
  }
}

void Table::calculate_accumulated_sum(const QString& column_name) {
  QVector<double> list_values;
  QVector<double> accu;

  for (int n = 0; n < model->rowCount(); n++) {
    list_values.append(model->record(n).value(column_name).toDouble());
  }

  if (!list_values.empty()) {
    std::reverse(list_values.begin(), list_values.end());

    // cumulative sum

    accu.resize(list_values.size());

    std::partial_sum(list_values.begin(), list_values.end(), accu.begin());

    std::reverse(accu.begin(), accu.end());

    for (int n = 0; n < model->rowCount(); n++) {
      auto rec = model->record(n);

      rec.setGenerated("accumulated_" + column_name, true);

      rec.setValue("accumulated_" + column_name, accu[n]);

      model->setRecord(n, rec);
    }
  }
}

void Table::calculate_accumulated_product(const QString& column_name) {
  QVector<double> list_values;
  QVector<double> accu;

  for (int n = 0; n < model->rowCount(); n++) {
    list_values.append(model->record(n).value(column_name).toDouble());
  }

  if (!list_values.empty()) {
    std::reverse(list_values.begin(), list_values.end());

    for (auto& value : list_values) {
      value = value * 0.01 + 1.0;
    }

    // cumulative product

    accu.resize(list_values.size());

    std::partial_sum(list_values.begin(), list_values.end(), accu.begin(), std::multiplies<>());

    for (auto& value : accu) {
      value = (value - 1.0) * 100;
    }

    std::reverse(accu.begin(), accu.end());

    for (int n = 0; n < model->rowCount(); n++) {
      auto rec = model->record(n);

      rec.setGenerated("accumulated_" + column_name, true);

      rec.setValue("accumulated_" + column_name, accu[n]);

      model->setRecord(n, rec);
    }
  }
}

void Table::calculate() {
  if (model->rowCount() == 0) {
    return;
  }

  auto rec = model->record(model->rowCount() - 1);

  rec.setGenerated("net_return", true);
  rec.setGenerated("net_return_perc", true);

  rec.setValue("net_return", 0);
  rec.setValue("net_return_perc", 0);

  model->setRecord(model->rowCount() - 1, rec);

  for (int n = model->rowCount() - 2; n >= 0; n--) {
    double value = model->record(n).value("value").toDouble();
    double last_value = model->record(n + 1).value("value").toDouble();

    double vreturn = value - last_value;

    double return_perc = 100 * vreturn / last_value;

    auto rec = model->record(n);

    rec.setGenerated("return", true);
    rec.setGenerated("return_perc", true);

    rec.setValue("return", vreturn);
    rec.setValue("return_perc", return_perc);

    model->setRecord(n, rec);
  }

  calculate_accumulated_product("return_perc");

  clear_charts();

  make_chart1();
  make_chart2();
}

void Table::make_chart1() {
  chart1->setTitle(name.toUpper());

  add_axes_to_chart(chart1, QLocale().currencySymbol());

  auto s1 = add_series_to_chart(chart1, model, "Value", "value");

  connect(s1, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout1, s1->name()); });
}

void Table::make_chart2() {
  chart2->setTitle(name.toUpper());

  add_axes_to_chart(chart2, "%");

  QVector<int> dates;
  QVector<double> vreturn;
  QVector<double> accumulated_return;

  for (int n = 0; n < model->rowCount() && dates.size() < spinbox_days->value(); n++) {
    auto qdt = QDateTime::fromString(model->record(n).value("date").toString(), "dd/MM/yyyy");

    dates.append(qdt.toSecsSinceEpoch());
    vreturn.append(model->record(n).value("return_perc").toDouble());
  }

  if (dates.empty()) {
    return;
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

  perc_chart_oldest_date = dates[dates.size() - 1];

  auto s1 = add_series_to_chart(chart2, dates, accumulated_return, "Accumulated Return");

  connect(s1, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, s1->name()); });
}
