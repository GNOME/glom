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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GLOM_UTILITY_WIDGETS_DBADDDELL_CELLRENDERDBERLIST_H
#define GLOM_UTILITY_WIDGETS_DBADDDELL_CELLRENDERDBERLIST_H

#include <glom/mode_data/datawidget/combochoiceswithtreemodel.h>
#include <gtkmm.h>
//#include <gtkmm/cellrenderercombo.h>
//#include <gtkmm/liststore.h>


namespace Glom
{

/** A CellRendererCombo to show database records.
 * For instance, to show related choices.
 */
class CellRendererDbList
 : public Gtk::CellRendererCombo,
   public DataWidgetChildren::ComboChoicesWithTreeModel
{
public:
  CellRendererDbList();
  virtual ~CellRendererDbList();

  void set_choices_with_second(const type_list_values_with_second& list_values);

  void set_restrict_values_to_list(bool val = true);

private:

  virtual void use_model();
  virtual void on_editing_started(Gtk::CellEditable* cell_editable, const Glib::ustring& path);

  virtual void set_value(const Gnome::Gda::Value& value);
  virtual Gnome::Gda::Value get_value() const;

  void set_text(const Glib::ustring& text);
  Glib::ustring get_text() const;
  
  bool m_repacked_first_cell;
};

} //namespace Glom

#endif //GLOM_UTILITY_WIDGETS_DBADDDELL_CELLRENDERDBERLIST_H
