#ifndef ATTENDANCEWIN_H
#define ATTENDANCEWIN_H

#include "qfaceobject.h"

#include <QMainWindow>

#include <QTcpSocket>
#include <QTcpServer>

#include <QSqlTableModel>
#include <QSqlRecord>

QT_BEGIN_NAMESPACE
namespace Ui { class AttendanceWin; }
QT_END_NAMESPACE

class AttendanceWin : public QMainWindow
{
    Q_OBJECT

public:
    AttendanceWin(QWidget *parent = nullptr);
    ~AttendanceWin();

signals:
    void query(cv::Mat& image); //定义 查询信号query



protected slots:
    void accept_client();
    void read_data();

    void recv_faceid(int64_t faceid);   // 槽函数声明

private:
    Ui::AttendanceWin *ui;

    QTcpServer mserver;
    QTcpSocket *msocket;

    quint64 bsize;  //判断服务器接收的数据的大小

    QFaceObject fobj;   //人脸识别对象

    QSqlTableModel model;   //

};
#endif // ATTENDANCEWIN_H
