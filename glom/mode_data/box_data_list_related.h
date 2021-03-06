/* Glom
 *
 * Copyright (C) 2001-2004 Murray Cumming
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

#ifndef GLOM_MODE_DATA_BOX_DATA_LIST_RELATED_H
#define GLOM_MODE_DATA_BOX_DATA_LIST_RELATED_H

#include "config.h" // GLOM_ENABLE_CLIENT_ONLY

#include "box_data_portal.h"

namespace Glom
{

class Box_Data_List_Related : public Box_Data_Portal
{
public:
  Box_Data_List_Related();

  bool init_db_details(const std::shared_ptr<const LayoutItem_Portal>& portal, bool show_title) override;

  /** Use this if no portal is yet defined, so the user can use the context menu to define a portal.
   */
  bool init_db_details_without_portal(const Glib::ustring &parent_table) override;

  void set_find_mode(bool val = true) override;

protected:
  bool fill_from_database() override;

  void show_title_in_ui(const Glib::ustring& title) override;

  //Signal handlers:
  void on_adddel_record_changed();
  void on_adddel_user_requested_edit(const Gtk::TreeModel::iterator& row);

  void on_adddel_script_button_clicked(const std::shared_ptr<const LayoutItem_Button>& layout_item, const Gtk::TreeModel::iterator& row);
  bool on_script_button_idle(const std::shared_ptr<const LayoutItem_Button>& layout_item, const Gnome::Gda::Value& primary_key);

  void on_adddel_record_added(const Gtk::TreeModel::iterator& row, const Gnome::Gda::Value& primary_key_value);

#ifndef GLOM_ENABLE_CLIENT_ONLY
  void on_adddel_user_requested_layout();
#endif // !GLOM_ENABLE_CLIENT_ONLY

#ifndef GLOM_ENABLE_CLIENT_ONLY
  void on_dialog_layout_hide() override;
#endif // !GLOM_ENABLE_CLIENT_ONLY

  //Implementations of pure virtual methods from Base_DB_Table_Data:
  std::shared_ptr<Field> get_field_primary_key() const override; //TODO: Already in base class?
  Gnome::Gda::Value get_primary_key_value_selected() const override;
  void set_primary_key_value(const Gtk::TreeModel::iterator& row, const Gnome::Gda::Value& value) override;
  Gnome::Gda::Value get_primary_key_value(const Gtk::TreeModel::iterator& row) const override;

  //Overrides of functions from Box_Data:
  Document::type_list_layout_groups create_layout_get_layout() override;
  void create_layout() override;

  void enable_buttons();

#ifndef GLOM_ENABLE_CLIENT_ONLY
  Dialog_Layout* create_layout_dialog() const override;
  void prepare_layout_dialog(Dialog_Layout* dialog) override;
#endif // !GLOM_ENABLE_CLIENT_ONLY


  //Member widgets:
  mutable DbAddDel_WithButtons m_AddDel; //mutable because its get_ methods aren't const.

  bool m_read_only;
};

} //namespace Glom

#endif // GLOM_MODE_DATA_BOX_DATA_LIST_RELATED_H
