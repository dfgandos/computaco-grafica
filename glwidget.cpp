#include "glwidget.h"
#include <fstream>
#include <iostream>
#include <QtGlobal>

GLWidget::GLWidget(QWidget *parent) : QGLWidget(parent)
{
    vertices = NULL;
    normals = NULL;
    texCoords = NULL;
    tangents = NULL;
    indices = NULL;

    vboVertices = NULL;
    vboNormals = NULL;
    vboTexCoords = NULL;
    vboTangents = NULL;
    vboIndices = NULL;

    shaderProgram = NULL;
    vertexShader = NULL;
    fragmentShader = NULL;
    currentShader = 0;

    zoom = 0.0;
}

GLWidget::~GLWidget()
{
    destroyVBOs();
    destroyShaders();
}

void GLWidget::toggleBackgroundColor(bool toBlack)
{
    if (toBlack)
        glClearColor(0, 0, 0, 1);
    else
        glClearColor(1, 1, 1, 1);

    updateGL();
}

void GLWidget::initializeGL()
{
   glEnable(GL_DEPTH_TEST);
   QImage texColor= QImage(":/textures/bricksDiffuse.png") ;
   QImage texNormal= QImage(":/textures/bricksNormal.png") ;

   texture.initializeOpenGLFunctions();
   texture.glActiveTexture(GL_TEXTURE0);
   texID[0] = bindTexture(texColor);
   texture.glActiveTexture(GL_TEXTURE1);
   texID[1] = bindTexture(texNormal);

   connect(&timer,SIGNAL(timeout()),this,SLOT(animate()));
   timer.start (0) ;
}

void GLWidget:: resizeGL(int width, int height)
{
    glViewport (0, 0, width , height );
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(60.0 ,static_cast <qreal >( width ) / static_cast <qreal >( height ), 0.1 , 20.0);
    trackBall . resizeViewport(width , height );
    updateGL ();
}

void GLWidget:: paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT |  GL_DEPTH_BUFFER_BIT);

    if (!vboVertices)
        return;

    modelViewMatrix.setToIdentity();
    modelViewMatrix.lookAt(camera.eye,camera.at,camera.up);
    modelViewMatrix.translate(0, 0, zoom);
    modelViewMatrix.rotate(trackBall.getRotation());

    shaderProgram->bind();
    shaderProgram->setUniformValue("modelViewMatrix", modelViewMatrix);
    shaderProgram->setUniformValue("projectionMatrix", projectionMatrix);
    shaderProgram->setUniformValue("normalMatrix", modelViewMatrix.normalMatrix());

    QVector4D ambientProduct = light.ambient * material.ambient;
    QVector4D diffuseProduct = light.diffuse * material.diffuse;
    QVector4D specularProduct = light.specular * material.specular;

    shaderProgram->setUniformValue("lightPosition", light. position);
    shaderProgram->setUniformValue("ambientProduct",ambientProduct);
    shaderProgram->setUniformValue("diffuseProduct",diffuseProduct);
    shaderProgram->setUniformValue("specularProduct",specularProduct);
    shaderProgram->setUniformValue("shininess", static_cast<GLfloat>(material. shininess));

    shaderProgram->setUniformValue("texColorMap", 0);
    shaderProgram->setUniformValue("texNormalMap", 1);

    texture.glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D , texID [0]);
    texture.glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D , texID [1]);

    vboVertices->bind();
    shaderProgram->enableAttributeArray("vPosition");
    shaderProgram->setAttributeBuffer("vPosition", GL_FLOAT, 0, 4, 0);

    vboNormals->bind();
    shaderProgram->enableAttributeArray("vNormal");
    shaderProgram->setAttributeBuffer("vNormal", GL_FLOAT, 0, 3, 0);

    vboTexCoords -> bind();
    shaderProgram->enableAttributeArray("vTexCoord");
    shaderProgram->setAttributeBuffer("vTexCoord", GL_FLOAT, 0, 2, 0);

    vboTangents ->bind();
    shaderProgram -> enableAttributeArray("vTangent");
    shaderProgram -> setAttributeBuffer("vTangent", GL_FLOAT, 0, 4, 0);

    vboIndices -> bind();

    glDrawElements(GL_TRIANGLES, numFaces * 3,  GL_UNSIGNED_INT, 0);

    vboIndices -> release();
    vboTangents -> release();
    vboTexCoords -> release();
    vboNormals -> release();
    vboVertices -> release();
    shaderProgram -> release();
}

// Função para escolher o arquivo e projetar na tela
void GLWidget::showFileOpenDialog()
{
    QByteArray fileFormat = "off";
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this,
        "Open File",
        QDir::homePath(),
        QString("%1 Files (*.%2)")
        .arg(QString(fileFormat.toUpper()))
        .arg(QString(fileFormat)));

    if (!fileName.isEmpty())
    {
        readOFFFile(fileName);

        genNormals();
        genTexCoordsCylinder();
        genTangents();

        createVBOs();
        currentShader = 0;
        createShaders();

        updateGL();
    }
}

/*
 * Essa função é responsável por fazer a  leitura do arquivo OFF usando a classe nativa stream,
 * salvando as informações da malha.
 */
void GLWidget::readOFFFile(const QString &fileName)
{
    // A stream é usada pra abrir o arquivo
    std::ifstream stream;
    stream.open(fileName.toLatin1(), std::ifstream::in);

    // Se o arquivo não foi aberto, gera esse warning
    if(!stream.is_open()) {
        qWarning("Cannot open file.");
        return;
    }

    std::string line;

    stream >> line;
    stream >> numVertices >> numFaces>> line;

    delete[] vertices;
    vertices = new QVector4D[numVertices];

    delete[] indices;
    indices = new unsigned int[numFaces * 3 * 5]; // Implementação malha mista N

    // Os vertices da malha serão salvos como vetores 4D.
    // Isso, porque é usado para as coordenadas x, y, z e a posição usada pra zoom e tamanho
    if(numVertices > 0) {
        double minLim = std::numeric_limits<double>::min();
        double maxLim = std::numeric_limits<double>::max();
        QVector4D max(minLim, minLim, minLim, 1.0);
        QVector4D min(maxLim, maxLim, maxLim, 1.0);

        for(unsigned int i = 0; i < numVertices; i++) {
            float x, y, z;
            stream >> x >> y >> z;
            max.setX(qMax(max.x(),x));
            max.setY(qMax(max.y(),y));
            max.setZ(qMax(max.z(),z));
            min.setX(qMin(min.x(),x));
            min.setY(qMin(min.y(),y));
            min.setZ(qMin(min.z(),z));

            vertices[i] = QVector4D(x, y, z, 1.0);
        }

        // Usado pra redimensionar a malha
        QVector4D midpoint = (min + max) * 0.5;
        double invdiag = 1/(max-min).length();

        for(unsigned int i = 0; i < numVertices; i++) {
            vertices[i] = (vertices[i] - midpoint)*invdiag;
            vertices[i].setW(1);
        }
    }

    int i = 0;
    while(!stream.eof()) {
        unsigned int a, b, c, x;

        stream >> line >> a >> b >> c;
        std::string::size_type sz;   // alias of size_t
        int i_dec = std::stoi (line,&sz);
        indices[i*3  ]=a;
        indices[i*3+1]=b;
        indices[i*3+2]=c;
        i_dec -= 3;
        i++;
        while(i_dec!= 0){
            stream >> x;
            indices[i*3  ]=a;
            indices[i*3+1]=c;
            indices[i*3+2]=x;
            c=x;
            i_dec--;
            i++;
        }
    }
    numFaces = i;
    stream.close();
}

/* Calcula as normais nos vétices da malha
 * As normais são salvas como objetos Qvector3D no array
 */
void GLWidget :: genNormals ()
{
    delete[] normals;
    normals = new QVector3D[numVertices];

    for(unsigned int i = 0; i < numFaces; i++) {
        unsigned int i1 = indices[i*3  ];
        unsigned int i2 = indices[i*3+1];
        unsigned int i3 = indices[i*3+2];

        QVector3D v1 = vertices[i1].toVector3D();
        QVector3D v2 = vertices[i2].toVector3D();
        QVector3D v3 = vertices[i3].toVector3D();

        QVector3D faceNormal = QVector3D::crossProduct(v2-v1, v3-v1);
        normals[i1] += faceNormal;
        normals[i2] += faceNormal;
        normals[i3] += faceNormal;
    }
    for(unsigned int i = 0; i < numVertices; i++)
        normals[i].normalize();
}

/* As coordenadas cilindricas são geradas das texturas
 * para os vertices da malha.
 * O array texCoords armazena as cordenadas dos objetos Qvector2D
 */
void GLWidget :: genTexCoordsCylinder ()
{
    delete[] texCoords;
    texCoords = new QVector2D[numVertices];

    double minLim = std::numeric_limits<double>::min();
    double maxLim = std::numeric_limits<double>::max();
    QVector2D max(minLim, minLim);
    QVector2D min(maxLim, maxLim);

    for(unsigned int i = 0; i < numVertices; i++) {
        QVector2D pos = vertices[i].toVector2D();
        max.setX(qMax(max.x(), pos.x()));
        max.setY(qMax(max.y(), pos.y()));
        min.setX(qMin(min.x(), pos.x()));
        min.setY(qMin(min.y(), pos.y()));
    }

    QVector2D size = max - min;
    for(unsigned int i = 0; i < numVertices; i++) {
        double x = 2.0 * (vertices[i].x() - min.x()) /
                         size.x() - 1.0;
        texCoords[i] = QVector2D(acos(x) / M_PI,
                                (vertices[i].y() - min.y()) /
                                 size.y());
    }
}

/*
 * Para cada vértice são estimados os vetores tangentes.
 * Essa função foi baseada no método de Langeyl que usa o
 * mapeamento de normais como base
 */
void GLWidget :: genTangents ()
{
    delete[] tangents;

    tangents = new QVector4D[numVertices];
    QVector3D * bitangents = new QVector3D[numVertices];

    for(unsigned int i = 0; i < numFaces; i++) {
        unsigned int i1= indices[i * 3];
        unsigned int i2= indices[i*3+1];
        unsigned int i3= indices[i*3+2];

        QVector3D E = vertices[i1].toVector3D();
        QVector3D F = vertices[i2].toVector3D();
        QVector3D G = vertices[i3].toVector3D();

        QVector2D stE = texCoords[i1];
        QVector2D stF = texCoords[i2];
        QVector2D stG = texCoords[i3];

        QVector3D P = F - E;
        QVector3D Q = G - E;

        QVector2D st1 = stF - stE;
        QVector2D st2 = stG - stE;

        QMatrix2x2 M;
        M(0,0) =  st2.y(); M(0,1) = -st1.y();
        M(1,0) = -st2.x(); M(1,1) =  st1.x();
        M *= (1.0 / (st1.x()*st2.y() - st2.x()*st1.y()));
        QVector4D T = QVector4D(M(0,0)*P.x()+M(0,1)*Q.x(),
                                M(0,0)*P.y()+M(0,1)*Q.y(),
                                M(0,0)*P.z()+M(0,1)*Q.z(),
                                0.0);
        QVector3D B = QVector3D(M(1,0)*P.x()+M(1,1)*Q.x(),
                                M(1,0)*P.y()+M(1,1)*Q.y(),
                                M(1,0)*P.z()+M(1,1)*Q.z());
        tangents[i1] += T;
        tangents[i2] += T;
        tangents[i3] += T;

        bitangents[i1] += B;
        bitangents[i2] += B;
        bitangents[i3] += B;
    }

    for(unsigned int i = 0; i < numVertices; i++) {
        const QVector3D & n = normals[i];
        const QVector4D & t = tangents[i];

        tangents[i] = (t - n * QVector3D::dotProduct(n,
                       t.toVector3D())).normalized();
        QVector3D b = QVector3D::crossProduct(n, t.toVector3D());
        double hand = QVector3D::dotProduct(b, bitangents[i]);
        tangents[i].setW((hand < 0.0) ? -1.0 : 1.0);
    }

    delete[] bitangents;
}

/* Os VBOs do OpenGL são criados nessa função pra renderizar as malhas
 * Também utilizamos os Vertex Buffer Objects (VBO) pra manipular diretamente os dados do servidor
*/
void GLWidget :: createVBOs ()
{
    // Isso é pra não dar problemas de superposição e lixo nas variáveis
    destroyVBOs();

    // Usado pra armazenar a posição dos vertices
    vboVertices = new QGLBuffer(QGLBuffer::VertexBuffer);
    vboVertices->create(); // cria o buffer no servidor
    vboVertices->bind();   // faz o vínculo
    vboVertices->setUsagePattern(QGLBuffer::StaticDraw);
    vboVertices->allocate(vertices, numVertices * sizeof(QVector4D)); // envia os dados do vértice pro VBO

    delete[] vertices;
    vertices = NULL;

    // Usado pra armazenar as normais dos vértices
    vboNormals = new QGLBuffer(QGLBuffer::VertexBuffer);
    vboNormals->create();
    vboNormals->bind();
    vboNormals->setUsagePattern(QGLBuffer::StaticDraw);
    vboNormals->allocate(normals, numVertices * sizeof(QVector3D));

    delete[] normals;
    normals = NULL;

    // Usado pra armazenar as coordenadas da textura
    vboTexCoords = new QGLBuffer(QGLBuffer::VertexBuffer);
    vboTexCoords->create();vboTexCoords->bind();
    vboTexCoords->setUsagePattern(QGLBuffer::StaticDraw);
    vboTexCoords->allocate(texCoords, numVertices * sizeof(QVector2D));

    delete[] texCoords;
    texCoords = NULL;

    // A mesma coisa, só que pra vetores tangentes
    vboTangents = new QGLBuffer(QGLBuffer::VertexBuffer);
    vboTangents->create();
    vboTangents->bind();
    vboTangents->setUsagePattern(QGLBuffer::StaticDraw);
    vboTangents->allocate(tangents, numVertices * sizeof(QVector4D));

    delete[] tangents;
    tangents = NULL;

    // Esse é a mesma coisa que os anteriores, mas usa o InderBuffer para os índices
    vboIndices = new QGLBuffer(QGLBuffer::IndexBuffer);
    vboIndices->create();
    vboIndices->bind();
    vboIndices->setUsagePattern(QGLBuffer::StaticDraw);
    vboIndices->allocate(indices,numFaces * 3 * sizeof(unsigned int));

    delete[] indices;
    indices = NULL;
}

// Destrói os VBOs criados
void GLWidget::destroyVBOs()
{
    if(vboVertices) {
        vboVertices->release();
        delete vboVertices;
        vboVertices = NULL;
    }
    if(vboNormals) {
        vboNormals->release();
        delete vboNormals;
        vboNormals = NULL;
    }
    if(vboTexCoords) {
        vboTexCoords->release();
        delete vboTexCoords;
        vboTexCoords = NULL;
    }
    if(vboTangents) {
        vboTangents->release();
        delete vboTangents;
        vboTangents = NULL;
    }
    if(vboIndices) {
        vboIndices->release();
        delete vboIndices;
        vboIndices = NULL;
    }
}

// Os shaders do objeto são carregados nessa função
// Mas a aplicação do shader no objeto é feito em outra função
void GLWidget :: createShaders ()
{
    destroyShaders();

    QString   vertexShaderFile[] = {":/shaders/vgouraud.glsl",
                                    ":/shaders/vphong.glsl",
                                    ":/shaders/vtexture.glsl",
                                    ":/shaders/vnormal.glsl"
                                   };
    QString fragmentShaderFile[] = {":/shaders/fgouraud.glsl",
                                    ":/shaders/fphong.glsl",
                                    ":/shaders/ftexture.glsl",
                                    ":/shaders/fnormal.glsl"
                                    };

    vertexShader = new QGLShader(QGLShader::Vertex);
    if(!vertexShader->compileSourceFile(vertexShaderFile[currentShader]))
        qWarning() << vertexShader->log();

    fragmentShader = new QGLShader(QGLShader::Fragment);
    if(!fragmentShader->compileSourceFile(fragmentShaderFile[currentShader]))
        qWarning() << fragmentShader->log();

    shaderProgram = new QGLShaderProgram;
    shaderProgram->addShader(vertexShader);
    shaderProgram->addShader(fragmentShader);

    if(!shaderProgram->link())
        qWarning() << shaderProgram->log() << Qt::endl;
}

// Essa função deleta os shaders compilados aplicados no programa
void GLWidget :: destroyShaders ()
{
    delete vertexShader;
    vertexShader = NULL;

    delete fragmentShader;
    fragmentShader = NULL;

    if(shaderProgram) {
        shaderProgram->release();
        delete shaderProgram;
        shaderProgram = NULL;
     }
}

void GLWidget :: keyPressEvent ( QKeyEvent * event )
{
    switch (event ->key ()){
        case Qt :: Key_0 :
            currentShader = 0;
            createShaders ();
            updateGL ();
            break ;
        case Qt :: Key_1 :
            currentShader = 1;
            createShaders ();
            updateGL ();
            break;
        case Qt :: Key_2 :
            currentShader = 2;
            createShaders ();
            updateGL();
            break;
        case Qt :: Key_Escape:
            qApp ->quit();
    }
}

void GLWidget :: mouseMoveEvent ( QMouseEvent * event )
{
    trackBall . mouseMove (event -> pos());
}

void GLWidget :: mousePressEvent ( QMouseEvent * event )
{
    if (event -> button () & Qt :: LeftButton )
        trackBall . mousePress (event -> pos());
}

void GLWidget :: mouseReleaseEvent ( QMouseEvent * event )
{
    if (event -> button () == Qt :: LeftButton )
        trackBall . mouseRelease (event -> pos());
}

void GLWidget :: wheelEvent ( QWheelEvent * event )
{
    zoom += 0.001 * event -> delta ();
}

void GLWidget :: animate()
{
    updateGL();
}

void GLWidget::gurro()
{
    currentShader = 0;
    createShaders();
    updateGL();
}

void GLWidget::fong()
{
    currentShader = 1;
    createShaders();
    updateGL();
}

void GLWidget::textura()
{
    currentShader = 2;
    createShaders();
    updateGL();
}

void GLWidget::normal()
{
    currentShader = 3;
    createShaders();
    updateGL();
}
