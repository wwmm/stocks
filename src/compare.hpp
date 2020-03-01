#ifndef COMPARE_HPP
#define COMPARE_HPP

#include <QSqlDatabase>
#include <deque>
#include "callout.hpp"
#include "table.hpp"
#include "ui_compare.h"

class Compare : public QWidget, protected Ui::Compare {
  Q_OBJECT
 public:
  explicit Compare(const QSqlDatabase& database, QWidget* parent = nullptr);

  void process(const QVector<Table const*>& tables);

 private:
  QSqlDatabase db;

  QChart* const chart;

  Callout* const callout;

  QVector<Table const*> tables;

  void process_tables();

  void make_chart_return();
  void make_chart_return_volatility();
  void make_chart_accumulated_return();
  void make_chart_accumulated_return_second_derivative();

  void on_chart_selection(const bool& state);
  void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
};

#endif