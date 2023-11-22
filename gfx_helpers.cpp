#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QGoochMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DRender/QMesh>

void add_obj_to_scene(Qt3DCore::QEntity *rootEntity, QString filename) {
  Qt3DCore::QEntity *objEntity = new Qt3DCore::QEntity(rootEntity);
  Qt3DRender::QMesh *mesh = new Qt3DRender::QMesh();
  Qt3DRender::QMaterial *material = new Qt3DExtras::QGoochMaterial(rootEntity);
  QUrl url;
  url.setScheme("file");
  url.setPath(filename);
  mesh->setSource(url);
  Qt3DCore::QTransform *objTransform = new Qt3DCore::QTransform;
  // meters to mm
  objTransform->setScale3D(QVector3D(1000, 1000, 1000));
  objEntity->addComponent(mesh);
  objEntity->addComponent(objTransform);
  objEntity->addComponent(material);
  objEntity->setObjectName("Obj");
}

void add_xyz_to_scene(Qt3DCore::QEntity *rootEntity) {
  for (int i = 0; i < 3; ++i) {
    Qt3DCore::QEntity *axisEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DExtras::QCylinderMesh *mesh = new Qt3DExtras::QCylinderMesh;
    mesh->setLength(1000);
    mesh->setRadius(0.2);
    Qt3DExtras::QGoochMaterial *material =
        new Qt3DExtras::QGoochMaterial(rootEntity);
    Qt3DCore::QTransform *axisTransform = new Qt3DCore::QTransform;
    axisTransform->setScale3D(QVector3D(1, 1, 1));
    if (i == 0) {
      axisTransform->setRotation(
          QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 0.0f));
      axisTransform->setTranslation(QVector3D(0, 500, 0));
      axisEntity->setObjectName("Yaxis");
      material->setCool(QColor(0, 255, 0));
      material->setWarm(QColor(0, 255, 0));
    }
    if (i == 1) {
      axisTransform->setRotation(
          QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 90.0f));
      axisTransform->setTranslation(QVector3D(0, 0, 500));
      axisEntity->setObjectName("Zaxis");
      material->setCool(QColor(0, 0, 255));
      material->setWarm(QColor(0, 0, 255));
    }
    if (i == 2) {
      axisTransform->setRotation(
          QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 90.0f));
      axisTransform->setTranslation(QVector3D(500, 0, 0));
      axisEntity->setObjectName("Xaxis");
      material->setCool(QColor(255, 0, 0));
      material->setWarm(QColor(255, 0, 0));
    }
    axisEntity->addComponent(mesh);
    axisEntity->addComponent(axisTransform);
    axisEntity->addComponent(material);
  }
}

void add_line_to_scene(Qt3DCore::QEntity *rootEntity, QVector3D a,
                       QVector3D b) {
  Qt3DCore::QEntity *vEntity = new Qt3DCore::QEntity(rootEntity);
  Qt3DExtras::QGoochMaterial *material = new Qt3DExtras::QGoochMaterial(rootEntity);
  Qt3DCore::QTransform *vTransform = new Qt3DCore::QTransform;
  Qt3DExtras::QCylinderMesh *mesh = new Qt3DExtras::QCylinderMesh();
  mesh->setRadius(0.1);
  mesh->setLength((b - a).length());
  vTransform->setScale3D(QVector3D(1, 1, 1));
  if ((b - a).length() != 0) {
    QVector3D v = QVector3D::crossProduct(QVector3D(0, 1, 0), (b - a));
    if (v.length() != 0) {
      vTransform->setRotation(
          QQuaternion::rotationTo(QVector3D(0, 1, 0), b - a));
    }
  }
  vTransform->setTranslation(0.5 * (a + b));
  vEntity->addComponent(mesh);
  vEntity->addComponent(vTransform);

  material->setCool(QColor(255, 0, 255));
  material->setWarm(QColor(255, 0, 255));
  
  vEntity->addComponent(material);
  vEntity->setObjectName("Path");
}
