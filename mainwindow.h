#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QButtonGroup>
#include <QMainWindow>
#include <QSerialPort>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>

enum m_status { UNINITIALIZED, IDLE, JOG, CHECK, HOLD0, HOLD1, RUN, ALARM };

struct MachineState {
  m_status status = UNINITIALIZED;
  const QString status_str[8]{"UNINITIALIZED", "IDLE", "JOG", "CHECK", "HOLD0", "HOLD1", "RUN", "ALARM"};
  std::array<double, 3> xyz_mcs;
  std::array<double, 2> fs;
  std::array<double, 3> ov;
  std::array<double, 3> wco = {-9999, -9999, -9999};
};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:

  void request_data();

  void on_actionExit_triggered();

  void on_load_nc_clicked();

  void on_reset_view_clicked();

  void on_nc_preview_clicked();

  void on_load_obj_clicked();

  void on_clear_obj_clicked();

  void on_set_button_clicked();

  void on_goto_button_clicked();

  std::vector<QVector3D> parse_gcode(QStringList lines);

  void on_show_button_clicked();

  void on_input_command_returnPressed();

  void on_xp_jog_clicked();
  void on_xm_jog_clicked();
  void on_yp_jog_clicked();
  void on_ym_jog_clicked();
  void on_zp_jog_clicked();
  void on_zm_jog_clicked();

  void on_play_button_clicked();
  void on_pause_button_clicked();
  void on_stop_button_clicked();
  void on_reset_button_clicked();

  void on_mode_button_clicked();

  void on_feed_hold_clicked();
  void on_cycle_start_clicked();

  void on_feed_cp_button_clicked();
  void on_feed_cm_button_clicked();
  void on_feed_reset_button_clicked();
  void on_speed_cp_button_clicked();
  void on_speed_cm_button_clicked();
  void on_speed_reset_button_clicked();
  
  void send_gcode();
  
  void readLineData();
  void writeData(const QByteArray &data);
  QPair<QString, QString> ds_helper();

private:
  Ui::MainWindow *ui;
  double counter;
  Qt3DCore::QEntity *rootEntity;
  QButtonGroup *bg1, *bg2;
  QSerialPort *p;
  MachineState state;
  Qt3DRender::QCamera *camera;
  
  int response_count = 0;
  QStringList gcode_queue{};
  bool sending_gcode = false;
  
};
#endif // MAINWINDOW_H
