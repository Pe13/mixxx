#include "rendergraph/materialshader.h"

#include <QFile>
#include <QtGlobal>
#ifdef USE_QSHADER_FOR_GL
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <rhi/qshader.h>
#else
#include <private/qshader_p.h>
#endif
#include <QOpenGLContext>
#endif

#include <optional>

using namespace rendergraph;

namespace {
#ifdef USE_QSHADER_FOR_GL
QString resource(const QString& filename) {
    return QStringLiteral(":/shaders/rendergraph/%1.qsb").arg(filename);
}

std::optional<QShaderKey> searchGlslEsShader(const QList<QShaderKey>& keys) {
    std::optional<QShaderKey> selectedKey;
    for (const auto& key : keys) {
        if (key.source() != QShader::GlslShader ||
                !(key.sourceVersion().flags() & QShaderVersion::GlslEs)) {
            continue;
        }
        // Prefer newer version, or just take any GlEsShader
        if (!selectedKey ||
                key.sourceVersion().version() >
                        selectedKey.value().sourceVersion().version()) {
            selectedKey = key;
        }
    }
    return selectedKey;
}

std::optional<QShaderKey> searchGlslShader(const QList<QShaderKey>& keys) {
    std::optional<QShaderKey> selectedKey;
    for (const auto& key : keys) {
        if (key.source() != QShader::GlslShader ||
                (key.sourceVersion().flags() & QShaderVersion::GlslEs)) {
            continue;
        }
        // Prefer version 120 or the first available
        if (!selectedKey || key.sourceVersion().version() == 120) {
            selectedKey = key;
        }
    }
    return selectedKey;
}

QByteArray loadShaderCodeFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODeviceBase::ReadOnly)) {
        return QByteArray();
    }
    const auto qsbShader = QShader::fromSerialized(file.readAll());
    std::optional<QShaderKey> selectedKey;
    const QList<QShaderKey> keys = qsbShader.availableShaders();

    const QOpenGLContext* context = QOpenGLContext::currentContext();
    if (context && context->isOpenGLES()) {
        // First look for OpenGL ES shaders
        selectedKey = searchGlslEsShader(keys);
    }
    if (!selectedKey) {
        // If not ES, or if we couldn't find a GLES shader, look for desktop GLSL
        selectedKey = searchGlslShader(keys);
    }
    if (!selectedKey && !keys.isEmpty()) {
        selectedKey = keys.first();
    }

    if (selectedKey) {
        return qsbShader.shader(selectedKey.value()).shader();
    }
    return QByteArray();
}
#else
QString resource(const QString& filename) {
    return QStringLiteral(":/shaders/rendergraph/%1.gl").arg(filename);
}

QByteArray loadShaderCodeFromFile(const QString& path) {
    QFile file(path);
    file.open(QIODeviceBase::ReadOnly);
    return file.readAll();
}
#endif
} // namespace

MaterialShader::MaterialShader(const char* vertexShaderFilename,
        const char* fragmentShaderFilename,
        const UniformSet& uniformSet,
        const AttributeSet& attributeSet) {
    const QString vertexShaderFileFullPath = resource(vertexShaderFilename);
    const QString fragmentShaderFileFullPath = resource(fragmentShaderFilename);

    addShaderFromSourceCode(QOpenGLShader::Vertex,
            loadShaderCodeFromFile(vertexShaderFileFullPath));
    addShaderFromSourceCode(QOpenGLShader::Fragment,
            loadShaderCodeFromFile(fragmentShaderFileFullPath));

    link();

    for (const auto& attribute : attributeSet.attributes()) {
        int location = QOpenGLShaderProgram::attributeLocation(attribute.m_name);
        m_attributeLocations.push_back(location);
    }
    for (const auto& uniform : uniformSet.uniforms()) {
        int location = QOpenGLShaderProgram::uniformLocation(uniform.m_name);
        m_uniformLocations.push_back(location);
    }
}
