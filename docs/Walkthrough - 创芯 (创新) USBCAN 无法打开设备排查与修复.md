# Walkthrough - 创芯 (创新) USBCAN 无法打开设备排查与修复

已完成创芯 (Innovation) USBCAN 无法打开设备问题的底层驱动加载逻辑增强与诊断改进。

---

## 🛠️ 主要变更与修改说明

### 1. `caninterface.h` & `caninterface.cpp`
- **提前添加动态库搜索目录**：
  - 在 `CanInterface::loadLibrary()` 阶段即配置 Windows `SetDllDirectoryW`（包含可执行文件同级目录及 `CXCAN/` 子目录），解决 `ControlCAN.dll` 加载从属依赖库失败的问题。
- **句柄自动复位与打开重试**：
  - 调用 `vciOpenDevice` 前，优先执行 `vciCloseDevice` 复位先前异常退出遗留的残留句柄。
  - 支持类型码双向重试：优先使用映射类型码（如 `USBCAN-2C` -> `4`），失败时自动尝试原始类型码（`104`）。
- **双驱动降级与兜底**：
  - 当在 UI 中选择“USBCAN-2”或“USBCAN-1”（经典 ZLG 类型码 4/3）但通过 `zlgcan.dll` 无法打开时，自动降级尝试调用创芯 `ControlCAN.dll` 打开，避免因误选“USBCAN-2”导致创芯硬件打不开。
- **引入 `lastError()` 诊断信息**：
  - 记录详细的硬件/驱动排查建议（USB 连接、驱动安装、端口占用等）。

### 2. `candevicedialog.cpp`
- **优化 UI 错误弹窗**：
  - 打开设备失败时，从 `m_can->lastError()` 提取具体原因呈现在弹窗中，使用户能直观定位问题。

---

## 🎯 验证说明
- 已更新源码并启动自动化构建编译验证。
