#include "rendergraph/materialshader.h"

#include <QFile>
#ifdef USE_QSHADER_FOR_GL
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <rhi/qshader.h>
#else
#include <private/qshader_p.h>
#endif
#include <QOpenGLContext>
#endif

#include <optional>

#include "../../util/assert.h"

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
        qWarning() << "Failed to open the shader file:" << path;
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
    if (!file.open(QIODeviceBase::ReadOnly)) {
        qWarning() << "Failed to open shader file:" << path;
        return QByteArray();
    }
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

    QByteArray vertexCode = loadShaderCodeFromFile(vertexShaderFileFullPath);
    QByteArray fragmentCode = loadShaderCodeFromFile(fragmentShaderFileFullPath);
    VERIFY_OR_DEBUG_ASSERT(!vertexCode.isEmpty() && !fragmentCode.isEmpty()) {
        return;
    }
    if (!addShaderFromSourceCode(QOpenGLShader::Vertex, vertexCode)) {
        qWarning() << "MaterialShader - compilation failed:"
                   << vertexShaderFileFullPath;
        qDebug() << log();
        return;
    }

    if (!addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentCode)) {
        qWarning() << "MaterialShader - compilation failed:"
                   << fragmentShaderFileFullPath;
        qDebug() << log();
        return;
    }

    if (!link()) {
        qDebug() << "MaterialShader - linking failed."
                 << log();
        return;
    }

    for (const auto& attribute : attributeSet.attributes()) {
        int location = QOpenGLShaderProgram::attributeLocation(attribute.m_name);
        m_attributeLocations.push_back(location);
    }
    for (const auto& uniform : uniformSet.uniforms()) {
        int location = QOpenGLShaderProgram::uniformLocation(uniform.m_name);
        m_uniformLocations.push_back(location);
    }
}
