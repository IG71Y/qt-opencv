#ifndef FACEATTENDENCE_H
#define FACEATTENDENCE_H

#include <QMainWindow>
#include <opencv.hpp>
#include <QTcpSocket>
#include <QTimer>
using namespace cv;
using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class FaceAttendence; }
QT_END_NAMESPACE

class FaceAttendence : public QMainWindow
{
    Q_OBJECT

public:
    FaceAttendence(QWidget *parent = nullptr);                                                      //拷贝构造函数？
    ~FaceAttendence();

    //定时器事件，接受一个 QTimerEvent 类型的指针作为参数
    void timerEvent(QTimerEvent *e);

protected slots:
    void recv_data();   //终端接收服务器信息 槽函数

private slots:
    void timer_connect();
    void stop_connect();
    void start_connect();

private:
    Ui::FaceAttendence *ui;

    VideoCapture cap;    //摄像头

    cv::CascadeClassifier cascade;    //haar--级联分类器对象

    QTcpSocket msocket;    //创建网络套接字，创建定时器
    QTimer mtimer;      //创建定时器

    int flag;   //标志是否是同一个人脸进入到识别区域

    cv::Mat faceMat;     //客户端向服务器发送的人脸数据
};
#endif // FACEATTENDENCE_H
