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

#ifndef GLOM_UTILITYWIDGETS_FLOWTABLE_H
#define GLOM_UTILITYWIDGETS_FLOWTABLE_H

#include <gtkmm.h>
#include "layoutwidgetbase.h"

namespace Glom
{

class FlowTable : public Gtk::SpreadTable
{
public:
  FlowTable();
  virtual ~FlowTable();

  typedef Gtk::Container type_base;

  virtual void add(Gtk::Widget& first, Gtk::Widget& second, bool expand_second = false);
  virtual void add(Gtk::Widget& first, bool expand = false); //override
  void insert_before(Gtk::Widget& first, Gtk::Widget& second, Gtk::Widget& before, bool expand_second);
  void insert_before(Gtk::Widget& first, Gtk::Widget& before, bool expand);

  /** Show extra UI that is useful in RAD tools:
   */
  virtual void set_design_mode(bool value = true);

  void remove_all();

protected:

  /** Get the column in which the specified "first" widget is placed.
   * result false if the widget is not one of the "first" widgets, or
   * if has not yet been placed in a column, because the size has not yet been requested.
   */
  bool get_column_for_first_widget(const Gtk::Widget& first, guint& column) const;

private:

  bool m_design_mode;

  //For drawing:
  Glib::RefPtr<Gdk::Window> m_refGdkWindow;
};

} //namespace Glom

#endif //GLOM_UTILITYWIDGETS_FLOWTABLE_H
