#include "MenuModel.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"

juce::StringArray MenuModel::getMenuBarNames() {
  return {juce::CharPointer_UTF8("File..."),
          juce::CharPointer_UTF8("Export...")};
}

void MenuModel::menuItemSelected(int menuItemID, int topLevelMenuIndex) {
  if (topLevelMenuIndex == 0 && menuItemID == 1) {
    onLoadSoundFileClick();
    return;
  }

  if (topLevelMenuIndex == 1 && menuItemID == 2) {
    onExportParametersClick();
    return;
  }
}

juce::PopupMenu MenuModel::getMenuForIndex(int topLevelMenuIndex,
                                           const juce::String &menuName) {
  juce::PopupMenu popupMenu;

  if (topLevelMenuIndex == 0) {
    popupMenu.addItem(1, juce::CharPointer_UTF8("Load..."));
    return popupMenu;
  }

  if (topLevelMenuIndex == 1) {
    popupMenu.addItem(2, juce::CharPointer_UTF8("Save parameters as TXT..."));
    return popupMenu;
  }

  return popupMenu;
}
