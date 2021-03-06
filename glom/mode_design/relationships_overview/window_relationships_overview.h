/* Glom
 *
 * Copyright (C) 2001-2013 Murray Cumming
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#ifndef GLOM_WINDOW_RELATIONSHIPS_OVERVIEW
#define GLOM_WINDOW_RELATIONSHIPS_OVERVIEW

#include <libglom/document/view.h>
#include "glom/utility_widgets/canvas/canvas_editable.h"
#include "canvas_group_dbtable.h"
#include <gtkmm/dialog.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/menubar.h>
#include <gtkmm/printoperation.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/toggleaction.h>
#include <gtkmm/builder.h>
#include <glibmm/weakref.h>
#include <goocanvasmm/canvas.h>
#include <unordered_map>
#include <vector>

//#include "relationships_canvas.h"

namespace Glom
{

class Window_RelationshipsOverview
 : public Gtk::ApplicationWindow,
   public View_Composite_Glom
{
public:
  static const char* glade_id;
  static const bool glade_developer;

  Window_RelationshipsOverview(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~Window_RelationshipsOverview();

  void load_from_document() override;

private:
  class TableView;

  void draw_tables();
  void draw_lines();
  void setup_context_menu();
  void print_or_preview(Gtk::PrintOperationAction print_action);
  void on_button_close();

  void on_menu_file_print();
  void on_menu_file_page_setup();
  void on_menu_file_save();
  void on_menu_view_showgrid(const Glib::VariantBase& /* parameter */);

  void on_table_moved(const Glib::RefPtr<CanvasItemMovable>& item, double x_offset, double y_offset);
  void on_table_show_context(GdkEventButton* event, const Glib::WeakRef<CanvasGroupDbTable>& table);

  void on_context_menu_edit_fields(const Glib::VariantBase& parameter, const Glib::WeakRef<CanvasGroupDbTable>& table);
  void on_context_menu_edit_relationships(const Glib::VariantBase& parameter, const Glib::WeakRef<CanvasGroupDbTable>& table);

  void on_scroll_value_changed();

  Glib::RefPtr<CanvasGroupDbTable> get_table_group(const Glib::ustring& table_name);

  Glib::RefPtr<Gtk::Builder> m_builder;
  Glib::RefPtr<Gio::SimpleAction> m_action_showgrid;

  bool m_modified;
  CanvasEditable m_canvas;
  Gtk::ScrolledWindow* m_scrolledwindow_canvas;

  //typedef std::unordered_map<Glib::RefPtr<Goocanvas::Item>, TableView*> type_map_item_tables;
  //type_map_item_tables m_tables;

  static int m_last_size_x, m_last_size_y;

  Glib::RefPtr<Goocanvas::Group> m_group_tables;
  Glib::RefPtr<Goocanvas::Group> m_group_lines;

  //Context menu:
  std::unique_ptr<Gtk::Menu> m_context_menu;
  Glib::RefPtr<Gio::SimpleAction> m_action_edit_fields, m_action_edit_relationships;
  sigc::connection m_connection_edit_fields, m_connection_edit_relationships;

  //Printing:
  Glib::RefPtr<Gtk::PrintSettings> m_settings;
  Glib::RefPtr<Gtk::PageSetup> m_page_setup;
};

} //namespace Glom

#endif //GLOM_WINDOW_RELATIONSHIPS_OVERVIEW
