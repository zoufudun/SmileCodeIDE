import { useRef, useCallback, useEffect, useState } from 'react';
import { useWebSocketStore } from '../../store/useWebSocketStore';
import { useLayoutStore } from '../../store/useLayoutStore';
import { BackgroundTemplate } from '../rooms/BackgroundTemplate';
import { RoomBox } from '../rooms/RoomBox';
import { DeviceCard } from './DeviceCard';
import { UnplacedDock } from './UnplacedDock';
import { useRegionContainment } from '../../hooks/useRegionContainment';
import { useDragAndDrop } from '../../hooks/useDragAndDrop';
import styles from './DeviceCanvas.module.css';

export function DeviceCanvas() {
  const latestMappings = useWebSocketStore((s) => s.latestMappings);
  const isEditMode = useLayoutStore((s) => s.isEditMode);
  const rooms = useLayoutStore((s) => s.rooms);
  const devicePositions = useLayoutStore((s) => s.devicePositions);
  const backgroundTemplate = useLayoutStore((s) => s.backgroundTemplate);
  const setDevicePosition = useLayoutStore((s) => s.setDevicePosition);

  // 拖拽 hook
  const { createDragHandler } = useDragAndDrop(
    '#device-grid',
    (id, x, y) => setDevicePosition(Number(id), x, y)
  );

  // 卡片引用，用于获取实际 DOM 尺寸
  const cardRefs = useRef<Record<number, HTMLDivElement | null>>({});

  // 将设备分为已摆放和未摆放
  const placedDevices = latestMappings.filter((d) => devicePositions[d.deviceId]);
  const unplacedDevices = latestMappings.filter((d) => !devicePositions[d.deviceId]);

  // 构建房间包含关系所需的卡片数据
  const [cardInfos, setCardInfos] = useState<
    Array<{ x: number; y: number; w: number; h: number; isAlarming: boolean }>
  >([]);

  useEffect(() => {
    const infos = placedDevices.map((d) => {
      const pos = devicePositions[d.deviceId];
      const el = cardRefs.current[d.deviceId];
      return {
        x: pos.x,
        y: pos.y,
        w: el?.offsetWidth || 135,
        h: el?.offsetHeight || 155,
        isAlarming: d.status,
      };
    });
    setCardInfos(infos);
  }, [placedDevices, devicePositions]);

  // 计算房间包含关系
  const containment = useRegionContainment(cardInfos, rooms);

  const setCardRef = useCallback((deviceId: number) => (el: HTMLDivElement | null) => {
    cardRefs.current[deviceId] = el;
  }, []);

  return (
    <>
      <div id="device-grid" className={styles.canvas}>
        {/* SVG 背景模板 */}
        {backgroundTemplate && <BackgroundTemplate type={backgroundTemplate} />}

        {/* 房间 */}
        {rooms.map((room) => (
          <RoomBox
            key={room.id}
            room={room}
            deviceCount={containment[room.id]?.count || 0}
            hasAlarm={containment[room.id]?.hasAlarm || false}
          />
        ))}

        {/* 已摆放的设备卡片 */}
        {placedDevices.map((device) => {
          const pos = devicePositions[device.deviceId];
          return (
            <div
              key={device.deviceId}
              ref={setCardRef(device.deviceId)}
              style={{ position: 'absolute', left: pos.x, top: pos.y }}
              data-id={device.deviceId}
              {...(isEditMode ? createDragHandler(cardRefs.current[device.deviceId]!).onMouseDown
                ? { onMouseDown: (e: React.MouseEvent) => {
                    // 需要从 ref 获取最新的元素
                    if (cardRefs.current[device.deviceId]) {
                      const handler = createDragHandler(cardRefs.current[device.deviceId]!);
                      handler.onMouseDown(e);
                    }
                  }}
                : {}
                : {}
              )}
            >
              <DeviceCard device={device} absolute />
            </div>
          );
        })}
      </div>

      {/* 未摆放设备 Dock */}
      <UnplacedDock devices={unplacedDevices} />
    </>
  );
}
