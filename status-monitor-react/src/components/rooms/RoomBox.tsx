import { useRef, useState } from 'react';
import type { Room } from '../../types/layout';
import { useLayoutStore } from '../../store/useLayoutStore';
import { useDragAndDrop } from '../../hooks/useDragAndDrop';
import { RoomResizeHandles } from './RoomResizeHandles';
import styles from './RoomBox.module.css';

interface RoomBoxProps {
  room: Room;
  deviceCount: number;
  hasAlarm: boolean;
}

export function RoomBox({ room, deviceCount, hasAlarm }: RoomBoxProps) {
  const ref = useRef<HTMLDivElement>(null);
  const isEditMode = useLayoutStore((s) => s.isEditMode);
  const renameRoom = useLayoutStore((s) => s.renameRoom);
  const removeRoom = useLayoutStore((s) => s.removeRoom);
  const updateRoomGeometry = useLayoutStore((s) => s.updateRoomGeometry);

  const [editing, setEditing] = useState(false);
  const [editValue, setEditValue] = useState(room.name);

  // 房间拖拽
  const { createDragHandler } = useDragAndDrop(
    '#device-grid',
    (id, x, y) => updateRoomGeometry(id, { x, y }),
    { isRoom: true }
  );

  const handleRename = () => {
    const newName = editValue.trim();
    if (newName) {
      renameRoom(room.id, newName);
    } else {
      setEditValue(room.name);
    }
    setEditing(false);
  };

  return (
    <div
      ref={ref}
      id={`room-box-${room.id}`}
      data-id={room.id}
      className={`${styles.box} ${hasAlarm ? styles.alarmActive : ''}`}
      style={{ left: room.x, top: room.y, width: room.w, height: room.h }}
    >
      {/* 拖拽手柄（仅 header 可拖拽） */}
      <div
        className={styles.header}
        {...(isEditMode ? createDragHandler(ref.current!).onMouseDown ? { onMouseDown: (e: React.MouseEvent) => {
          const handler = createDragHandler(ref.current!);
          handler.onMouseDown(e);
        }} : {} : {})}
      >
        <div className={styles.titleArea}>
          {editing ? (
            <input
              className={styles.titleInput}
              value={editValue}
              onChange={(e) => setEditValue(e.target.value)}
              onBlur={handleRename}
              onKeyDown={(e) => { if (e.key === 'Enter') handleRename(); }}
              autoFocus
              onClick={(e) => e.stopPropagation()}
            />
          ) : (
            <span
              className={styles.title}
              onDoubleClick={() => { if (isEditMode) { setEditValue(room.name); setEditing(true); } }}
            >
              {room.name}
            </span>
          )}
          <span className={styles.badge}>{deviceCount}</span>
        </div>

        {isEditMode && (
          <div className={styles.actions}>
            <button
              className={styles.btnRename}
              title="重命名"
              onClick={(e) => { e.stopPropagation(); setEditValue(room.name); setEditing(true); }}
            >
              ✎
            </button>
            <button
              className={styles.btnDelete}
              title="删除"
              onClick={(e) => {
                e.stopPropagation();
                if (confirm(`确定要删除房间 "${room.name}" 吗？该房间内的设备卡片不会被删除。`)) {
                  removeRoom(room.id);
                }
              }}
            >
              ✕
            </button>
          </div>
        )}
      </div>

      <RoomResizeHandles roomId={room.id} roomRef={ref} />
    </div>
  );
}
