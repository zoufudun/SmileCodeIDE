import { create } from 'zustand';
import { FOOTER_DEFAULT_HEIGHT } from '../constants/defaults';

interface UIState {
  footerHeight: number;
  footerVisible: boolean;
  infoDrawerOpen: boolean;
  modalAddRoomOpen: boolean;
  newRoomName: string;

  setFooterHeight: (h: number) => void;
  toggleFooter: () => void;
  setFooterVisible: (visible: boolean) => void;
  toggleInfoDrawer: () => void;
  setInfoDrawerOpen: (open: boolean) => void;
  setModalAddRoomOpen: (open: boolean) => void;
  setNewRoomName: (name: string) => void;
}

export const useUIStore = create<UIState>((set) => ({
  footerHeight: FOOTER_DEFAULT_HEIGHT,
  footerVisible: true,
  infoDrawerOpen: false,
  modalAddRoomOpen: false,
  newRoomName: '',

  setFooterHeight: (h) => set({ footerHeight: h }),
  toggleFooter: () => set((s) => ({ footerVisible: !s.footerVisible })),
  setFooterVisible: (visible) => set({ footerVisible: visible }),
  toggleInfoDrawer: () => set((s) => ({ infoDrawerOpen: !s.infoDrawerOpen })),
  setInfoDrawerOpen: (open) => set({ infoDrawerOpen: open }),
  setModalAddRoomOpen: (open) => set({ modalAddRoomOpen: open }),
  setNewRoomName: (name) => set({ newRoomName: name }),
}));
