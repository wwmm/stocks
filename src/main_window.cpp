#include "main_window.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include "effects.hpp"
#include "table.hpp"

MainWindow::MainWindow(QMainWindow* parent) : QMainWindow(parent) {
  setupUi(this);

  tab_widget->setCurrentIndex(0);

  // shadow effects

  frame_table_selection_stocks->setGraphicsEffect(card_shadow());
  button_add_table->setGraphicsEffect(button_shadow());
  button_remove_table->setGraphicsEffect(button_shadow());
  button_clear_table->setGraphicsEffect(button_shadow());
  button_save_table->setGraphicsEffect(button_shadow());
  button_calculate_table->setGraphicsEffect(button_shadow());
  button_run_analysis->setGraphicsEffect(button_shadow());

  button_database_file->setGraphicsEffect(button_shadow());

  // signals

  connect(button_add_table, &QPushButton::clicked, this, &MainWindow::add_table);
  connect(button_remove_table, &QPushButton::clicked, this, &MainWindow::on_remove_table);
  connect(button_clear_table, &QPushButton::clicked, this, &MainWindow::on_clear_table);
  connect(button_save_table, &QPushButton::clicked, this, &MainWindow::on_save_table);
  connect(button_run_analysis, &QPushButton::clicked, this, &MainWindow::on_run_analysis);

  connect(button_calculate_table, &QPushButton::clicked, this, [&]() {
    auto table = dynamic_cast<Table*>(stackedwidget_stocks->widget(stackedwidget_stocks->currentIndex()));

    table->calculate();
  });

  connect(button_database_file, &QPushButton::clicked, this, [&]() {
    auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDesktopServices::openUrl(path);
  });

  connect(listwidget_analysis, &QListWidget::currentRowChanged, this,
          [&](int currentRow) { stackedwidget_analysis->setCurrentIndex(currentRow); });

  connect(listwidget_tables_stocks, &QListWidget::currentRowChanged, this,
          [&](int currentRow) { stackedwidget_stocks->setCurrentIndex(currentRow); });
  connect(listwidget_tables_stocks, &QListWidget::itemChanged, this, [&](QListWidgetItem* item) {
    on_listwidget_item_changed(item, listwidget_tables_stocks, stackedwidget_stocks);
  });

  // apply custom stylesheet

  QFile styleFile(":/custom.css");

  styleFile.open(QFile::ReadOnly);

  QString style(styleFile.readAll());

  setStyleSheet(style);

  styleFile.close();

  // load the sqlite database file

  if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
    qCritical("sqlite driver is not available!");
  } else {
    db = QSqlDatabase::addDatabase("QSQLITE");

    auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (!QDir(path).exists()) {
      QDir().mkpath(path);
    }

    path += "/stocks.sqlite";

    qDebug() << "Database file: " + path.toLatin1();

    db.setDatabaseName(path);

    if (db.open()) {
      qDebug("The database file was opened!");

      load_saved_tables();

      auto tables = QVector<Table const*>();

      for (int n = 0; n < stackedwidget_stocks->count(); n++) {
        tables.append(dynamic_cast<Table const*>(stackedwidget_stocks->widget(n)));
      }

      auto compare = load_compare();
      auto correlation = load_correlation();
      auto pca = load_pca();

      listwidget_analysis->setCurrentRow(0);

      // This has to be done after loading the other tables

      compare->process(tables);
      correlation->process(tables);
      pca->process(tables);
    } else {
      qCritical("Failed to open the database file!");
    }
  }

  show();
}

auto MainWindow::load_compare() -> Compare* {
  auto c = new Compare(db);

  stackedwidget_analysis->addWidget(c);

  listwidget_analysis->addItem("Compare");

  return c;
}

auto MainWindow::load_correlation() -> Correlation* {
  auto c = new Correlation(db);

  stackedwidget_analysis->addWidget(c);

  listwidget_analysis->addItem("Correlation");

  return c;
}

auto MainWindow::load_pca() -> PCA* {
  auto pca = new PCA(db);

  stackedwidget_analysis->addWidget(pca);

  listwidget_analysis->addItem("PCA");

  return pca;
}

void MainWindow::add_table() {
  auto name = QString("stock%1").arg(stackedwidget_stocks->count());

  auto query = QSqlQuery(db);

  query.prepare("create table " + name +
                " (id integer primary key, date int default (cast(strftime('%s','now') as int))," +
                " value real default 0.0, return_perc real default 0.0, accumulated_return_perc real default 0.0)");

  if (query.exec()) {
    load_table<Table>(name, stackedwidget_stocks, listwidget_tables_stocks);

    stackedwidget_stocks->setCurrentIndex(stackedwidget_stocks->count() - 1);

    listwidget_tables_stocks->setCurrentRow(listwidget_tables_stocks->count() - 1);
  } else {
    qDebug() << "Failed to create table " + name.toUtf8() + ". Maybe it already exists.";
  }
}

void MainWindow::load_saved_tables() {
  auto query = QSqlQuery(db);

  query.prepare("select name from sqlite_master where type='table'");

  if (query.exec()) {
    auto names = QVector<QString>();

    while (query.next()) {
      auto name = query.value(0).toString();

      names.append(name);
    }

    std::sort(names.begin(), names.end());

    auto stocks = QVector<QString>();

    for (auto& name : names) {
      qInfo() << "Found table: " + name.toUtf8();

      auto query = QSqlQuery(db);

      query.prepare("select * from " + name);

      if (query.exec()) {
        stocks.append(name);
      }
    }

    for (auto& name : stocks) {
      load_table<Table>(name, stackedwidget_stocks, listwidget_tables_stocks);
    }

    for (int n = 0; n < stackedwidget_stocks->count(); n++) {
      auto table = dynamic_cast<Table*>(stackedwidget_stocks->widget(n));

      table->calculate();
    }

    if (listwidget_tables_stocks->count() > 0) {
      listwidget_tables_stocks->setCurrentRow(0);
    }
  } else {
    qDebug("Failed to get table names!");
  }
}

void MainWindow::on_listwidget_item_changed(QListWidgetItem* item, QListWidget* lw, QStackedWidget* sw) {
  if (item == lw->currentItem()) {
    QString new_name = item->text();

    auto table = dynamic_cast<Table*>(sw->widget(sw->currentIndex()));

    // finish any pending operation before changing the table name

    table->model->submitAll();

    auto query = QSqlQuery(db);

    query.prepare("alter table " + table->name + " rename to " + new_name);

    if (query.exec()) {
      table->name = new_name;

      lw->currentItem()->setText(new_name.toUpper());

      table->init_model();

      table->set_chart1_title(new_name);
      table->set_chart2_title(new_name);
    } else {
      qDebug() << "failed to rename table " + table->name.toUtf8();
    }
  }
}

void MainWindow::remove_table(QListWidget* lw, QStackedWidget* sw) {
  auto box = QMessageBox(this);

  box.setText("Remove the selected table from the database?");
  box.setInformativeText("This action cannot be undone!");
  box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  box.setDefaultButton(QMessageBox::Yes);

  auto r = box.exec();

  if (r == QMessageBox::Yes) {
    auto table = dynamic_cast<Table*>(sw->widget(sw->currentIndex()));

    sw->removeWidget(sw->widget(sw->currentIndex()));

    auto it = lw->takeItem(lw->currentRow());

    delete it;

    qsettings.beginGroup(table->name);

    qsettings.remove("");

    qsettings.endGroup();

    auto query = QSqlQuery(db);

    query.prepare("drop table if exists " + table->name);

    if (!query.exec()) {
      qDebug() << "Failed remove table " + table->name.toUtf8() + ". Maybe has already been removed.";
    }
  }
}

void MainWindow::on_remove_table() {
  remove_table(listwidget_tables_stocks, stackedwidget_stocks);
}

void MainWindow::clear_table(const QStackedWidget* sw) {
  if (sw->count() < 1) {
    return;
  }

  auto box = QMessageBox(this);

  box.setText("Remove all rows from the selected table?");
  box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  box.setDefaultButton(QMessageBox::Yes);

  auto r = box.exec();

  if (r == QMessageBox::Yes) {
    auto table = dynamic_cast<Table*>(sw->widget(sw->currentIndex()));

    auto query = QSqlQuery(db);

    query.prepare("delete from " + table->name);

    if (query.exec()) {
      table->model->select();

      table->clear_charts();
    } else {
      qDebug() << table->model->lastError().text().toUtf8();
    }
  }
}

void MainWindow::on_clear_table() {
  clear_table(stackedwidget_stocks);
}

void MainWindow::save_table(const QStackedWidget* sw) {
  auto table = dynamic_cast<Table*>(sw->widget(sw->currentIndex()));

  if (!table->model->submitAll()) {
    qDebug() << "failed to save table " + table->name.toUtf8() + " to the database";

    qDebug() << table->model->lastError().text().toUtf8();
  }
}

void MainWindow::on_save_table() {
  save_table(stackedwidget_stocks);
}

void MainWindow::on_run_analysis() {
  auto tables = QVector<Table const*>();

  for (int n = 0; n < stackedwidget_stocks->count(); n++) {
    tables.append(dynamic_cast<Table const*>(stackedwidget_stocks->widget(n)));
  }

  auto compare = dynamic_cast<Compare*>(stackedwidget_analysis->widget(0));

  compare->process(tables);

  auto correlation = dynamic_cast<Correlation*>(stackedwidget_analysis->widget(1));

  correlation->process(tables);

  auto pca = dynamic_cast<PCA*>(stackedwidget_analysis->widget(2));

  pca->process(tables);
}