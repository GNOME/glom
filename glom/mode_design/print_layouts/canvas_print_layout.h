/* Glom
 *
 * Copyright (C) 2007 Murray Cumming
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GLOM_CANVAS_PRINT_LAYOUT_EDIT_H
#define GLOM_CANVAS_PRINT_LAYOUT_EDIT_H

#include <glom/utility_widgets/canvas/canvas_editable.h>
#include <glom/mode_design/print_layouts/canvas_layout_item.h>
#include <glom/libglom/data_structure/print_layout.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/toggleaction.h>

namespace Glom
{

/// A canvas that contains CanvasLayoutItem items.
class Canvas_PrintLayout : public CanvasEditable
{
public:
  Canvas_PrintLayout();
  virtual ~Canvas_PrintLayout();

  void set_print_layout(const Glib::ustring& table_name, const sharedptr<const PrintLayout>& print_layout);
  sharedptr<PrintLayout> get_print_layout();

protected:

  void setup_context_menu();
  void add_layout_group(const sharedptr<LayoutGroup>& group);
  void add_layout_group_children(const sharedptr<LayoutGroup>& group);
  void fill_layout_group(const sharedptr<LayoutGroup>& group);

  void on_item_show_context_menu(guint button, guint32 activate_time, const Glib::RefPtr<CanvasLayoutItem>& item);
  void on_context_menu_delete(const Glib::RefPtr<CanvasLayoutItem>& item);

  bool m_modified;

  //Context menu for existing items:
  Gtk::Menu* m_context_menu;
  Glib::RefPtr<Gtk::ActionGroup> m_context_menu_action_group;
  Glib::RefPtr<Gtk::UIManager> m_context_menu_uimanager;

  Glib::RefPtr<Gtk::Action> m_action_delete;
  sigc::connection m_connection_delete; 
};

} //namespace Glom

#endif //GLOM_CANVAS_PRINT_LAYOUT_EDIT_H

