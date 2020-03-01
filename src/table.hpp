#ifndef TABLE_HPP
#define TABLE_HPP

#include <QLocale>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QTableView>
#include <QtCharts>
#include "callout.hpp"
#include "model.hpp"
#include "ui_table.h"

class Table : public QWidget, protected Ui::Table {
  Q_OBJECT
 public:
  explicit Table(QWidget* parent = nullptr);

  QString name;
  Model* model;

  void set_database(const QSqlDatabase& database);
  void set_chart1_title(const QString& title);
  void set_chart2_title(const QString& title);
  void clear_charts();
  void calculate();

  virtual void init_model();

 signals:
  void hideProgressBar();
  void newChartMouseHover(const QPointF& point);

 protected:
  QSqlDatabase db;

  QChart* const chart1;
  QChart* const chart2;

  Callout* const callout1;
  Callout* const callout2;

  auto eventFilter(QObject* object, QEvent* event) -> bool override;
  void remove_selected_rows();
  void reset_zoom();
  void calculate_accumulated_sum(const QString& column_name);
  void calculate_accumulated_product(const QString& column_name);

  static void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
  void on_chart_selection(const bool& state);

 private:
  QLocale locale;

  int perc_chart_oldest_date = 0;

  void make_chart1();
  void make_chart2();

  void on_add_row();
};

#endif