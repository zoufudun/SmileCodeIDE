import { useRoomResize } from '../../hooks/useRoomResize';
import { useLayoutStore } from '../../store/useLayoutStore';
import styles from './RoomResizeHandles.module.css';

interface RoomResizeHandlesProps {
  roomId: string;
  roomRef: React.RefObject<HTMLDivElement | null>;
}

type ResizeDir = 't' | 'b' | 'l' | 'r' | 'se';

const HANDLES: { dir: ResizeDir; className: string }[] = [
  { dir: 't', className: 'top' },
  { dir: 'b', className: 'bottom' },
  { dir: 'l', className: 'left' },
  { dir: 'r', className: 'right' },
  { dir: 'se', className: 'se' },
];

export function RoomResizeHandles({ roomId, roomRef }: RoomResizeHandlesProps) {
  const isEditMode = useLayoutStore((s) => s.isEditMode);
  const updateRoomGeometry = useLayoutStore((s) => s.updateRoomGeometry);

  const { onResizeStart } = useRoomResize('#device-grid', (id, partial) => {
    updateRoomGeometry(id, partial);
  });

  if (!isEditMode) return null;

  return (
    <>
      {HANDLES.map(({ dir, className }) => (
        <div
          key={dir}
          className={styles[className]}
          onMouseDown={(e) => {
            if (roomRef.current) {
              onResizeStart(e, dir, roomRef.current);
            }
          }}
        />
      ))}
    </>
  );
}
