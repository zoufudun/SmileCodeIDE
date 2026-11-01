# Electron 监控端二期优化与功能增强方案

根据您的反馈，我们将进一步完善和优化界面功能，具体包括：
1. **房间名称修改**：双击房间标题或点击编辑按钮即可修改房间名。
2. **规范汉化翻译日志**：使右侧“信息日志”中的文本完全匹配所要求的翻译模板。
3. **极简扫把清除图标**：使用 `🧹`（或极简扫把图标）来表示日志清空按钮。
4. **可设定的布局网格 (Grid)**：新增网格吸附与对齐尺寸设置（如：自由/5px/10px/20px），支持动态调节网格吸附大小与网格图层显示。
5. **多套热门主题切换**：提供“科技蓝 (默认)”、“翡翠绿 (安全)”、“琥珀金 (工业)”和“烈焰红 (警报)”多套精品主题色彩。

---

## 拟修改的内容 (Proposed Changes)

### 1. 样式与主题层 [style.css](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/style.css)

#### [MODIFY] [style.css](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/style.css)
- **主题 CSS 变量提取**：在 `:root` 中引入 `--primary-color`（主要前景色）、`--primary-glow`（发光色）、`--primary-border`（边框色）和 `--bg-color`（页面背景色）。
- **定义主题样式集**：
  - `body.theme-cyber`（默认科技蓝）：使用 `#00f0ff`
  - `body.theme-emerald`（翡翠绿安全风）：使用 `#10b981`
  - `body.theme-amber`（琥珀金工业风）：使用 `#f59e0b`
  - `body.theme-ruby`（烈焰红高警警报风）：使用 `#ef4444`
- **更新旧样式**：将原 style.css 中所有的硬编码颜色（如 `#00f0ff` 等）替换为相应的 CSS 变量。
- **美化主题选择下拉框**：在导航栏右侧加入美观的下拉菜单 `.select-theme-skin`。
- **清空日志按钮美化**：重塑 `.btn-clear` 样式，适配简约图标按钮。

---

### 2. 结构层 [index.html](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/index.html)

#### [MODIFY] [index.html](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/index.html)
- **导航栏**：
  - 增加主题选择下拉框：
    ```html
    <select id="select-theme-skin" class="select-theme-skin">
      <option value="cyber">🌌 科技蓝 (默认)</option>
      <option value="emerald">🟢 翡翠绿 (安全)</option>
      <option value="amber"> Amber 琥珀金 (工业)</option>
      <option value="ruby">🔴 烈焰红 (警报)</option>
    </select>
    ```
- **布局设置栏**：
  - 新增网格对齐下拉选择框：
    ```html
    <div class="toolbar-group">
      <span class="toolbar-label">📐 网格对齐:</span>
      <select id="select-grid-snap" class="select-layout-template">
        <option value="1">自由吸附</option>
        <option value="5">5 px</option>
        <option value="10" selected>10 px</option>
        <option value="20">20 px</option>
      </select>
    </div>
    ```
- **日志清除按钮**：
  - 将原底部的 `清空日志` 按钮文案更新为：`🧹 清空`
  - 将右侧信息日志抽屉的 `清空` 按钮文案更新为：`🧹 清空`

---

### 3. 逻辑控制层 [renderer.js](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/renderer.js)

#### [MODIFY] [renderer.js](file:///m:/TOPFIRE/SmileCodeIDEUpdate/status-monitor-electron/renderer.js)
- **修改房间名称 (Room Renaming)**：
  - 在 `createRoomElement` 中，除原有的编辑按钮外，为 `.room-title` 绑定双击事件 (`dblclick`)，允许直接双击房间标题唤起编辑，完成修改后重新触发状态分析，并可随着保存动作持久化到本地。
- **信息日志报文翻译汉化格式优化**：
  - 重新调整 `translateRawMessage(rawData)` 对 `update` 数据包的翻译逻辑，保证其输出格式如下：
    - **烟温探测器异常/恢复**：
      `📡 设备变化：烟温探测器 [标签] (异常报警 🚨) 状态变更为: 报警` / `状态变更为: 复位`
    - **控制分配阀开启/关闭**：
      `📡 设备变化：控制分配阀 [标签] (开启 🟢) 状态变更为: 开启` / `状态变更为: 关闭`
- **动态网格吸附控制**：
  - 绑定网格吸附下拉框 `select-grid-snap` 变更事件，在拖动卡片和房间时（`onMouseMove` / `onResizeMouseMove`），动态将位置取整基准设定为配置的像素大小。同时，当吸附像素变化时，自动调整背景网格的 `.style.backgroundSize`。
- **主题切换控制**：
  - 绑定主题选择框 `select-theme-skin` 的切换事件，在 `body` 节点动态移除旧的主题 class，并赋予新的主题 class（如 `theme-emerald`）。保存选择到 `localStorage` 中，以便下次打开自动应用。
- **SVG 素材升级**：
  - 将 preset background svg 里的 `#00f0ff` stroke 色改用 `var(--primary-color)` 动态注入，使其契合当前选中的主题颜色。

---

## 验证计划 (Verification Plan)

### 手动测试流程
1. **启动与验证主题**：
   - 启动 Electron 网关。
   - 切换右上角的主题下拉框，验证页面主题（文字、霓虹灯、背景色、卡片边框、背景轮廓线）是否完美变成翡翠绿、琥珀金、烈焰红。
2. **布局网格设置测试**：
   - 进入 `布局设置`，切换网格吸附（例如：5px，20px，自由）。
   - 拖拽卡片和房间，验证吸附是否严格按照设定像素运行。
3. **修改房间名称测试**：
   - 在编辑状态下，双击某房间标题，验证是否能输入新名称。
   - 点击修改并保存，重载页面检查修改是否持久化。
4. **日志翻译模板验证**：
   - 模拟数据发送，查看右侧抽屉式“信息日志”，检查文案是否与指定的“状态变更为: 报警/复位/开启/关闭”模板完全一致。
5. **清空日志扫把测试**：
   - 确认日志清除按钮是否带有 `🧹` 清除标识。
