#ifndef QFACEOBJECT_H
#define QFACEOBJECT_H

#include <QObject>
#include <seeta/FaceEngine.h>
#include <opencv.hpp>

//人脸数据存储， 人脸检测，人脸识别
class QFaceObject : public QObject
{
    Q_OBJECT
public:
    explicit QFaceObject(QObject *parent = nullptr);    //  explicit关键字
    ~QFaceObject();

public slots:
    int64_t face_register(cv::Mat& faceImage);  //人脸注册。输入是终端的Mat图片。输出是int64_t的人脸id。 传递了对象的引用
    int  face_query(cv::Mat& faceImage);        //人脸查询。输入是终端的Mat图片。输出是数据库中int类型的人脸id

signals:
    void send_faceid(int64_t faceid);   //（服务器如果查询到人脸） 创建 发送信号
private:
    seeta::FaceEngine  *fengineptr;

};

#endif // QFACEOBJECT_H
