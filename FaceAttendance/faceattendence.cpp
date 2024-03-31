#include "faceattendence.h"
#include "ui_faceattendence.h"
#include <QImage>
#include <QPainter>

#include <QDebug>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>

FaceAttendence::FaceAttendence(QWidget *parent)                                 //有参构造函数调用？
    : QMainWindow(parent)                                                       //初始化列表
    , ui(new Ui::FaceAttendence)
{
    ui->setupUi(this);

    //打开摄像头
    cap.open(0);//dev/video

    //每100毫秒启动一次定时器事件
    startTimer(100);

    //导入级联分类器文件(训练好的脸部特征数据)
    cascade.load("C:/opencv452/etc/haarcascades/haarcascade_frontalface_default.xml");

    //QTcpSocket当断开连接的时候disconnected信号
    connect(&msocket,&QTcpSocket::disconnected,this, &FaceAttendence::start_connect);

    //与服务器连接成功会发送connected
    connect(&msocket,&QTcpSocket::connected,this, &FaceAttendence::stop_connect);

    //定时器连接服务器
    connect(&mtimer, &QTimer::timeout,this,&FaceAttendence::timer_connect);

    //启动定时器
    mtimer.start(2000); //定时器每2s钟连接一次，直到连接成功就不在连接

    //【【【【关联接收数据的槽函数】】】】】】
    connect(&msocket, &QTcpSocket::readyRead,this, &FaceAttendence::recv_data);

    flag =0;

    ui->widgetLb->hide();
}

FaceAttendence::~FaceAttendence()
{
    delete ui;
}

//定时器事件函数（定时器启动以后执行）
//重写了 timerEvent 函数。这意味着当一个定时器事件发生时，这个函数会被调用。
void FaceAttendence::timerEvent(QTimerEvent *e)
{
    Mat srcImage;   //OpenCV采集Mat格式数据
    if(cap.grab())  cap.read(srcImage); //读取一帧数据
    cv::resize(srcImage,srcImage,Size(480,480));    //缩放图片，把图片大小设与显示窗口一样大
    Mat grayImage;
    cvtColor(srcImage, grayImage, COLOR_BGR2GRAY);  //Mat图转灰度图，加快人脸检测速度

    //【【2.终端opencv实时检测跟踪人脸，并发送数据到服务器】】
    std::vector<Rect> faceRects;    //faceRects 被检测物体的矩形框向量(容器)
    cascade.detectMultiScale(grayImage, faceRects); //检测人脸(使用灰度图提高速度)
    //终端opencv实时检测跟踪人脸
    if(faceRects.size()>0 && flag>=0)
    {
        Rect rect = faceRects.at(0); //第一个人脸的矩形框
        //rectangle(srcImage,rect,Scalar(0,0,255));   //标注人脸框
        ui->headpicLb->move(rect.x,rect.y); //如果检测到人脸，把人脸框（headpicLb人脸框图片--QLabel）移动到人脸上

        if(flag > 2)
        {
            //【5.客户端压缩数据发送到服务器】
            //客户端压缩数据
            std::vector<uchar> buf;
            cv::imencode(".jpg",srcImage,buf);  //把OpenCV的Mat格式数据（srcImage）转化为能够网络发送的QbyteArray数据，先用imencode把srcImage编码成jpg格式（为了压缩数据大小）
            QByteArray byte((const char*)buf.data(),buf.size()); //把uchar类型的（vector动态数组）强转成QByteArray类型的（byte字节数组）
            quint64 backsize = byte.size(); //获取byte大小
            //quint32 backsize = byte.size(); //获取byte大小
            QByteArray sendData;            //要发送的数据sendData
            QDataStream stream(&sendData,QIODevice::WriteOnly); //创建数据流（数据，权限）
//            stream.setVersion(QDataStream::Qt_5_12);
            stream<<backsize<<byte; //把数据放进数据流

            //客户端发送数据
            msocket.write(sendData);
            flag = -2;
            qDebug()<<"考勤客户端向服务器发送1次人脸数据！";

            //保存终端发送给服务器的人脸数据
            faceMat = srcImage(rect);   //矩形框中的人脸
            imwrite("./face.jpg",faceMat);  //保存矩形框中的人脸到本地
        }
        flag++;
    }

    if(faceRects.size() == 0)
    {
        ui->headpicLb->move(120,120);   //如果没有检测到人脸，把人脸框移动到中心位置(120,120)
        ui->widgetLb->hide();           //如果没有检测到人脸，就不显示“考勤成功”标识
        flag=0;
    }

    //【【1.摄像头数据在界面上实时显示】】

    if(srcImage.data == nullptr) return;
    cvtColor(srcImage,srcImage, COLOR_BGR2RGB); //把opencv的Mat格式的图像数据（BGR）转Qt里面的QImage(RGB)
    QImage image(srcImage.data, srcImage.cols, srcImage.rows, srcImage.step1(), QImage::Format_RGB888);
    QPixmap mmp = QPixmap::fromImage(image);

    ui->videoLb->setPixmap(mmp);
}

    //【【【【考勤终端接收服务器发送的数据】】】
void FaceAttendence::recv_data()
{
    QByteArray array = msocket.readAll();   //接收数据类型为QByteArray
    qDebug()<<"考勤终端接收到服务器发送的json数据为："<<array;

    //Json数据解析
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(array,&err);

    //如果json数据错误
    if(err.error != QJsonParseError::NoError)
    {
        qDebug()<<"考勤终端接收的json数据错误！";
        return;
    }

    //如果json数据没有问题，则解析json数据
    QJsonObject obj = doc.object();     //obj对象的格式实际是：{employeeID:%1,name:%2,department:软件,time:%3}
    QString employeeID  = obj.value("employeeID").toString();
    QString name        = obj.value("name").toString();
    QString department  = obj.value("department").toString();
    QString timestr     = obj.value("time").toString();

    //终端显示员工考勤信息（工号，姓名，部门，考勤时间）
    ui->numberEdit      ->setText(employeeID);
    ui->nameEdit        ->setText(name);
    ui->departmentEdit  ->setText(department);
    ui->timeEdit        ->setText(timestr);
    ui->headLb->setStyleSheet("border-radius:75px;border-image: url(./face.jpg);");     //通过样式来显示图片
    ui->widgetLb->show();   //如果考勤成功，就显示“考勤成功”标识
}

    //【【【3.终端TCP连接服务器】】】
void FaceAttendence::timer_connect()
{
    //连接服务器
    msocket.connectToHost("10.100.156.151",9999);
    qDebug()<<"考勤终端正在连接服务器";
}

void FaceAttendence::stop_connect()
{
    mtimer.stop();  //成功连接服务器以后，停止定时器
    qDebug()<<"考勤终端成功连接服务器";
}

void FaceAttendence::start_connect()
{
    mtimer.start(1000);//启动定时器
    qDebug()<<"考勤终端与服务器断开连接";
}

