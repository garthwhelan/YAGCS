#include "mainwindow.h"
#include "gfx_helpers.h"
#include "ui_mainwindow.h"
#include <QButtonGroup>
#include <QColor>
#include <QDoubleValidator>
#include <QFile>
#include <QGraphicsView>
#include <QPair>
#include <QSerialPort>
#include <QTimer>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QGoochMaterial>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>
#include <iostream>
#include <QFileDialog>

const int REQUEST_DATA_MS = 100;
const int RUN_GCODE_MS = 10;

void MainWindow::on_input_command_returnPressed() {
  QString text = this->findChild<QLineEdit *>("input_command")->text() + "\n";
  this->findChild<QLineEdit *>("input_command")->setText("");
  this->writeData(text.toLocal8Bit());
}

void MainWindow::on_play_button_clicked() {
  qDebug() << this->gcode_queue.length();
  this->sending_gcode = true;
}

void MainWindow::on_reset_button_clicked() {
  const unsigned char res[1] = {0x18};
  this->writeData(QByteArray((const char*)res));
  this->response_count = 0;
}

void MainWindow::on_feed_cp_button_clicked() {
  const unsigned char res[1] = {0x91};
  this->writeData(QByteArray((const char*)res));  
}
void MainWindow::on_feed_cm_button_clicked() {
  const unsigned char res[1] = {0x92};
  this->writeData(QByteArray((const char*)res));  
}
void MainWindow::on_feed_reset_button_clicked() {
  const unsigned char res[1] = {0x90};
  this->writeData(QByteArray((const char*)res));  
}
void MainWindow::on_speed_cp_button_clicked() {
  const unsigned char res[1] = {0x9A};
  this->writeData(QByteArray((const char*)res));  
}
void MainWindow::on_speed_cm_button_clicked() {
  const unsigned char res[1] = {0x9B};
  this->writeData(QByteArray((const char*)res));  
}
void MainWindow::on_speed_reset_button_clicked() {
  const unsigned char res[1] = {0x99};
  this->writeData(QByteArray((const char*)res));
}

void MainWindow::on_pause_button_clicked() {
  this->sending_gcode = false;
}

void MainWindow::on_stop_button_clicked() {
  this->sending_gcode = false;
  this->gcode_queue = QStringList{};
}

void MainWindow::send_gcode() {

  if (this->sending_gcode == false and
      this->gcode_queue.length() > 0) {
    this->findChild<QPushButton *>("play_button")->setEnabled(true);
  } else {
    this->findChild<QPushButton *>("play_button")->setEnabled(false);
  }
  while (this->sending_gcode and this->gcode_queue.length() > 0 and this->response_count == 0) {
    QString line = this->gcode_queue.first();
    this->gcode_queue.pop_front();
    if (line.left(1) == "(") { // Don't send comments
      continue;
    }
    if (line.contains("M6")) { // Pause for user to change tool
      this->response_count -= 1;          
      this->writeData((line+"\n").toLocal8Bit());
      this->sending_gcode = false;
      continue;
    }
    this->response_count -= 1;    
    this->writeData((line+"\n").toLocal8Bit());
    qDebug() << line;
    if (this->gcode_queue.length() == 0) {
      this->sending_gcode = false;
    }
  }
}

void MainWindow::readLineData() {
  while (p->canReadLine()) {
    
    QString read_string = p->readLine();

    if (read_string.right(2) == "\r\n") {
      read_string = read_string.left(read_string.length() - 2);
    } else if (read_string.right(1) == "\n") {
      read_string = read_string.left(read_string.length() - 1);
    }

    qDebug() << "FROMGRBL:" << read_string;
        
    if (read_string == "ok") {
      this->response_count += 1;
      continue;
    }
    
    if (read_string == "$G") {
      continue;
    }

    if (read_string.left(3) == "$J=") {
      continue;
    }

    if (read_string.left(1) == "<" and read_string.right(1) == ">") {
      QStringList msg_components = read_string.right(read_string.length() - 1)
        .left(read_string.length() - 1)
        .split("|");

      QString state = msg_components.takeFirst();
      if (state == "Idle") {
        this->state.status = IDLE;
      } else if (state == "Jog") {
        this->state.status = JOG;
      } else if (state == "Check") {
        this->state.status = CHECK;
      } else if (state == "Hold:0") {
        this->state.status = HOLD0;
      } else if (state == "Hold:1") {
        this->state.status = HOLD1;
      } else if (state == "Run") {
        this->state.status = RUN;
      } else if (state == "Alarm") {
        this->state.status = ALARM;
      } else {
        assert(false);
      }

      for (QString s : msg_components) {
        if (s.left(5) == "MPos:") { // Machine Position
          QStringList l = s.right(s.length() - 5).split(",");
          this->state.xyz_mcs = {l[0].toDouble(), l[1].toDouble(),
                                 l[2].toDouble()};
        } else if (s.left(3) == "FS:") { // Feed, Speed
          QStringList l = s.right(s.length() - 3).left(s.length() - 4).split(",");
          this->state.fs = {l[0].toDouble(), l[1].toDouble()};
        } else if (s.left(3) == "Ov:") { // Feed, Rapid, Speed Overrides
          QStringList l = s.right(s.length() - 3).left(s.length() - 4).split(",");
          this->state.ov = {l[0].toDouble(), l[1].toDouble(),
                            l[2].toDouble()};
          this->findChild<QLabel *>("feed_label")->setText("Feed: " + QString::number(this->state.ov[0]) + "%");
          this->findChild<QLabel *>("speed_label")->setText("Spindle: " + QString::number(this->state.ov[2]) + "%");
        } else if (s.left(4) == "WCO:") { // Work Coordinate Offset
          QStringList l = s.right(s.length() - 4).left(s.length() - 5).split(",");
          this->state.wco = {l[0].toDouble(), l[1].toDouble(),
                             l[2].toDouble()};
        } else if (s.left(2) == "A:") { // Alarm
          continue;
        } else {
          assert(false);
        }
      }

      this->findChild<QLineEdit *>("x_mcs")->setText(QString::number(this->state.xyz_mcs[0]));
      this->findChild<QLineEdit *>("y_mcs")->setText(QString::number(this->state.xyz_mcs[1]));
      this->findChild<QLineEdit *>("z_mcs")->setText(QString::number(this->state.xyz_mcs[2]));
      
      if (this->state.wco[0] != -9999) { //Fixy
        if (this->findChild<QPushButton *>("mode_button")
            ->text() == "DISABLED") {
          this->findChild<QLineEdit *>("x_wcs")->setText(QString::number(this->state.xyz_mcs[0] - this->state.wco[0]));
          this->findChild<QLineEdit *>("y_wcs")->setText(QString::number(this->state.xyz_mcs[1] - this->state.wco[1]));
          this->findChild<QLineEdit *>("z_wcs")->setText(QString::number(this->state.xyz_mcs[2] - this->state.wco[2]));
        }
      }
      continue;
    }

    if (read_string.left(1) == "[" and read_string.right(1) == "]") {
      //TODO: parse and make more human readable
      this->findChild<QStatusBar *>("statusbar")
        ->showMessage(this->state.status_str[this->state.status] + " | " + read_string);
       continue;
    }

    //assert(false);    
  }
}


void MainWindow::on_reset_view_clicked() {
  this->camera->lens()->setPerspectiveProjection(5, 1.3, 0.1, 10000);
  this->camera->setPosition(QVector3D(-1000., 1000., 1000.0));
  this->camera->setViewCenter(QVector3D(0, 0, 0));
  this->camera->setUpVector(QVector3D(0, 0, 1));
}

void MainWindow::writeData(const QByteArray &data) {
  //qDebug() << data;
  p->write(data);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  p = new QSerialPort();
  p->setPortName("/dev/ttyUSB0");
  p->setBaudRate(QSerialPort::Baud115200);
  p->setDataBits(QSerialPort::Data8);
  p->setParity(QSerialPort::NoParity);
  p->setStopBits(QSerialPort::OneStop);
  p->setFlowControl(QSerialPort::NoFlowControl);
  // connect(p, &QSerialPort::errorOccured, this, &MainWindow::handleError);
  connect(p, &QSerialPort::readyRead, this, &MainWindow::readLineData);
  if (p->open(QIODevice::ReadWrite)) {
  } else {
    assert(false);
  }

  Qt3DExtras::Qt3DWindow *view = new Qt3DExtras::Qt3DWindow();
  this->rootEntity = new Qt3DCore::QEntity;

  this->camera = view->camera();

  camera->lens()->setPerspectiveProjection(5, 1.3, 0.1, 10000);
  camera->setPosition(QVector3D(-1000., 1000., 1000.0));
  camera->setViewCenter(QVector3D(0, 0, 0));
  camera->setUpVector(QVector3D(0, 0, 1));
  // For camera controls
  Qt3DExtras::QOrbitCameraController *camController =
      new Qt3DExtras::QOrbitCameraController(rootEntity);
  camController->setLinearSpeed(5000.0f);
  camController->setLookSpeed(1800.0f);
  camController->setCamera(camera);
  view->setRootEntity(rootEntity);
  this->findChild<QLayout *>("HOLDER")->replaceWidget(
      this->findChild<QWidget *>("placeholder"),
      QWidget::createWindowContainer(view));

  add_xyz_to_scene(rootEntity);

  for (auto dir : {"x", "y", "z"}) {
    this->findChild<QLineEdit *>(QString(dir) + "_wcs")
        ->setValidator(new QDoubleValidator(-1000, 1000, 3, this));
    this->findChild<QLineEdit *>(QString(dir) + "_wcs")->setEnabled(false);
    this->findChild<QLineEdit *>(QString(dir) + "_mcs")->setEnabled(false);
  }

  this->bg1 = new QButtonGroup();
  this->bg2 = new QButtonGroup();
  this->bg1->addButton(this->findChild<QRadioButton *>("jog_set_1"));
  this->bg1->addButton(this->findChild<QRadioButton *>("jog_set_2"));
  this->bg1->addButton(this->findChild<QRadioButton *>("jog_set_3"));
  this->bg1->addButton(this->findChild<QRadioButton *>("jog_set_4"));
  this->bg2->addButton(this->findChild<QRadioButton *>("goto_button"));
  this->bg2->addButton(this->findChild<QRadioButton *>("set_button"));
  this->bg2->addButton(this->findChild<QRadioButton *>("show_button"));

  QTimer *request_data_timer = new QTimer(this);
  this->counter = 0;
  connect(request_data_timer, &QTimer::timeout, this, &MainWindow::request_data);
  request_data_timer->start(REQUEST_DATA_MS);

  QTimer *run_gcode_timer = new QTimer(this);
  connect(run_gcode_timer, &QTimer::timeout, this, &MainWindow::send_gcode);
  run_gcode_timer->start(RUN_GCODE_MS);

  for (QWidget *child : this->findChildren<QWidget *>()) {
    child->setStyleSheet(child->styleSheet()+"font: 700 11pt \"DejaVu Sans Mono\";");
  }
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::request_data() {
  counter += 1;
  if (int(counter) % 2 == 0) {
    this->writeData("?");
  } else {
    this->response_count -= 1;
    this->writeData("$G\n");
  }
}

QPair<QString, QString> MainWindow::ds_helper() {
  QString distance, speed;
  if (this->findChild<QRadioButton *>("jog_set_1")->isChecked()) {
    distance = this->findChild<QLineEdit *>("jog_d_1")->text();
    speed = this->findChild<QLineEdit *>("jog_s_1")->text();
  } else if (this->findChild<QRadioButton *>("jog_set_2")->isChecked()) {
    distance = this->findChild<QLineEdit *>("jog_d_2")->text();
    speed = this->findChild<QLineEdit *>("jog_s_2")->text();
  } else if (this->findChild<QRadioButton *>("jog_set_3")->isChecked()) {
    distance = this->findChild<QLineEdit *>("jog_d_3")->text();
    speed = this->findChild<QLineEdit *>("jog_s_3")->text();
  } else if (this->findChild<QRadioButton *>("jog_set_4")->isChecked()) {
    distance = this->findChild<QLineEdit *>("jog_d_4")->text();
    speed = this->findChild<QLineEdit *>("jog_s_4")->text();
  } else {
    assert(false);
  }
  return QPair<QString, QString>(distance, speed);
}

void MainWindow::on_feed_hold_clicked() {
  QByteArray q = "!";
  this->writeData(q);
}

void MainWindow::on_cycle_start_clicked() {
  QByteArray q = "~";
  this->writeData(q);
}

void MainWindow::on_xp_jog_clicked() {
  QPair<QString, QString> ds = ds_helper();
  QString str = QString("$J=G91 G21 X%1 F%2\n").arg(ds.first, ds.second);
  QByteArray q = str.toLocal8Bit();
  this->writeData(q);
}
void MainWindow::on_xm_jog_clicked() {
  QPair<QString, QString> ds = ds_helper();
  QString str = QString("$J=G91 G21 X-%1 F%2\n").arg(ds.first, ds.second);
  QByteArray q = str.toLocal8Bit();
  this->writeData(q);
}
void MainWindow::on_yp_jog_clicked() {
  QPair<QString, QString> ds = ds_helper();
  QString str = QString("$J=G91 G21 Y%1 F%2\n").arg(ds.first, ds.second);
  QByteArray q = str.toLocal8Bit();
  this->writeData(q);
}
void MainWindow::on_ym_jog_clicked() {
  QPair<QString, QString> ds = ds_helper();
  QString str = QString("$J=G91 G21 Y-%1 F%2\n").arg(ds.first, ds.second);
  QByteArray q = str.toLocal8Bit();
  this->writeData(q);
}
void MainWindow::on_zp_jog_clicked() {
  QPair<QString, QString> ds = ds_helper();
  QString str = QString("$J=G91 G21 Z%1 F%2\n").arg(ds.first, ds.second);
  QByteArray q = str.toLocal8Bit();
  this->writeData(q);
}
void MainWindow::on_zm_jog_clicked() {
  QPair<QString, QString> ds = ds_helper();
  QString str = QString("$J=G91 G21 Z-%1 F%2\n").arg(ds.first, ds.second);
  QByteArray q = str.toLocal8Bit();
  this->writeData(q);
}

void MainWindow::on_actionExit_triggered() { QApplication::quit(); }

void MainWindow::on_load_nc_clicked() {
  QString filename = QFileDialog::getOpenFileName(this, tr("Open"), "/home/", tr("NC files (*.nc)"));
  QFile file(filename);
  if (file.exists() and filename.right(3) == ".nc") {
    QTextStream stream(&file);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      auto text = stream.readAll();
      this->findChild<QTextBrowser *>("nc_browser")->setText(text);

      QStringList lines = text.split("\n");
      this->findChild<QTextBrowser *>("tool_list")->setText(lines.filter("(T").join("\n"));
      
    } else {
      qDebug().nospace() << "nc file not found";
    }
  }

  //(T# 
  QString text = this->findChild<QTextBrowser *>("nc_browser")->toPlainText();
  QStringList lines = text.split("\n");
  this->gcode_queue = lines;

}

std::vector<QVector3D> MainWindow::parse_gcode(QStringList lines) {

  std::vector<QVector3D> ret_arr{};

  QVector3D loc(0,0,0);//this->state.xyz_mcs[0],
  //                this->state.xyz_mcs[1],
  //              this->state.xyz_mcs[2]);
  
  for (QString line : lines) {
    // Ignore lines starting with (, g90 absolute, g91 incremental, g94
    // units/minute (err on g93), g17 XY, G18 XZ, G19 YZ plane selection, G21 mm
    // (g20 is inches), T4 M6 (toolchange to T4), SXXX M3 (spindle speed), g1
    // feed, g0 rapid, g2,g3 clockwise and cc arcs, G54 WCS;
    if (!line.isEmpty()) {
      if (line.startsWith("(")) {
      }
      QStringList x = line.split(" ");
      QVector3D tmp(-9999, -9999, -9999);
      for (QString word : x) {
        if (word.startsWith("X")) {
          tmp[0] = word.right(word.length() - 1).toDouble();
        }
        if (word.startsWith("Y")) {
          tmp[1] = word.right(word.length() - 1).toDouble();
        }
        if (word.startsWith("Z")) {
          tmp[2] = word.right(word.length() - 1).toDouble();
        }
      }
      for (int i = 0; i < 3; ++i) {
        if (tmp[i] == -9999) {
          tmp[i] = loc[i];
        }
      }
      ret_arr.push_back(tmp);
      loc = tmp;
    }
  }
  return ret_arr;
}

void MainWindow::on_nc_preview_clicked() {
  QString text = this->findChild<QTextBrowser *>("nc_browser")->toPlainText();
  // qDebug() << text;
  QStringList lines = text.split("\n");
  std::vector<QVector3D> gc = parse_gcode(lines);
  for (long unsigned int i = 0; i < gc.size() - 2; i = i + 2) {
    QVector3D a = gc[i];
    QVector3D b = gc[i + 10];
    add_line_to_scene(this->rootEntity, a, b);
  }
}

void MainWindow::on_load_obj_clicked() {

  QString filename = QFileDialog::getOpenFileName(this, tr("Open"), "/home/", tr("Obj files (*.obj)"));
  if (filename.right(4).compare(".obj") == 0 and
      QFile(filename).exists()) {
    add_obj_to_scene(this->rootEntity, filename);
  } else {
    qDebug().nospace() << "Obj not found.";
  }
}

void MainWindow::on_clear_obj_clicked() {
  QList<Qt3DCore::QEntity *> childs =
      this->rootEntity->findChildren<Qt3DCore::QEntity *>();
  for (Qt3DCore::QEntity *c : childs) {
    if (c->objectName().left(3) == "Obj" or c->objectName().left(4) == "Path") {
      delete c;
    }
  }
}

void MainWindow::on_set_button_clicked() {
  this->findChild<QPushButton *>("mode_button")->setText("SET");
  this->findChild<QPushButton *>("mode_button")->setEnabled(true);
  for (auto dir : {"x", "y", "z"}) {
    this->findChild<QLineEdit *>(QString(dir) + "_wcs")->setEnabled(true);
    this->findChild<QLineEdit *>(QString(dir) + "_mcs")->setEnabled(false);
  }
}

void MainWindow::on_mode_button_clicked() {
  QString text = this->findChild<QPushButton *>("mode_button")->text();
  if (text.compare("GOTO") == 0) {
    QString x = this->findChild<QLineEdit *>("x_wcs")->text();
    QString y = this->findChild<QLineEdit *>("y_wcs")->text();
    QString z = this->findChild<QLineEdit *>("z_wcs")->text();
    QString feed = ds_helper().second;
    QString str = QString("$J=G90 G21 X%1 Y%2 Z%3 F%4\n").arg(x, y, z, feed);
    this->writeData(str.toLocal8Bit());
  } else if (text.compare("SET") == 0) {
    QString x = this->findChild<QLineEdit *>("x_wcs")->text();
    QString y = this->findChild<QLineEdit *>("y_wcs")->text();
    QString z = this->findChild<QLineEdit *>("z_wcs")->text();
    QString str = QString("G10 L20 X%1 Y%2 Z%3\n").arg(x, y, z);
    this->writeData(str.toLocal8Bit());
  } else if (text.compare("DISABLED") == 0) {
    return;
  } else {
    assert(false);
  }
}

void MainWindow::on_goto_button_clicked() {
  this->findChild<QPushButton *>("mode_button")->setText("GOTO");
  this->findChild<QPushButton *>("mode_button")->setEnabled(true);
  for (auto dir : {"x", "y", "z"}) {
    this->findChild<QLineEdit *>(QString(dir) + "_wcs")->setEnabled(true);
    this->findChild<QLineEdit *>(QString(dir) + "_mcs")->setEnabled(false);
  }
}

void MainWindow::on_show_button_clicked() {
  this->findChild<QPushButton *>("mode_button")->setText("DISABLED");
  this->findChild<QPushButton *>("mode_button")->setEnabled(false);
  for (auto dir : {"x", "y", "z"}) {
    this->findChild<QLineEdit *>(QString(dir) + "_wcs")->setEnabled(false);
    this->findChild<QLineEdit *>(QString(dir) + "_mcs")->setEnabled(false);
  }
}
