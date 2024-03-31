#include "attendancewin.h"
#include "selectwin.h"

#include <QApplication>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <opencv.hpp>

#include "registerwin.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<cv::Mat>("cv::Mat&");
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<int64_t>("int64_t");


//    RegisterWin ww;
//    ww.show();

    //【连接数据库】
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE"); //添加QSQLITE数据库。如果是QMYSQL数据库的话要自己编译。
    //【设置数据库的名称】
    db.setDatabaseName("server.db");
    //【打开数据库】
    if(!db.open())
    {
        qDebug()<<db.lastError().text();
        return -1;
    }

    //【如果打开成功，创建员工信息表employee】（将多个字符串连接成一个完整的 SQL 查询语句）（faceID不能重复！）
    QString createsql = "create table if not exists employee(employeeID integer primary key autoincrement,"
                        "name varchar(256),"
                        "sex varchar(32),"
                        "birthday text,"
                        "address text,"
                        "phone text,"
                        "faceID integer unique,"
                        "headfile text)";
    QSqlQuery query; //创建查询对象
    if(!query.exec(createsql))
    {
       qDebug()<<query.lastError().text();
       return -1;
    }

    //【如果打开成功，创建考勤表格attendance】
    createsql = "create table if not exists attendance(attendaceID integer primary key autoincrement,"
                "employeeID integer,"
                "attendaceTime TimeStamp NOT NULL DEFAULT(datetime('now','localtime')))";
    if(!query.exec(createsql))
    {
       qDebug()<<query.lastError().text();
       return -1;
    }

    AttendanceWin w;
    w.show();

//    SelectWin sw;
//    sw.show();
    return a.exec();
}
