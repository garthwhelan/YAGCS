#include <Qt3DCore/QEntity>

void add_obj_to_scene(Qt3DCore::QEntity *rootEntity, QString filename);
void add_xyz_to_scene(Qt3DCore::QEntity *rootEntity);
void add_line_to_scene(Qt3DCore::QEntity *rootEntity, QVector3D a, QVector3D b);
