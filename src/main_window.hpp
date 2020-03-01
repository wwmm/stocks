#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "compare.hpp"
#include "correlation.hpp"
#include "pca.hpp"
#include "ui_main_window.h"

class MainWindow : public QMainWindow, private Ui::MainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QMainWindow* parent = nullptr);

 private:
  QSettings qsettings;

  QSqlDatabase db;

  auto load_compare() -> Compare*;
  auto load_correlation() -> Correlation*;
  auto load_pca() -> PCA*;

  void add_table();
  void load_saved_tables();
  void clear_table(const QStackedWidget* sw);
  void remove_table(QListWidget* lw, QStackedWidget* sw);

  static void save_table(const QStackedWidget* sw);

  void on_save_table();
  void on_clear_table();
  void on_remove_table();
  void on_run_analysis();

  void on_listwidget_item_changed(QListWidgetItem* item, QListWidget* lw, QStackedWidget* sw);

  template <class T>
  auto load_table(const QString& name, QStackedWidget* sw, QListWidget* lw) -> T* {
    auto table = new T();

    table->set_database(db);
    table->name = name;
    table->init_model();

    sw->addWidget(table);

    lw->addItem(name);

    auto added_item = lw->item(lw->count() - 1);

    added_item->setFlags(added_item->flags() | Qt::ItemIsEditable);

    return table;
  }
};

#endif