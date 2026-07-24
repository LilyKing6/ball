#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <QOpenGLShaderProgram>
#include <QHash>
#include <QString>

class ShaderManager {
public:
    static ShaderManager& instance() { static ShaderManager s; return s; }

    bool load(const QString& name, const QString& vertPath, const QString& fragPath);
    QOpenGLShaderProgram* get(const QString& name) const;
    void bind(const QString& name);
    void releaseAll();

private:
    QHash<QString, QOpenGLShaderProgram*> m_shaders;
};

#endif // SHADERMANAGER_H
