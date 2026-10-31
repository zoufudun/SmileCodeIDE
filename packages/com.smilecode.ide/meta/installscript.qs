function Component()
{
    // Constructor
}

Component.prototype.createOperations = function()
{
    // Call default createOperations to install data
    component.createOperations();

    if (systemInfo.productType === "windows") {
        // Desktop Shortcut
        component.addOperation("CreateShortcut",
                               "@TargetDir@/SmileCodeIDE.exe",
                               "@DesktopDir@/SmileCodeIDE.lnk",
                               "workingDirectory=@TargetDir@",
                               "iconPath=@TargetDir@/SmileCodeIDE.exe",
                               "description=SmileCodeIDE 嵌入式智能开发工作台");

        // Start Menu Shortcuts
        component.addOperation("CreateShortcut",
                               "@TargetDir@/SmileCodeIDE.exe",
                               "@StartMenuDir@/SmileCodeIDE.lnk",
                               "workingDirectory=@TargetDir@",
                               "iconPath=@TargetDir@/SmileCodeIDE.exe",
                               "description=SmileCodeIDE 嵌入式智能开发工作台");

        // Uninstall Shortcut
        component.addOperation("CreateShortcut",
                               "@TargetDir@/maintenancetool.exe",
                               "@StartMenuDir@/卸载 SmileCodeIDE.lnk",
                               "workingDirectory=@TargetDir@",
                               "description=卸载 SmileCodeIDE");
    }
}

