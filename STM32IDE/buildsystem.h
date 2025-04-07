#ifndef BUILDSYSTEM_H
#define BUILDSYSTEM_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QFileInfo>

class BuildSystem : public QObject
{
    Q_OBJECT

public:
    explicit BuildSystem(QObject *parent = nullptr);
    ~BuildSystem();

    // 设置项目路径
    void setProjectPath(const QString &path);
    
    // 设置输出路径
    void setOutputPath(const QString &path);
    
    // 编译单个文件
    bool compileFile(const QString &filePath);
    
    // 构建整个项目
    bool buildProject();
    
    // 清理构建文件
    bool cleanProject();
    
    // 获取最后的错误信息
    QString lastError() const;

signals:
    // 编译输出信号
    void buildOutput(const QString &output);
    
    // 编译完成信号
    void buildFinished(bool success);

private slots:
    // 处理编译进程输出
    void handleProcessOutput();
    
    // 处理编译进程完成
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    // 获取编译命令
    QString getCompileCommand(const QString &filePath);
    
    // 项目路径
    QString m_projectPath;
    
    // 输出路径
    QString m_outputPath;
    
    // 编译进程
    QProcess *m_process;
    
    // 最后的错误信息
    QString m_lastError;
};

#endif // BUILDSYSTEM_H