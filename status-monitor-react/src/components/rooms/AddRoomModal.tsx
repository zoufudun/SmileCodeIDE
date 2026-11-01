import { useState, useEffect } from 'react';
import { Modal } from '../common/Modal';
import { useUIStore } from '../../store/useUIStore';
import { useLayoutStore } from '../../store/useLayoutStore';
import { useLogStore } from '../../store/useLogStore';
import styles from './AddRoomModal.module.css';

export function AddRoomModal() {
  const modalAddRoomOpen = useUIStore((s) => s.modalAddRoomOpen);
  const setModalAddRoomOpen = useUIStore((s) => s.setModalAddRoomOpen);
  const rooms = useLayoutStore((s) => s.rooms);
  const addRoom = useLayoutStore((s) => s.addRoom);
  const appendLog = useLogStore((s) => s.appendLog);

  const [name, setName] = useState('');

  useEffect(() => {
    if (modalAddRoomOpen) {
      setName(`房间 ${rooms.length + 1}`);
    }
  }, [modalAddRoomOpen, rooms.length]);

  const handleConfirm = () => {
    const trimmed = name.trim();
    if (!trimmed) {
      alert('请输入有效的房间名称！');
      return;
    }
    const room = addRoom(trimmed);
    appendLog(`✓ 成功创建房间: "${trimmed}"`, 'system');
    setModalAddRoomOpen(false);
  };

  const handleClose = () => {
    setModalAddRoomOpen(false);
  };

  return (
    <Modal open={modalAddRoomOpen} title="＋ 新建房间" onClose={handleClose}>
      <div className={styles.body}>
        <div className={styles.formGroup}>
          <label htmlFor="input-room-name">房间名称</label>
          <input
            id="input-room-name"
            type="text"
            className={styles.input}
            value={name}
            onChange={(e) => setName(e.target.value)}
            placeholder="例如：1F - 研发测试中心"
            autoFocus
            onKeyDown={(e) => { if (e.key === 'Enter') handleConfirm(); }}
          />
        </div>
      </div>
      <div className={styles.footer}>
        <button className={`${styles.btn} ${styles.secondary}`} onClick={handleClose}>
          取消
        </button>
        <button className={`${styles.btn} ${styles.primary}`} onClick={handleConfirm}>
          创建
        </button>
      </div>
    </Modal>
  );
}
