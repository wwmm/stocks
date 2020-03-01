#ifndef CORRELATION_HPP
#define CORRELATION_HPP

#include <QSqlDatabase>
#include "callout.hpp"
#include "table.hpp"
#include "ui_correlation.h"

class Correlation : public QWidget, protected Ui::Correlation {
  Q_OBJECT
 public:
  explicit Correlation(const QSqlDatabase& database, QWidget* parent = nullptr);

  void process(const QVector<Table const*>& tables);

 private:
  QSqlDatabase db;

  QChart* const chart;

  Callout* const callout;

  QVector<Table const*> tables;

  void process_tables();

  static void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
};

#endif