/*
 * @Description: 
 * @Version: 1.0
 * @Autor: PhodonZou
 * @Date: 2025-04-07 23:45:41
 * @LastEditors: PhodonZou
 * @LastEditTime: 2025-04-07 23:45:43
 */
#include "buildsystem.h"
#include <QDir>
#include <QDebug>

BuildSystem::BuildSystem(QObject *parent) : QObject(parent), m_process(nullptr)
{
    m_process = new QProcess(this);
    
    // 连接进程信号
    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildSystem::handleProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildSystem::handleProcessOutput);
    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &BuildSystem::handleProcessFinished);
}

BuildSystem::~BuildSystem()
{
    if (m_process) {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            m_process->waitForFinished(3000);
        }
    }
}

void BuildSystem::setProjectPath(const QString &path)
{
    m_projectPath = path;
    
    // 确保项目路径存在
    QDir dir(m_projectPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

void BuildSystem::setOutputPath(const QString &path)
{
    m_outputPath = path;
    
    // 确保输出路径存在
    QDir dir(m_outputPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

bool BuildSystem::compileFile(const QString &filePath)
{
    if (m_projectPath.isEmpty()) {
        m_lastError = "项目路径未设置";
        return false;
    }
    
    if (m_outputPath.isEmpty()) {
        m_outputPath = m_projectPath + "/build";
        QDir dir(m_outputPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
    
    // 获取编译命令
    QString command = getCompileCommand(filePath);
    if (command.isEmpty()) {
        m_lastError = "不支持的文件类型";
        return false;
    }
    
    // 设置工作目录
    m_process->setWorkingDirectory(m_projectPath);
    
    // 启动编译进程
    qDebug() << "执行命令: " << command;
    m_process->start(command);
    
    return m_process->waitForStarted();
}

bool BuildSystem::buildProject()
{
    if (m_projectPath.isEmpty()) {
        m_lastError = "项目路径未设置";
        return false;
    }
    
    // 查找项目中的所有C/C++文件
    QDir dir(m_projectPath);
    QStringList filters;
    filters << "*.c" << "*.cpp";
    QStringList files = dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    
    if (files.isEmpty()) {
        m_lastError = "项目中没有找到C/C++源文件";
        return false;
    }
    
    // 编译每个文件
    bool success = true;
    for (const QString &file : files) {
        QString filePath = m_projectPath + "/" + file;
        if (!compileFile(filePath)) {
            success = false;
            break;
        }
        
        // 等待当前文件编译完成
        m_process->waitForFinished(-1);
    }
    
    return success;
}

bool BuildSystem::cleanProject()
{
    if (m_outputPath.isEmpty()) {
        m_lastError = "输出路径未设置";
        return false;
    }
    
    // 删除输出目录中的所有文件
    QDir dir(m_outputPath);
    return dir.removeRecursively();
}

QString BuildSystem::lastError() const
{
    return m_lastError;
}

QString BuildSystem::getCompileCommand(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    QString fileName = fileInfo.fileName();
    QString baseName = fileInfo.baseName();
    QString outputFile = m_outputPath + "/" + baseName + ".o";
    
    // 根据文件类型选择编译器
    if (extension == "c") {
        // 使用GCC编译C文件
        return QString("gcc -c -Wall -std=c11 -o \"%1\" \"%2\"").arg(outputFile, filePath);
    } else if (extension == "cpp") {
        // 使用G++编译C++文件
        return QString("g++ -c -Wall -std=c++11 -o \"%1\" \"%2\"").arg(outputFile, filePath);
    }
    
    return QString();
}

void BuildSystem::handleProcessOutput()
{
    // 读取标准输出
    QByteArray output = m_process->readAllStandardOutput();
    if (!output.isEmpty()) {
        emit buildOutput(QString::fromLocal8Bit(output));
    }
    
    // 读取标准错误
    QByteArray error = m_process->readAllStandardError();
    if (!error.isEmpty()) {
        emit buildOutput(QString::fromLocal8Bit(error));
    }
}

void BuildSystem::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    
    if (!success) {
        m_lastError = "编译失败，退出代码: " + QString::number(exitCode);
    }
    
    emit buildFinished(success);
}