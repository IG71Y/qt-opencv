#include "registerwin.h"
#include "ui_registerwin.h"

#include <QFileDialog>

#include <qfaceobject.h>

#include <QSqlTableModel>
#include <QSqlRecord>

#include <QMessageBox>

#include <QDebug>

RegisterWin::RegisterWin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegisterWin)
{
    ui->setupUi(this);
}

RegisterWin::~RegisterWin()
{
    delete ui;
}

//【【重置】 按钮-槽函数】】
void RegisterWin::on_resetBt_clicked()
{
    //清空数据
    ui->nameEdit->clear();
    ui->birthdayEdit->setDate(QDate::currentDate());
    ui->addressEdit->clear();
    ui->phoneEdit->clear();
    ui->picFileEdit->clear();
}

//【【【【【注册】 按钮-槽函数】】】】】
void RegisterWin::on_registerBt_clicked()
{
    //【1.通过添加人脸照片，使用qfaceobject模块的face_register函数，得到faceID】
    QFaceObject  faceobj;
    cv::Mat image = cv::imread(ui->picFileEdit->text().toUtf8().data());
    //人脸注册
    int faceID = faceobj.face_register(image);  //【【【【【【【使用qfaceobject模块的face_register函数，得到faceID】】】】】】
    qDebug()<<"添加的人脸照片的faceID是："<<faceID;

    //把员工头像保存到一个固定路径下
    QString headfile = QString("./data/%1.jpg").arg(QString(ui->nameEdit->text().toUtf8().toBase64())); //设置保存路径。名字的Base64编码作为图片的名称
    //QString headfile = QString("./data/%1.jpg").arg(QString(ui->nameEdit->text()));
    cv::imwrite(headfile.toUtf8().data(), image);   //保存员工头像图像


    //【2.把个人信息存储到数据库--员工信息表employee】
    QSqlTableModel model;   //创建数据库表模型
    model.setTable("employee"); //绑定表格
    QSqlRecord record = model.record();
    //把数据录入表格
    record.setValue("name",ui->nameEdit->text());
    record.setValue("sex",ui->mrb->isChecked()?"男":"女");
    record.setValue("birthday", ui->birthdayEdit->text());
    record.setValue("address",ui->addressEdit->text());
    record.setValue("phone",ui->phoneEdit->text());
    record.setValue("faceID", faceID);
    record.setValue("headfile",headfile); //headfile：数据库员工头像的保存路径
    //把记录插入到数据库表格中
    bool ret = model.insertRecord(0,record);

    //【3.如果插入提示注册成功，如果没有插入成功提示注册失败】
    if(ret)
    {
        QMessageBox::information(this,"注册提示","员工注册成功，员工信息已上传到数据库！");
        //提交。把数据上传到数据库中
        model.submitAll();
    }else
    {
        QMessageBox::information(this,"注册提示","员工注册失败，员工信息未上传到数据库！");
    }
}

//【【添加头像】 按钮-槽函数】】
void RegisterWin::on_addpicBt_clicked()
{
    //通过文件对话框，选择头像图片路径
    QString filepath = QFileDialog::getOpenFileName(this);
    ui->picFileEdit->setText(filepath); //显示图片路径

    //显示图片
    QPixmap mmp(filepath);
    mmp = mmp.scaledToWidth(ui->headpicLb->width());
    ui->headpicLb->setPixmap(mmp);
}

//【打开摄像头】 按钮-槽函数】
void RegisterWin::on_videoswitchBt_clicked()
{
    if(ui->videoswitchBt->text() == "打开摄像头")
    {
        if(cap.open(0))
        {
            timerid = startTimer(100); //启动定时器事件
            ui->videoswitchBt->setText("关闭摄像头");    //打开摄像头后，将按钮显示为关闭摄像头
        }
    }else
    {
        killTimer(timerid); //关闭定时器事件
        ui->videoswitchBt->setText("打开摄像头");
        cap.release();//关闭摄像头
    }
}

//【【获取摄像头数据并且显示在界面上】】
void RegisterWin::timerEvent(QTimerEvent *e)
{
if(cap.isOpened())
{
    cap>>image;
    if(image.data == nullptr) return;
}

// 把数据显示在QT上面（Mat数据转成QImage）
cv::Mat rgbImage;
cv::cvtColor(image,rgbImage,cv::COLOR_BGR2RGB);
QImage qImg(rgbImage.data, rgbImage.cols, rgbImage.rows,rgbImage.step1(), QImage::Format_RGB888);   //Mat数据转成QImage
//在Qt界面上显示摄像头数据
QPixmap mmp=QPixmap::fromImage(qImg);
mmp = mmp.scaledToWidth(ui->headpicLb->width());
ui->headpicLb->setPixmap(mmp);

}

//【拍照】 按钮-槽函数】
void RegisterWin::on_cameraBt_clicked()
{

    //把员工头像保存到一个固定路径下
    QString headfile = QString("./data/%1.jpg").arg(QString(ui->nameEdit->text().toUtf8().toBase64())); //设置保存路径。名字的Base64编码作为图片的名称
    ui->picFileEdit->setText(headfile); //显示图片路径
    cv::imwrite(headfile.toUtf8().data(), image);
    //保存员工图片后，
    killTimer(timerid); //关闭定时器事件
    ui->videoswitchBt->setText("打开摄像头");
    cap.release();//关闭摄像头
}
