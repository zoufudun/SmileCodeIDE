#pragma once
#include <QtTest>
#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QToolBar>
#include <QMenuBar>
#include <QDockWidget>
#include <functional>
#include <QSettings>
#include <QTemporaryDir>
#include "mainwindow.h"
#include "ideworkflowicons.h"

class MainWindowWorkflowTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void controllerExistsAndActionsFollowState();
    void renderWorkflowIcons();
    void compactWorkspaceHasExplicitConfiguration();
    void cancelledConfigurationDialogsKeepSavedState();
    void acceptedFlashDialogWritesOnlyCurrentProject();
    void testBuildTriggerCrash();
    void testMenuUniquenessAndBuildSafety();
    void testRecentProjectDirectoryResolution();

private:
    static void withModal(bool &sawDialog, std::function<void(QDialog *)> action, int attempts = 50);
    static void rejectModal(bool &sawDialog);

    QTemporaryDir m_settings;
};
