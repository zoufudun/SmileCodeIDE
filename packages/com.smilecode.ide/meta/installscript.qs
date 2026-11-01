function Component()
{
    // Constructor
}

Component.prototype.createOperations = function()
{
    // Call default createOperations to install data
    component.createOperations();

    if (systemInfo.productType === "windows") {
        var targetExe = installer.value("TargetDir") + "/SmileCodeIDE.exe";
        var workingDir = installer.value("TargetDir");

        // 1. Desktop Shortcut
        component.addOperation("CreateShortcut",
                               targetExe,
                               "@DesktopDir@/SmileCodeIDE.lnk",
                               "workingDirectory=" + workingDir,
                               "iconPath=" + targetExe,
                               "iconId=0",
                               "description=SmileCodeIDE Studio");

        // 2. Start Menu Program Shortcut
        component.addOperation("CreateShortcut",
                               targetExe,
                               "@StartMenuDir@/SmileCodeIDE.lnk",
                               "workingDirectory=" + workingDir,
                               "iconPath=" + targetExe,
                               "iconId=0",
                               "description=SmileCodeIDE Studio");

        // 3. Start Menu Uninstaller Shortcut
        var maintTool = installer.value("TargetDir") + "/maintenancetool.exe";
        component.addOperation("CreateShortcut",
                               maintTool,
                               "@StartMenuDir@/Uninstall SmileCodeIDE.lnk",
                               "workingDirectory=" + workingDir,
                               "description=Uninstall SmileCodeIDE");
    }
}
