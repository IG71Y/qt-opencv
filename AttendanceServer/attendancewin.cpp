#include "attendancewin.h"
#include "ui_attendancewin.h"

#include <QDateTime>
#include <QSqlRecord>

#include <QThread>
#include <opencv.hpp>

#include <QSqlQuery>
#include <QSqlError>

AttendanceWin::AttendanceWin(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AttendanceWin)
{
    ui->setupUi(this);

    //当连接客户端时，会发送newconnection
    connect(&mserver, &QTcpServer::newConnection, this, &AttendanceWin::accept_client);

    //监听，启动服务器
    mserver.listen(QHostAddress::Any,9999);

    //初始化接收数据的大小
    bsize = 0;

    //给sql模型绑定数据库表
    model.setTable("employee");


    //【【【【【【【线程优化人脸识别】】】】】】】】】

    //创建一个线程对象thread
    QThread *thread = new QThread();
    //把QFaceObject对象移动到thread线程中执行
    fobj.moveToThread(thread);
    //启动线程
    thread->start();

    //【触发查询信号>fobj调用查询函数】
    connect(this, &AttendanceWin::query, &fobj, &QFaceObject::face_query);

    //【【关联QFaceObject对象里面的send_faceid信号>接收查询结果】】
    connect(&fobj, &QFaceObject::send_faceid, this, &AttendanceWin::recv_faceid);

}

AttendanceWin::~AttendanceWin()
{
    delete ui;
}

    //【【4.服务器与客户端连接】】
//服务器接受客户端连接
void AttendanceWin::accept_client()
{
    //mserver获取与客户端通信的套接字msocket
    msocket = mserver.nextPendingConnection();

    //当客户端有数据到达时，msocket会发送readyRead信号
    connect(msocket, &QTcpSocket::readyRead, this, &AttendanceWin::read_data);
}

//【【【【【6.服务器读取客户端发送的数据并显示到界面】】】】
void AttendanceWin::read_data()
{
    QDataStream stream(msocket); //把msocket绑定到数据流
//    stream.setVersion(QDataStream::Qt_5_12);

    //1.服务器判断接收的数据大小
    if(bsize == 0){
        if(msocket->bytesAvailable()<(qint64)sizeof(bsize)) return ;//判读数据够不够8个字节
        //if(msocket->bytesAvailable()<(qint32)sizeof(bsize)) return ;
        stream>>bsize;//采集数据的长度
    }

    //2.服务器判读数据是否发送完毕（够不够8个字节）
    if(msocket->bytesAvailable()<bsize)   //说明数据还没有发送完成，返回继续等待
    {
        return ;
    }

    //3.服务器开始读取数据
    QByteArray data;
    stream>>data;
    bsize = 0;
    if(data.size() == 0) return;  //如果服务器读取数据的大小为0，说明没有读取到数据

    //4.服务器显示接收到的图片（TabWidget1）
    QPixmap mmp;
    mmp.loadFromData(data,"jpg");
    mmp = mmp.scaled(ui->picLb->size());
    ui->picLb->setPixmap(mmp);

    //5.服务器识别人脸
    cv::Mat faceImage;
    std::vector<uchar> decode;
    decode.resize(data.size());
    memcpy(decode.data(),data.data(),data.size());  //把数据流中的数据data复制给decode,  使得decode满足imdecode的输入格式要求。实际上是QByteArray>uchar>imdecode>Mat>face_query>int faceid
    faceImage = cv::imdecode(decode, cv::IMREAD_COLOR); // 即：将接收到的数据转换成face_query要求的Mat输入格式的数据。

//    int faceid = fobj.face_query(faceImage); //fobj.face_query。输入人脸照片，返回人脸ID。消耗资源较多。
    emit query(faceImage);  //输入人脸照片,发送信号>fobj.face_query

}

void AttendanceWin::recv_faceid(int64_t faceid)
{
    //qDebug()<<"00000"<<faceid;
    //从数据库中查询faceid对应的个人信息

    //qDebug()<<"识别到的人脸id:"<<faceid;
    //如果考勤失败（未识别到人脸）
    if(faceid < 0)
    {
        QString sdmsg = QString("{\"employeeID\":\"\",\"name\":\"\",\"department\":\"\",\"time\":\"\"}");
        msocket->write(sdmsg.toUtf8()); //把空数据发送给客户端
        qDebug()<<"未识别到有效人脸，考勤失败！";
        return ;
    }

    //给模型设置过滤器
    model.setFilter(QString("faceID=%1").arg(faceid));
    //查询
    model.select();
    //判断是否查询到数据
    if(model.rowCount() == 1)
    {
        //工号，姓名，部门，时间
        //{employeeID:%1,name:%2,department:软件,time:%3}
        QSqlRecord record = model.record(0);
        //数据打包
        QString sdmsg = QString("{\"employeeID\":\"%1\",\"name\":\"%2\",\"department\":\"软件\",\"time\":\"%3\"}")
                .arg(record.value("employeeID").toString())
                .arg(record.value("name").toString())
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

        //【把考勤数据写入数据库--考勤表attendance】
        QString insertSql = QString("insert into attendance(employeeID) values('%1')").arg(record.value("employeeID").toString());
        QSqlQuery query;    //执行query的对象

        //如果考勤失败（不能插入考勤表）
        if(!query.exec(insertSql))
        {
            QString sdmsg = QString("{\"employeeID\":\"\",\"name\":\"\",\"department\":\"\",\"time\":\"\"}");
            msocket->write(sdmsg.toUtf8()); //把空数据发送给客户端
            qDebug()<<"考勤失败的原因："<<query.lastError().text();
            return ;
        }else
        //如果考勤成功
        {
            msocket->write(sdmsg.toUtf8()); //把打包好的数据发送给客户端
        }
    }
}
