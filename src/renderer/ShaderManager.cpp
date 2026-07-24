#include "ShaderManager.h"
#include <QFile>
#include <QDebug>

bool ShaderManager::load(const QString& name, const QString& vertPath, const QString& fragPath) {
    auto* prog = new QOpenGLShaderProgram();

    QFile vf(vertPath);
    if (!vf.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open vertex shader:" << vertPath;
        delete prog; return false;
    }

    QFile ff(fragPath);
    if (!ff.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open fragment shader:" << fragPath;
        delete prog; return false;
    }

    if (!prog->addShaderFromSourceCode(QOpenGLShader::Vertex, vf.readAll())) {
        qWarning() << "Vertex shader compile error:" << prog->log();
        delete prog; return false;
    }

    if (!prog->addShaderFromSourceCode(QOpenGLShader::Fragment, ff.readAll())) {
        qWarning() << "Fragment shader compile error:" << prog->log();
        delete prog; return false;
    }

    if (!prog->link()) {
        qWarning() << "Shader link error:" << prog->log();
        delete prog; return false;
    }

    m_shaders[name] = prog;
    return true;
}

QOpenGLShaderProgram* ShaderManager::get(const QString& name) const {
    return m_shaders.value(name, nullptr);
}

void ShaderManager::bind(const QString& name) {
    auto* p = get(name);
    if (p) p->bind();
}

void ShaderManager::releaseAll() {
    for (auto* p : m_shaders) {
        p->release();
        delete p;
    }
    m_shaders.clear();
}
