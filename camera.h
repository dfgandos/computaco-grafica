#ifndef CAMERA_H
#define CAMERA_H

#include <QVector3D>

class Camera
{
public:
    Camera();
    QVector3D eye;
    QVector3D at;
    QVector3D up;
};

#endif // CAMERA_H
