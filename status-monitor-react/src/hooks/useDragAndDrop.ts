import { useCallback, useRef } from 'react';
import { useLayoutStore } from '../store/useLayoutStore';
import { snapToGrid, clampPosition } from '../utils/layout-helpers';

interface DragState {
  startX: number;
  startY: number;
  itemStartX: number;
  itemStartY: number;
  element: HTMLElement | null;
}

/**
 * 通用拖拽 hook，同时支持设备卡片和房间拖拽
 *
 * @param gridSelector 画布容器的选择器（用于获取边界），如 '#device-grid'
 * @param onDragEnd 拖拽结束回调：设备卡片调用 setDevicePosition，房间调用 updateRoomGeometry
 * @param options.isRoom 是否为房间拖拽（房间只通过 header 拖拽，设备卡片整卡可拖拽）
 */
export function useDragAndDrop(
  gridSelector: string,
  onDragEnd: (id: string, x: number, y: number) => void,
  options?: { isRoom?: boolean }
) {
  const gridSnapSize = useLayoutStore((s) => s.gridSnapSize);
  const isEditMode = useLayoutStore((s) => s.isEditMode);
  const dragRef = useRef<DragState>({
    startX: 0, startY: 0, itemStartX: 0, itemStartY: 0, element: null,
  });

  const onMouseDown = useCallback(
    (e: React.MouseEvent, element: HTMLElement) => {
      if (!isEditMode) return;

      // 房间拖拽时，点击按钮或标题文字不触发拖拽
      if (options?.isRoom) {
        const target = e.target as HTMLElement;
        if (
          target.tagName === 'BUTTON' ||
          target.classList.contains('room-title') ||
          target.classList.contains('room-badge')
        ) {
          return;
        }
      }

      e.preventDefault();

      const state = dragRef.current;
      state.startX = e.clientX;
      state.startY = e.clientY;
      state.element = element;

      const currentLeft = parseInt(element.style.left) || 0;
      const currentTop = parseInt(element.style.top) || 0;
      state.itemStartX = currentLeft;
      state.itemStartY = currentTop;

      const onMouseMove = (moveEvent: MouseEvent) => {
        const dx = moveEvent.clientX - state.startX;
        const dy = moveEvent.clientY - state.startY;

        let newX = state.itemStartX + dx;
        let newY = state.itemStartY + dy;

        // 网格吸附
        newX = snapToGrid(newX, gridSnapSize);
        newY = snapToGrid(newY, gridSnapSize);

        // 边界约束
        const grid = document.querySelector(gridSelector);
        if (grid && state.element) {
          const canvasW = grid.clientWidth;
          const canvasH = grid.clientHeight;
          const itemW = state.element.offsetWidth;
          const itemH = state.element.offsetHeight;
          const clamped = clampPosition(newX, newY, itemW, itemH);
          newX = clamped.x;
          newY = clamped.y;
        }

        if (state.element) {
          state.element.style.left = newX + 'px';
          state.element.style.top = newY + 'px';
        }
      };

      const onMouseUp = () => {
        if (state.element) {
          const id = state.element.dataset.id;
          const x = parseInt(state.element.style.left) || 0;
          const y = parseInt(state.element.style.top) || 0;
          if (id) onDragEnd(id, x, y);
        }
        state.element = null;
        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);
      };

      document.addEventListener('mousemove', onMouseMove);
      document.addEventListener('mouseup', onMouseUp);
    },
    [isEditMode, gridSnapSize, gridSelector, onDragEnd, options]
  );

  /** 创建拖拽处理器，绑定到指定元素 */
  const createDragHandler = useCallback(
    (element: HTMLElement) => ({
      onMouseDown: (e: React.MouseEvent) => onMouseDown(e, element),
    }),
    [onMouseDown]
  );

  return { createDragHandler, isEditMode };
}
