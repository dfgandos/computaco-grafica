#include "light.h"

Light::Light()
{
    position    = QVector4D(3.0, 3.0, 3.0, 0.0);
    ambient     = QVector4D(0.1,0.1,0.1,1.0);
    diffuse     = QVector4D(0.9,0.9,0.9,1.0);
    specular    = QVector4D(0.9,0.9,0.9,1.0);
}
