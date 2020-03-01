#ifndef PCA_HPP
#define PCA_HPP

#include <QSqlDatabase>
#include "callout.hpp"
#include "table.hpp"
#include "ui_pca.h"

class PCA : public QWidget, protected Ui::PCA {
  Q_OBJECT
 public:
  explicit PCA(const QSqlDatabase& database, QWidget* parent = nullptr);

  void process(const QVector<Table const*>& tables);

 private:
  QSqlDatabase db;

  QChart* chart;

  Callout* callout;

  QVector<Table const*> tables;

  void process_tables();
};

#endif