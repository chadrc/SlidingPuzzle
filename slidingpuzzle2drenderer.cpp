#include "slidingpuzzle2drenderer.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLShaderProgram>
#include <QVector>
#include <QVector3D>
#include <QOpenGLFunctions_4_3_Core>
#include <QtGlobal>
#include <QTime>
#include <QRect>

#include "oglitem.h"
#include "slidingpuzzleutility.h"
#include "vertex.h"

SlidingPuzzle2DRenderer::SlidingPuzzle2DRenderer(OGLItem *parent)
{
    qsrand(QTime::currentTime().second());
    oglItem = parent;
    initializeOpenGLFunctions();

    prog = new QOpenGLShaderProgram();
    vertSource = SlidingPuzzleUtility::GetTextFromFile(":/sliding2d.vert");
    fragSource = SlidingPuzzleUtility::GetTextFromFile(":/sliding2d.frag");
    prog->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSource.c_str());
    prog->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSource.c_str());

    prog->link();

    texture = new QOpenGLTexture(QImage(QString(":/test_cat.png")).mirrored());

    instancePosAttr = prog->attributeLocation("instancePos");
    vertexPosAttr = prog->attributeLocation("position");
    vertexClrAttr = prog->attributeLocation("color");
    vertexUvAttr = prog->attributeLocation("uvCoords");
    hiddenInstanceAttr = prog->uniformLocation("hiddenInstance");
    modelMatrixAttr = prog->uniformLocation("modelMatrix");
    viewProjMatrixAttr = prog->uniformLocation("viewProjMatrix");

//    qDebug() << "Hidden Attr: " << hiddenInstanceAttr;
//    qDebug() << "Instance Pos Attr: " << instancePosAttr;
//    qDebug() << "Vertex Pos Attr: " << vertexPosAttr;
//    qDebug() << "Vertex Clr Attr: " << vertexClrAttr;
//    qDebug() << "Vertex UV Attr: " << vertexUvAttr;
//    qDebug() << "Model Matrix Attr: " << modelMatrixAttr;
//    qDebug() << "View Proj Attr: " << viewProjMatrixAttr;

    funcs = nullptr;
//    funcs = (QOpenGLFunctions_4_3_Core*)QOpenGLContext::currentContext()->versionFunctions();
//    if (funcs == nullptr)
//    {
//        qDebug() << "Funcs do not exist.";
//        return;
//    }

    vertices.append(Vertex(QVector3D(-.5, -.5, 0),
                           QVector4D(0, 1.0, 0, 1.0),
                           QVector2D(0, 0)));

    vertices.append(Vertex(QVector3D(.5, -.5, 0),
                           QVector4D(1.0, 0, 0, 1.0),
                           QVector2D(1, 0)));

    vertices.append(Vertex(QVector3D(.5, .5, 0),
                           QVector4D(1.0, 0, 1.0, 1.0),
                           QVector2D(1, 1)));

    vertices.append(Vertex(QVector3D(-.5, .5, 0),
                           QVector4D(0, 0, 1.0, 1.0),
                           QVector2D(0, 1)));

    indicies.append(0);
    indicies.append(1);
    indicies.append(2);

    indicies.append(0);
    indicies.append(2);
    indicies.append(3);

    projectionMat.ortho(-1, 1, -1, 1, .01, 10);
    resetPuzzle(4);
}

SlidingPuzzle2DRenderer::~SlidingPuzzle2DRenderer()
{
    delete prog;
}

void SlidingPuzzle2DRenderer::resetPuzzle(int size)
{
    puzzleSize = size;
    for (int i=0; i<puzzleSize; i++)
    {
        for (int j=0; j<puzzleSize; j++)
        {
            auto pos = QVector3D(i+1, j+1, 0);
            positions.append(pos);
        }
    }

    hiddenInstance = qrand() % positions.size();
    hiddenPosition = hiddenInstance;
    qDebug() << "Hidden: " << hiddenInstance;
}

int SlidingPuzzle2DRenderer::getClosestCell(int x, int y)
{
    int hit = -1;

    // Set bottom left as (0,0)
    QVector4D worldC(x, oglItem->height() - y, 0, 1);

    int halfww = oglItem->width()/2;
    int halfwh = oglItem->height()/2;

    worldC.setX((worldC.x() - halfww) / halfww);
    worldC.setY((worldC.y() - halfwh) / halfwh);

    worldC = /*projectionMat * cameraMat * modelMat **/  worldC;

//    qDebug() << "Coords: " << worldC;

    float cellSize = (1.0f/puzzleSize)/2;

    QMatrix4x4 mat = projectionMat * cameraMat *  modelMat;

    for(int i=0; i<positions.size(); i++)
    {
        QVector4D translatedPos = mat *  QVector4D(positions[i].x(), positions[i].y(), 0, 1);
//        qDebug() << "Pos: " << positions[i] << "->" << translatedPos;

        if (worldC.x() > translatedPos.x() - cellSize && worldC.x() <= translatedPos.x() + cellSize
                && worldC.y() > translatedPos.y() - cellSize && worldC.y() <= translatedPos.y() + cellSize)
        {
            hit = i;
            break;
        }
    }

    QVector<int> hiddenNeighbors;
    QVector3D hVec = positions[hiddenInstance];

    for (int i=0; i<positions.size(); i++)
    {
        if (i == hiddenInstance)
        {
            continue;
        }

        QVector3D vec = positions[i];
        if ((vec.x() == hVec.x() && vec.y() - 1 == hVec.y()) || // above hVec
                (vec.x() == hVec.x() && vec.y() + 1 == hVec.y()) || // below hVec
                (vec.x() - 1 == hVec.x() && vec.y() == hVec.y()) || // right of hVec
                (vec.x() + 1 == hVec.x() && vec.y() == hVec.y())) // left of hVec
        {
            hiddenNeighbors.append(i);
            qDebug() << "Neighbor found: " << i;
        }
    }
    for(int i=0; i<hiddenNeighbors.size(); i++)
    {
        if (hiddenNeighbors[i] == hit)
        {
            QVector3D temp = positions[hiddenInstance];
            positions[hiddenInstance] = positions[hit];
            positions[hit] = temp;
            qDebug() << "Swapping: " << hiddenPosition << "->" << hit;
            hiddenPosition = hit;
            break;
        }
    }
    oglItem->window()->update();
//    qDebug() << "Hit: " << hit;
    return hit;
}

void SlidingPuzzle2DRenderer::setViewportSize(const QSize &size)
{
    viewportSize = size;
//    projectionMat.perspective(45, (float) size.width()/size.height(), .01, 10);
}

void SlidingPuzzle2DRenderer::setWindow(QQuickWindow *window)
{
    //qDebug() << "Window Set";
    this->window = window;

    funcs = (QOpenGLFunctions_4_3_Core*)QOpenGLContext::currentContext()->versionFunctions();

    prog->bind();
    if (vao.isCreated())
    {
        vao.create();
    }

    vao.bind();
    prog->enableAttributeArray(instancePosAttr);
    funcs->glVertexAttribDivisor(instancePosAttr, 1);
    prog->enableAttributeArray(vertexPosAttr);
    prog->enableAttributeArray(vertexClrAttr);
    prog->enableAttributeArray(vertexUvAttr);

    vertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vertexBuffer.allocate(vertices.constData(), sizeof(Vertex) * vertices.size());

    indexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    indexBuffer.create();
    indexBuffer.bind();
    indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    indexBuffer.allocate(indicies.constData(), sizeof(int)*indicies.size());

    prog->setAttributeBuffer(vertexPosAttr, GL_FLOAT, 0, 3, sizeof(Vertex));
    prog->setAttributeBuffer(vertexClrAttr, GL_FLOAT, sizeof(QVector3D), 4, sizeof(Vertex));
    prog->setAttributeBuffer(vertexUvAttr, GL_FLOAT, sizeof(QVector3D) + sizeof(QVector4D), 2, sizeof(Vertex));

    positionBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    positionBuffer.create();
    positionBuffer.bind();

    puzzleSize = 4;

    positionBuffer.allocate(positions.constData(), sizeof(QVector3D) * positions.size());
    prog->setAttributeBuffer(instancePosAttr, GL_FLOAT, 0, 3);

    cameraMat.setToIdentity();
    cameraMat.translate(-1-(1.0f/puzzleSize), -1-(1.0f/puzzleSize), -3);

    modelMat.setToIdentity();
    modelMat.scale(2.0f/puzzleSize);
}

void SlidingPuzzle2DRenderer::paint()
{
    if (funcs == nullptr)
    {
        qDebug() << "Funcs do not exist. Will not draw.";
        return;
    }

    prog->bind();
    vao.bind();

    positionBuffer.allocate(positions.constData(), sizeof(QVector3D) * positions.size());
//    positionBuffer.write(0, positions.constData(), sizeof(QVector3D) * positions.size());

    prog->setUniformValue(hiddenInstanceAttr, hiddenInstance);
    prog->setUniformValue(viewProjMatrixAttr, projectionMat * cameraMat);
    prog->setUniformValue(modelMatrixAttr, modelMat);

    prog->setUniformValue("size", puzzleSize);
    prog->setUniformValue("texture", 0);
    texture->bind();

//    qDebug() << "size: " << oglItem->width() << ", " << oglItem->height();
//    qDebug() << "pos: " << oglItem->x() << ", " << oglItem->y();
    glViewport(oglItem->x(), oglItem->y(), oglItem->width(), oglItem->height());

    glDisable(GL_DEPTH_TEST);

    glClearColor(.5, 0, .5, 1);
//    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    funcs->glDrawElementsInstanced(GL_TRIANGLES,
                                   indicies.size(),
                                   GL_UNSIGNED_INT,
                                   0,
                                   positions.size());

    // Clean up
    prog->disableAttributeArray(instancePosAttr);
    prog->disableAttributeArray(vertexPosAttr);
    prog->disableAttributeArray(vertexClrAttr);
    prog->disableAttributeArray(vertexUvAttr);
    prog->release();
    positionBuffer.release();
    vertexBuffer.release();
    indexBuffer.release();
    vao.release();

    glClearColor(1,1,1,1);
    window->resetOpenGLState();
}
