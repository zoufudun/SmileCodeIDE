import type { LayoutData } from '../types/layout';

/** 核潜艇布局模板 */
export const submarineTemplate: LayoutData = {
  backgroundTemplate: 'submarine',
  rooms: [
    { id: 'sub_1', name: '舱首/鱼雷舱', x: 150, y: 300, w: 180, h: 220 },
    { id: 'sub_2', name: '指挥与战术中心', x: 400, y: 220, w: 220, h: 300 },
    { id: 'sub_3', name: '生活休息舱', x: 680, y: 300, w: 180, h: 220 },
    { id: 'sub_4', name: '反应堆舱区', x: 900, y: 280, w: 180, h: 240 },
    { id: 'sub_5', name: '动力/推进舱', x: 1120, y: 300, w: 180, h: 220 },
  ],
  devices: {},
};

/** 写字楼布局模板 */
export const buildingTemplate: LayoutData = {
  backgroundTemplate: 'building',
  rooms: [
    { id: 'bld_3', name: '3F - 云数据机房', x: 100, y: 110, w: 600, h: 100 },
    { id: 'bld_2', name: '2F - 行政与会议中心', x: 100, y: 225, w: 600, h: 100 },
    { id: 'bld_1', name: '1F - 研发测试中心', x: 100, y: 340, w: 600, h: 100 },
  ],
  devices: {},
};

/** 水面战舰布局模板 */
export const warshipTemplate: LayoutData = {
  backgroundTemplate: 'warship',
  rooms: [
    { id: 'ship_1', name: '舰艏武器库区', x: 60, y: 180, w: 200, h: 180 },
    { id: 'ship_2', name: '舰桥驾驶控制舱', x: 280, y: 100, w: 240, h: 260 },
    { id: 'ship_3', name: '舰舯机电舱室', x: 540, y: 180, w: 200, h: 180 },
    { id: 'ship_4', name: '舰艉直升机库', x: 760, y: 160, w: 200, h: 200 },
  ],
  devices: {},
};

/** 模板名称映射 */
export const TEMPLATE_MAP: Record<string, LayoutData> = {
  submarine: submarineTemplate,
  building: buildingTemplate,
  warship: warshipTemplate,
};
