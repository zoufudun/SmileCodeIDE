#include "CIconFont.h"
#include <QFontDatabase>
#include <QDebug>

CIconFont* CIconFont::instance()
{
    static CIconFont s_instance;
    return &s_instance;
}

CIconFont::CIconFont(QObject *parent) : QObject(parent)
{
}

bool CIconFont::loadFont(const QString &fontFilePath)
{
    // 加载字体文件
    int fontId = QFontDatabase::addApplicationFont(fontFilePath);
    if (fontId == -1) {
        qDebug() << "[CIconFont] 字体加载失败：" << fontFilePath;
        return false;
    }

    // 获取字体族名
    m_fontFamily = QFontDatabase::applicationFontFamilies(fontId).value(0);
    if (m_fontFamily.isEmpty()) {
        qDebug() << "[CIconFont] 获取字体族名失败";
        return false;
    }

    // 初始化字体
    m_iconFont = QFont(m_fontFamily);
    qDebug() << "[CIconFont] 字体加载成功：" << m_fontFamily;
    return true;
}

void CIconFont::registerIcon(const QString &iconName, const QString &unicode)
{
    // 把 Unicode 字符串（如 "0xe604"）转成 QChar
    bool ok;
    ushort code = unicode.toUShort(&ok, 16);
    if (ok) {
        m_iconMap[iconName] = QChar(code);
    } else {
        qDebug() << "[CIconFont] 图标编码无效：" << unicode;
    }
}

void CIconFont::registerIcons(const QMap<QString, QString> &icons)
{
    for (auto it = icons.constBegin(); it != icons.constEnd(); ++it) {
        registerIcon(it.key(), it.value());
    }
}

QChar CIconFont::getIconChar(const QString &iconName)
{
    if (m_iconMap.contains(iconName)) {
        return m_iconMap[iconName];
    }
    qDebug() << "[CIconFont] 未找到图标：" << iconName;
    return QChar();
}

QFont CIconFont::getIconFont(int pixelSize)
{
    QFont font = m_iconFont;
    font.setPixelSize(pixelSize); // 设置图标大小
    return font;
}


CIconFont *CIconFont::getInstance()
{
    static CIconFont *pInstance = nullptr;
    if(nullptr == pInstance) {
        pInstance = new CIconFont();
    }
    return pInstance;
}

void CIconFont::init(QString strFile)
{
    int fontId = QFontDatabase::addApplicationFont(strFile);
    m_strFontName = QFontDatabase::applicationFontFamilies(fontId).at(0);
}

QFont CIconFont::font()
{
    return QFont(m_strFontName);
}
#include <QLabel>
#include <QSize>
QPixmap CIconFont::pixmap(QChar ch, QSize size, QString strColor)
{
    QLabel *pLabel = new QLabel();
    pLabel->setText(ch);
    pLabel->setStyleSheet(QString("font:%1px iconfont;color:%2;background:transparent;")
                          .arg(qMin(size.width(),size.height()))
                          .arg(strColor));
    pLabel->resize(size);

    QPixmap pixmap(pLabel->size());
    pixmap.fill(Qt::transparent);
    pLabel->render(&pixmap);
    pLabel->deleteLater();

    return pixmap;
}
