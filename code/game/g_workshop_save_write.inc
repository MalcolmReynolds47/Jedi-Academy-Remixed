// Production save dispatch shared with its round-trip regression. GPL-2.0-only.
void G_JAR_WorkshopWrite(bool autosave,int version){
 if(version!=jar::workshopSaveCurrent)return;
 if(prepared.empty())prepared=Encode(state,autosave);
 ojk::SavedGameHelper h(gi.saved_game);
 h.write_chunk<uint32_t>(INT_ID('J','W','S','Z'),uint32_t(prepared.size()));
 h.write_chunk<uint8_t>(INT_ID('J','W','S','T'),prepared.data(),int(prepared.size()));
 nativeSave(autosave,h);
 h.reset_buffer();G_MusicExport(autosave,h);h.write_chunk(INT_ID('J','M','U','S'));
 h.reset_buffer();G_ShipBoundaryExport(autosave,h);h.write_chunk(INT_ID('J','S','H','P'));
 h.reset_buffer();G_JAR_MoverMotionExport(autosave,h);h.write_chunk(INT_ID('J','M','F','R'));
 h.reset_buffer();G_SkyPortalExport(autosave,h);h.write_chunk(INT_ID('J','S','K','Y'));
 h.reset_buffer();G_SubBSPOwnershipExport(autosave,h);h.write_chunk(INT_ID('J','B','S','O'));
 h.reset_buffer();G_ContactClocksExport(autosave,h);h.write_chunk(INT_ID('J','H','R','T'));
 h.reset_buffer();G_TeleportArrivalsExport(autosave,h);h.write_chunk(INT_ID('J','T','A','R'));
 h.reset_buffer();G_VehiclePushExport(autosave,h);h.write_chunk(INT_ID('J','V','P','D'));
 h.reset_buffer();G_SandboxChargesExport(autosave,h);h.write_chunk(INT_ID('J','C','H','G'));
 h.reset_buffer();writeUndoHistory(autosave,h);h.write_chunk(INT_ID('J','U','N','D'));
 prepared.clear();
}
