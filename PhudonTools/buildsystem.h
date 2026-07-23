/*
 * @Description: 
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-07 23:45:35
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-04-10 22:15:49
 */
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

    // 关闭当前工程
    void closeProject();

signals:
    // 编译输出信号
    void buildOutput(const QString &output);
    
    // 编译完成信号
    void buildFinished(bool success);
    
    // 工程关闭信号
    void projectClosed();

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

    // 在BuildSystem类中添加STM32特定的方法
    public:
        // 设置STM32工具链路径
        void setToolchainPath(const QString &path);
        
        // 设置STM32芯片型号
        void setTargetChip(const QString &chip);
        
        // 生成Makefile
        bool generateMakefile();
        
    // 在BuildSystem类的private部分添加
    private:
        // STM32工具链路径
        QString m_toolchainPath;
        
        // 目标芯片型号
        QString m_targetChip;
        
        // 生成链接脚本
        bool generateLinkerScript();
};

#endif // BUILDSYSTEM_H