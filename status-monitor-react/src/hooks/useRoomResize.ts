import { useCallback } from 'react';
import { useLayoutStore } from '../store/useLayoutStore';
import { snapToGrid } from '../utils/layout-helpers';
import { ROOM_MIN_WIDTH, ROOM_MIN_HEIGHT } from '../constants/defaults';

type ResizeDirection = 't' | 'b' | 'l' | 'r' | 'se';

/**
 * 房间缩放 hook（四方向 + SE 角）
 * @param canvasSelector 画布容器选择器
 * @param onResizeEnd 缩放结束回调 (id, x, y, w, h)
 */
export function useRoomResize(
  canvasSelector: string,
  onResizeEnd: (id: string, partial: { x?: number; y?: number; w?: number; h?: number }) => void
) {
  const gridSnapSize = useLayoutStore((s) => s.gridSnapSize);

  const onResizeStart = useCallback(
    (e: React.MouseEvent, direction: ResizeDirection, roomElement: HTMLElement) => {
      e.preventDefault();
      e.stopPropagation();

      const startWidth = roomElement.offsetWidth;
      const startHeight = roomElement.offsetHeight;
      const startLeft = roomElement.offsetLeft;
      const startTop = roomElement.offsetTop;
      const startMouseX = e.clientX;
      const startMouseY = e.clientY;

      const canvasEl = document.querySelector(canvasSelector);
      const canvasW = canvasEl ? canvasEl.clientWidth : 9999;
      const canvasH = canvasEl ? canvasEl.clientHeight : 9999;

      const onMouseMove = (moveEvent: MouseEvent) => {
        const dx = moveEvent.clientX - startMouseX;
        const dy = moveEvent.clientY - startMouseY;

        let newW = startWidth;
        let newH = startHeight;
        let newL = startLeft;
        let newT = startTop;

        if (direction === 'r' || direction === 'se') {
          newW = snapToGrid(startWidth + dx, gridSnapSize);
          newW = Math.max(ROOM_MIN_WIDTH, Math.min(newW, canvasW - startLeft));
        }
        if (direction === 'b' || direction === 'se') {
          newH = snapToGrid(startHeight + dy, gridSnapSize);
          newH = Math.max(ROOM_MIN_HEIGHT, Math.min(newH, canvasH - startTop));
        }
        if (direction === 'l') {
          newW = snapToGrid(startWidth - dx, gridSnapSize);
          if (newW >= ROOM_MIN_WIDTH) {
            newL = startLeft + (startWidth - newW);
          } else {
            newW = ROOM_MIN_WIDTH;
            newL = startLeft + (startWidth - ROOM_MIN_WIDTH);
          }
          newL = Math.max(0, newL);
        }
        if (direction === 't') {
          newH = snapToGrid(startHeight - dy, gridSnapSize);
          if (newH >= ROOM_MIN_HEIGHT) {
            newT = startTop + (startHeight - newH);
          } else {
            newH = ROOM_MIN_HEIGHT;
            newT = startTop + (startHeight - ROOM_MIN_HEIGHT);
          }
          newT = Math.max(0, newT);
        }

        roomElement.style.width = newW + 'px';
        roomElement.style.height = newH + 'px';
        roomElement.style.left = newL + 'px';
        roomElement.style.top = newT + 'px';
      };

      const onMouseUp = () => {
        const id = roomElement.dataset.id;
        if (id) {
          onResizeEnd(id, {
            x: roomElement.offsetLeft,
            y: roomElement.offsetTop,
            w: roomElement.offsetWidth,
            h: roomElement.offsetHeight,
          });
        }
        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);
      };

      document.addEventListener('mousemove', onMouseMove);
      document.addEventListener('mouseup', onMouseUp);
    },
    [gridSnapSize, canvasSelector, onResizeEnd]
  );

  return { onResizeStart };
}
