# 创建图标目录
$iconDir = "e:\QtHub\vue\STM32IDE\icons"
if (-not (Test-Path $iconDir)) {
    New-Item -ItemType Directory -Path $iconDir
}

# 图标URL列表 (使用GitHub上的Material Design Icons)
$icons = @{
    "horizontal_split" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/editor/horizontal_split/materialicons/24dp/2x/baseline_horizontal_split_black_24dp.png"
    "vertical_split" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/editor/vertical_split/materialicons/24dp/2x/baseline_vertical_split_black_24dp.png"
    "close" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/content/clear/materialicons/24dp/2x/baseline_clear_black_24dp.png"
    "undo" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/content/undo/materialicons/24dp/2x/baseline_undo_black_24dp.png"
    "redo" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/content/redo/materialicons/24dp/2x/baseline_redo_black_24dp.png"
    "cut" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/content/content_cut/materialicons/24dp/2x/baseline_content_cut_black_24dp.png"
    "copy" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/content/content_copy/materialicons/24dp/2x/baseline_content_copy_black_24dp.png"
    "paste" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/content/content_paste/materialicons/24dp/2x/baseline_content_paste_black_24dp.png"
    "find" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/action/search/materialicons/24dp/2x/baseline_search_black_24dp.png"
    "replace" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/action/find_replace/materialicons/24dp/2x/baseline_find_replace_black_24dp.png"
    "indent" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/editor/format_indent_increase/materialicons/24dp/2x/baseline_format_indent_increase_black_24dp.png"
    "unindent" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/editor/format_indent_decrease/materialicons/24dp/2x/baseline_format_indent_decrease_black_24dp.png"
    "comment" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/editor/insert_comment/materialicons/24dp/2x/baseline_insert_comment_black_24dp.png"
    "uncomment" = "https://raw.githubusercontent.com/google/material-design-icons/master/png/communication/clear_all/materialicons/24dp/2x/baseline_clear_all_black_24dp.png"
}

# 下载图标
foreach ($icon in $icons.GetEnumerator()) {
    $outFile = Join-Path $iconDir "$($icon.Key).png"
    Write-Host "下载图标: $($icon.Key) 到 $outFile"
    Invoke-WebRequest -Uri $icon.Value -OutFile $outFile
}

Write-Host "所有图标已下载到 $iconDir"