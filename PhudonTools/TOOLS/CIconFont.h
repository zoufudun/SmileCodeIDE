#ifndef CICONFONT_H
#define CICONFONT_H

#include <QObject>
#include <QIcon>
#include <QFont>
#include <QMap>
#include <QString>
#include <QChar>


class CIconFont : public QObject
{
    Q_OBJECT
public:
   // explicit CIconFont(QObject *parent = nullptr);

    // 单例模式：全局唯一实例，避免重复加载字体
    static CIconFont* instance();

    // 加载图标字体文件（支持绝对路径/资源路径）
    bool loadFont(const QString& fontFilePath);

    // 注册图标（键名 + Unicode 编码），方便后续通过名称获取
    void registerIcon(const QString& iconName, const QString& unicode);
    void registerIcons(const QMap<QString, QString>& icons);

    // 根据名称获取图标对应的 QChar
    QChar getIconChar(const QString& iconName);

    // 获取加载好的图标字体（直接给控件设置）
    QFont getIconFont(int pixelSize = 16);

    static CIconFont *getInstance();

    void init(QString strFile);
    QFont font();

    QPixmap pixmap(QChar ch,QSize size,QString strColor = "black");



private:
    explicit CIconFont(QObject *parent = nullptr);
    ~CIconFont() = default;

    // 禁止拷贝
    CIconFont(const CIconFont&) = delete;
    CIconFont& operator=(const CIconFont&) = delete;

    QFont m_iconFont;          // 加载好的图标字体
    QString m_fontFamily;      // 字体族名
    QString m_strFontName;
    QMap<QString, QChar> m_iconMap; // 图标名 -> Unicode 字符映射
};

#endif
